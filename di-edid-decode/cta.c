#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <libdisplay-info/cta.h>

#include "di-edid-decode.h"

static void
printf_cta_svds(const struct di_cta_svd *const *svds)
{
	size_t i;
	const struct di_cta_svd *svd;

	for (i = 0; svds[i] != NULL; i++) {
		svd = svds[i];

		printf("    VIC %3" PRIu8, svd->vic);
		if (svd->native)
			printf(" (native)");
		printf("\n");

		// TODO: print detailed mode info
	}
}

static uint8_t
encode_max_luminance(float max)
{
	if (max == 0)
		return 0;
	return (uint8_t) (log2f(max / 50) * 32);
}

static uint8_t
encode_min_luminance(float min, float max)
{
	if (min == 0)
		return 0;
	return (uint8_t) (255 * sqrtf(min / max * 100));
}

static void
print_cta_hdr_static_metadata(const struct di_cta_hdr_static_metadata_block *metadata)
{
	printf("    Electro optical transfer functions:\n");
	if (metadata->eotfs->traditional_sdr)
		printf("      Traditional gamma - SDR luminance range\n");
	if (metadata->eotfs->traditional_hdr)
		printf("      Traditional gamma - HDR luminance range\n");
	if (metadata->eotfs->pq)
		printf("      SMPTE ST2084\n");
	if (metadata->eotfs->hlg)
		printf("      Hybrid Log-Gamma\n");

	printf("    Supported static metadata descriptors:\n");
	if (metadata->descriptors->type1)
		printf("      Static metadata type 1\n");

	/* TODO: figure out a way to print raw values? */
	if (metadata->desired_content_max_luminance != 0)
		printf("    Desired content max luminance: %" PRIu8 " (%.3f cd/m^2)\n",
		       encode_max_luminance(metadata->desired_content_max_luminance),
		       metadata->desired_content_max_luminance);
	if (metadata->desired_content_max_frame_avg_luminance != 0)
		printf("    Desired content max frame-average luminance: %" PRIu8 " (%.3f cd/m^2)\n",
		       encode_max_luminance(metadata->desired_content_max_frame_avg_luminance),
		       metadata->desired_content_max_frame_avg_luminance);
	if (metadata->desired_content_min_luminance != 0)
		printf("    Desired content min luminance: %" PRIu8 " (%.3f cd/m^2)\n",
		       encode_min_luminance(metadata->desired_content_min_luminance,
					    metadata->desired_content_max_luminance),
		       metadata->desired_content_min_luminance);
}

static void
print_cta_vesa_transfer_characteristics(const struct di_cta_vesa_transfer_characteristics *tf)
{
	size_t i;

	switch (tf->usage) {
	case DI_CTA_VESA_TRANSFER_CHARACTERISTIC_USAGE_WHITE:
		printf("    White");
		break;
	case DI_CTA_VESA_TRANSFER_CHARACTERISTIC_USAGE_RED:
		printf("    Red");
		break;
	case DI_CTA_VESA_TRANSFER_CHARACTERISTIC_USAGE_GREEN:
		printf("    Green");
		break;
	case DI_CTA_VESA_TRANSFER_CHARACTERISTIC_USAGE_BLUE:
		printf("    Blue");
		break;
	}

	printf(" transfer characteristics:");
	for (i = 0; i < tf->points_len; i++)
		printf(" %u", (uint16_t) roundf(tf->points[i] * 1023.0f));
	printf("\n");

	uncommon_features.cta_transfer_characteristics = true;
}

static const char *
cta_audio_format_name(enum di_cta_audio_format format)
{
	switch (format) {
	case DI_CTA_AUDIO_FORMAT_LPCM:
		return "Linear PCM";
	case DI_CTA_AUDIO_FORMAT_AC3:
		return "AC-3";
	case DI_CTA_AUDIO_FORMAT_MPEG1:
		return "MPEG 1 (Layers 1 & 2)";
	case DI_CTA_AUDIO_FORMAT_MP3:
		return "MPEG 1 Layer 3 (MP3)";
	case DI_CTA_AUDIO_FORMAT_MPEG2:
		return "MPEG2 (multichannel)";
	case DI_CTA_AUDIO_FORMAT_AAC_LC:
		return "AAC LC";
	case DI_CTA_AUDIO_FORMAT_DTS:
		return "DTS";
	case DI_CTA_AUDIO_FORMAT_ATRAC:
		return "ATRAC";
	case DI_CTA_AUDIO_FORMAT_ONE_BIT_AUDIO:
		return "One Bit Audio";
	case DI_CTA_AUDIO_FORMAT_ENHANCED_AC3:
		return "Enhanced AC-3 (DD+)";
	case DI_CTA_AUDIO_FORMAT_DTS_HD:
		return "DTS-HD";
	case DI_CTA_AUDIO_FORMAT_MAT:
		return "MAT (MLP)";
	case DI_CTA_AUDIO_FORMAT_DST:
		return "DST";
	case DI_CTA_AUDIO_FORMAT_WMA_PRO:
		return "WMA Pro";
	case DI_CTA_AUDIO_FORMAT_MPEG4_HE_AAC:
		return "MPEG-4 HE AAC";
	case DI_CTA_AUDIO_FORMAT_MPEG4_HE_AAC_V2:
		return "MPEG-4 HE AAC v2";
	case DI_CTA_AUDIO_FORMAT_MPEG4_AAC_LC:
		return "MPEG-4 AAC LC";
	case DI_CTA_AUDIO_FORMAT_DRA:
		return "DRA";
	case DI_CTA_AUDIO_FORMAT_MPEG4_HE_AAC_MPEG_SURROUND:
		return "MPEG-4 HE AAC + MPEG Surround";
	case DI_CTA_AUDIO_FORMAT_MPEG4_AAC_LC_MPEG_SURROUND:
		return "MPEG-4 AAC LC + MPEG Surround";
	case DI_CTA_AUDIO_FORMAT_MPEGH_3D:
		return "MPEG-H 3D Audio";
	case DI_CTA_AUDIO_FORMAT_AC4:
		return "AC-4";
	case DI_CTA_AUDIO_FORMAT_LPCM_3D:
		return "L-PCM 3D Audio";
	}
	abort();
}

static const char *
cta_sad_mpegh_3d_level_name(enum di_cta_sad_mpegh_3d_level level)
{
	switch (level) {
	case DI_CTA_SAD_MPEGH_3D_LEVEL_UNSPECIFIED:
		return "Unspecified";
	case DI_CTA_SAD_MPEGH_3D_LEVEL_1:
		return "Level 1";
	case DI_CTA_SAD_MPEGH_3D_LEVEL_2:
		return "Level 2";
	case DI_CTA_SAD_MPEGH_3D_LEVEL_3:
		return "Level 3";
	case DI_CTA_SAD_MPEGH_3D_LEVEL_4:
		return "Level 4";
	case DI_CTA_SAD_MPEGH_3D_LEVEL_5:
		return "Level 5";
	}
	abort();
}

static void
print_cta_sads(const struct di_cta_sad *const *sads)
{
	size_t i;
	const struct di_cta_sad *sad;

	for (i = 0; sads[i] != NULL; i++) {
		sad = sads[i];

		printf("    %s:\n", cta_audio_format_name(sad->format));
		if (sad->max_channels != 0)
			printf("      Max channels: %d\n", sad->max_channels);

		if (sad->mpegh_3d)
			printf("      MPEG-H 3D Audio Level: %s\n",
			       cta_sad_mpegh_3d_level_name(sad->mpegh_3d->level));

		printf("      Supported sample rates (kHz):");
		if (sad->supported_sample_rates->has_192_khz)
			printf(" 192");
		if (sad->supported_sample_rates->has_176_4_khz)
			printf(" 176.4");
		if (sad->supported_sample_rates->has_96_khz)
			printf(" 96");
		if (sad->supported_sample_rates->has_88_2_khz)
			printf(" 88.2");
		if (sad->supported_sample_rates->has_48_khz)
			printf(" 48");
		if (sad->supported_sample_rates->has_44_1_khz)
			printf(" 44.1");
		if (sad->supported_sample_rates->has_32_khz)
			printf(" 32");
		printf("\n");

		if (sad->lpcm) {
			printf("      Supported sample sizes (bits):");
			if (sad->lpcm->has_sample_size_24_bits)
				printf(" 24");
			if (sad->lpcm->has_sample_size_20_bits)
				printf(" 20");
			if (sad->lpcm->has_sample_size_16_bits)
				printf(" 16");
			printf("\n");
		}

		if (sad->max_bitrate_kbs != 0)
			printf("      Maximum bit rate: %d kb/s\n", sad->max_bitrate_kbs);

		if (sad->enhanced_ac3 && sad->enhanced_ac3->supports_joint_object_coding)
			printf("      Supports Joint Object Coding\n");
		if (sad->enhanced_ac3 && sad->enhanced_ac3->supports_joint_object_coding_ACMOD28)
			printf("      Supports Joint Object Coding with ACMOD28\n");

		if (sad->mat) {
			if (sad->mat->supports_object_audio_and_channel_based) {
				printf("      Supports Dolby TrueHD, object audio PCM and channel-based PCM\n");
				printf("      Hash calculation %srequired for object audio PCM or channel-based PCM\n",
				       sad->mat->requires_hash_calculation ? "" : "not ");
			} else {
				printf("      Supports only Dolby TrueHD\n");
			}
		}

		if (sad->wma_pro) {
			printf("      Profile: %u\n",sad->wma_pro->profile);
		}

		if (sad->mpegh_3d && sad->mpegh_3d->low_complexity_profile)
			printf("      Supports MPEG-H 3D Audio Low Complexity Profile\n");
		if (sad->mpegh_3d && sad->mpegh_3d->baseline_profile)
			printf("      Supports MPEG-H 3D Audio Baseline Profile\n");

		if (sad->mpeg_aac) {
			printf("      AAC audio frame lengths:%s%s\n",
			       sad->mpeg_aac->has_frame_length_1024 ? " 1024_TL" : "",
			       sad->mpeg_aac->has_frame_length_960 ? " 960_TL" : "");
		}

		if (sad->mpeg_surround) {
			printf("      Supports %s signaled MPEG Surround data\n",
			       sad->mpeg_surround->signaling == DI_CTA_SAD_MPEG_SURROUND_SIGNALING_IMPLICIT ?
			       "only implicitly" : "implicitly and explicitly");
		}

		if (sad->mpeg_aac_le && sad->mpeg_aac_le->supports_multichannel_sound)
			printf("      Supports 22.2ch System H\n");
	}
}

static const char *
cta_data_block_tag_name(enum di_cta_data_block_tag tag)
{
	switch (tag) {
	case DI_CTA_DATA_BLOCK_AUDIO:
		return "Audio Data Block";
	case DI_CTA_DATA_BLOCK_VIDEO:
		return "Video Data Block";
	case DI_CTA_DATA_BLOCK_SPEAKER_ALLOC:
		return "Speaker Allocation Data Block";
	case DI_CTA_DATA_BLOCK_VESA_DISPLAY_TRANSFER_CHARACTERISTIC:
		return "VESA Display Transfer Characteristics Data Block";
	case DI_CTA_DATA_BLOCK_VIDEO_CAP:
		return "Video Capability Data Block";
	case DI_CTA_DATA_BLOCK_VESA_DISPLAY_DEVICE:
		return "VESA Video Display Device Data Block";
	case DI_CTA_DATA_BLOCK_COLORIMETRY:
		return "Colorimetry Data Block";
	case DI_CTA_DATA_BLOCK_HDR_STATIC_METADATA:
		return "HDR Static Metadata Data Block";
	case DI_CTA_DATA_BLOCK_HDR_DYNAMIC_METADATA:
		return "HDR Dynamic Metadata Data Block";
	case DI_CTA_DATA_BLOCK_VIDEO_FORMAT_PREF:
		return "Video Format Preference Data Block";
	case DI_CTA_DATA_BLOCK_YCBCR420:
		return "YCbCr 4:2:0 Video Data Block";
	case DI_CTA_DATA_BLOCK_YCBCR420_CAP_MAP:
		return "YCbCr 4:2:0 Capability Map Data Block";
	case DI_CTA_DATA_BLOCK_HDMI_AUDIO:
		return "HDMI Audio Data Block";
	case DI_CTA_DATA_BLOCK_ROOM_CONFIG:
		return "Room Configuration Data Block";
	case DI_CTA_DATA_BLOCK_SPEAKER_LOCATION:
		return "Speaker Location Data Block";
	case DI_CTA_DATA_BLOCK_INFOFRAME:
		return "InfoFrame Data Block";
	case DI_CTA_DATA_BLOCK_DISPLAYID_VIDEO_TIMING_VII:
		return "DisplayID Type VII Video Timing Data Block";
	case DI_CTA_DATA_BLOCK_DISPLAYID_VIDEO_TIMING_VIII:
		return "DisplayID Type VIII Video Timing Data Block";
	case DI_CTA_DATA_BLOCK_DISPLAYID_VIDEO_TIMING_X:
		return "DisplayID Type X Video Timing Data Block";
	case DI_CTA_DATA_BLOCK_HDMI_EDID_EXT_OVERRIDE :
		return "HDMI Forum EDID Extension Override Data Block";
	case DI_CTA_DATA_BLOCK_HDMI_SINK_CAP:
		return "HDMI Forum Sink Capability Data Block";
	}
	return "Unknown CTA-861 Data Block";
}

static const char *
video_cap_over_underscan_name(enum di_cta_video_cap_over_underscan over_underscan,
			      const char *unknown)
{
	switch (over_underscan) {
	case DI_CTA_VIDEO_CAP_UNKNOWN_OVER_UNDERSCAN:
		return unknown;
	case DI_CTA_VIDEO_CAP_ALWAYS_OVERSCAN:
		return "Always Overscanned";
	case DI_CTA_VIDEO_CAP_ALWAYS_UNDERSCAN:
		return "Always Underscanned";
	case DI_CTA_VIDEO_CAP_BOTH_OVER_UNDERSCAN:
		return "Supports both over- and underscan";
	}
	abort();
}

void
print_cta(const struct di_edid_cta *cta)
{
	const struct di_edid_cta_flags *cta_flags;
	const struct di_cta_data_block *const *data_blocks;
	const struct di_cta_data_block *data_block;
	enum di_cta_data_block_tag data_block_tag;
	const struct di_cta_svd *const *svds;
	const struct di_cta_video_cap_block *video_cap;
	const struct di_cta_colorimetry_block *colorimetry;
	const struct di_cta_hdr_static_metadata_block *hdr_static_metadata;
	const struct di_cta_vesa_transfer_characteristics *transfer_characteristics;
	const struct di_cta_sad *const *sads;
	size_t i;
	const struct di_edid_detailed_timing_def *const *detailed_timing_defs;

	printf("  Revision: %d\n", di_edid_cta_get_revision(cta));

	cta_flags = di_edid_cta_get_flags(cta);
	if (cta_flags->it_underscan) {
		printf("  Underscans IT Video Formats by default\n");
	}
	if (cta_flags->basic_audio) {
		printf("  Basic audio support\n");
	}
	if (cta_flags->ycc444) {
		printf("  Supports YCbCr 4:4:4\n");
	}
	if (cta_flags->ycc422) {
		printf("  Supports YCbCr 4:2:2\n");
	}
	printf("  Native detailed modes: %d\n", cta_flags->native_dtds);

	data_blocks = di_edid_cta_get_data_blocks(cta);
	for (i = 0; data_blocks[i] != NULL; i++) {
		data_block = data_blocks[i];

		data_block_tag = di_cta_data_block_get_tag(data_block);
		printf("  %s:\n", cta_data_block_tag_name(data_block_tag));

		switch (data_block_tag) {
		case DI_CTA_DATA_BLOCK_VIDEO:
			svds = di_cta_data_block_get_svds(data_block);
			printf_cta_svds(svds);
			break;
		case DI_CTA_DATA_BLOCK_VIDEO_CAP:
			video_cap = di_cta_data_block_get_video_cap(data_block);
			printf("    YCbCr quantization: %s\n",
			       video_cap->selectable_ycc_quantization_range ?
			       "Selectable (via AVI YQ)" : "No Data");
			printf("    RGB quantization: %s\n",
			       video_cap->selectable_ycc_quantization_range ?
			       "Selectable (via AVI Q)" : "No Data");
			printf("    PT scan behavior: %s\n",
			       video_cap_over_underscan_name(video_cap->pt_over_underscan,
							     "No Data"));
			printf("    IT scan behavior: %s\n",
			       video_cap_over_underscan_name(video_cap->it_over_underscan,
							     "IT video formats not supported"));
			printf("    CE scan behavior: %s\n",
			       video_cap_over_underscan_name(video_cap->ce_over_underscan,
							     "CE video formats not supported"));
			break;
		case DI_CTA_DATA_BLOCK_COLORIMETRY:
			colorimetry = di_cta_data_block_get_colorimetry(data_block);
			if (colorimetry->xvycc_601)
				printf("    xvYCC601\n");
			if (colorimetry->xvycc_709)
				printf("    xvYCC709\n");
			if (colorimetry->sycc_601)
				printf("    sYCC601\n");
			if (colorimetry->opycc_601)
				printf("    opYCC601\n");
			if (colorimetry->oprgb)
				printf("    opRGB\n");
			if (colorimetry->bt2020_cycc)
				printf("    BT2020cYCC\n");
			if (colorimetry->bt2020_ycc)
				printf("    BT2020YCC\n");
			if (colorimetry->bt2020_rgb)
				printf("    BT2020RGB\n");
			if (colorimetry->ictcp)
				printf("    ICtCp\n");
			if (colorimetry->st2113_rgb)
				printf("    ST2113RGB\n");
			break;
		case DI_CTA_DATA_BLOCK_HDR_STATIC_METADATA:
			hdr_static_metadata = di_cta_data_block_get_hdr_static_metadata(data_block);
			print_cta_hdr_static_metadata(hdr_static_metadata);
			break;
		case DI_CTA_DATA_BLOCK_VESA_DISPLAY_TRANSFER_CHARACTERISTIC:
			transfer_characteristics = di_cta_data_block_get_vesa_transfer_characteristics(data_block);
			print_cta_vesa_transfer_characteristics(transfer_characteristics);
			break;
		case DI_CTA_DATA_BLOCK_AUDIO:
			sads = di_cta_data_block_get_sads(data_block);
			print_cta_sads(sads);
			break;
		default:
			break; /* Ignore */
		}
	}

	detailed_timing_defs = di_edid_cta_get_detailed_timing_defs(cta);
	if (detailed_timing_defs[0]) {
		printf("  Detailed Timing Descriptors:\n");
	}
	for (i = 0; detailed_timing_defs[i] != NULL; i++) {
		print_detailed_timing_def(detailed_timing_defs[i]);
	}
}
