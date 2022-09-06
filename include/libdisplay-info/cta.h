#ifndef DI_CTA_H
#define DI_CTA_H

/**
 * libdisplay-info's low-level API for Consumer Technology Association
 * standards.
 *
 * The library implements CTA-861-H, available at:
 * https://shop.cta.tech/collections/standards/products/a-dtv-profile-for-uncompressed-high-speed-digital-interfaces-cta-861-h
 */

#include <stdbool.h>
#include <stdint.h>

/**
 * EDID CTA-861 extension block.
 */
struct di_edid_cta;

/**
 * Get the CTA extension revision (also referred to as "version" by the
 * specification).
 */
int
di_edid_cta_get_revision(const struct di_edid_cta *cta);

/**
 * Miscellaneous EDID CTA flags, defined in section 7.3.3.
 *
 * For CTA revision 1, all of the fields are zero.
 */
struct di_edid_cta_flags {
	/* Sink underscans IT Video Formats by default */
	bool it_underscan;
	/* Sink supports Basic Audio */
	bool basic_audio;
	/* Sink supports YCbCr 4:4:4 in addition to RGB */
	bool ycc444;
	/* Sink supports YCbCr 4:2:2 in addition to RGB */
	bool ycc422;
	/* Total number of native detailed timing descriptors */
	int native_dtds;
};

/**
 * Get miscellaneous CTA flags.
 */
const struct di_edid_cta_flags *
di_edid_cta_get_flags(const struct di_edid_cta *cta);

/**
 * CTA data block, defined in section 7.4.
 */
struct di_cta_data_block;

/**
 * Get CTA data blocks.
 *
 * The returned array is NULL-terminated.
 */
const struct di_cta_data_block *const *
di_edid_cta_get_data_blocks(const struct di_edid_cta *cta);

/**
 * CTA data block tag.
 *
 * Note, the enum values don't match the specification.
 */
enum di_cta_data_block_tag {
	/* Audio Data Block */
	DI_CTA_DATA_BLOCK_AUDIO = 1,
	/* Video Data Block */
	DI_CTA_DATA_BLOCK_VIDEO,
	/* Speaker Allocation Data Block */
	DI_CTA_DATA_BLOCK_SPEAKER_ALLOC,
	/* VESA Display Transfer Characteristic Data Block */
	DI_CTA_DATA_BLOCK_VESA_DISPLAY_TRANSFER_CHARACTERISTIC,

	/* Video Capability Data Block */
	DI_CTA_DATA_BLOCK_VIDEO_CAP,
	/* VESA Display Device Data Block */
	DI_CTA_DATA_BLOCK_VESA_DISPLAY_DEVICE,
	/* Colorimetry Data Block */
	DI_CTA_DATA_BLOCK_COLORIMETRY,
	/* HDR Static Metadata Data Block */
	DI_CTA_DATA_BLOCK_HDR_STATIC_METADATA,
	/* HDR Dynamic Metadata Data Block */
	DI_CTA_DATA_BLOCK_HDR_DYNAMIC_METADATA,
	/* Video Format Preference Data Block */
	DI_CTA_DATA_BLOCK_VIDEO_FORMAT_PREF,
	/* YCbCr 4:2:0 Video Data Block */
	DI_CTA_DATA_BLOCK_YCBCR420,
	/* YCbCr 4:2:0 Capability Map Data Block */
	DI_CTA_DATA_BLOCK_YCBCR420_CAP_MAP,
	/* HDMI Audio Data Block */
	DI_CTA_DATA_BLOCK_HDMI_AUDIO,
	/* Room Configuration Data Block */
	DI_CTA_DATA_BLOCK_ROOM_CONFIG,
	/* Speaker Location Data Block */
	DI_CTA_DATA_BLOCK_SPEAKER_LOCATION,
	/* InfoFrame Data Block */
	DI_CTA_DATA_BLOCK_INFOFRAME,
	/* DisplayID Type VII Video Timing Data Block */
	DI_CTA_DATA_BLOCK_DISPLAYID_VIDEO_TIMING_VII,
	/* DisplayID Type VIII Video Timing Data Block */
	DI_CTA_DATA_BLOCK_DISPLAYID_VIDEO_TIMING_VIII,
	/* DisplayID Type X Video Timing Data Block */
	DI_CTA_DATA_BLOCK_DISPLAYID_VIDEO_TIMING_X,
	/* HDMI Forum EDID Extension Override Data Block */
	DI_CTA_DATA_BLOCK_HDMI_EDID_EXT_OVERRIDE,
	/* HDMI Forum Sink Capability Data Block */
	DI_CTA_DATA_BLOCK_HDMI_SINK_CAP,
};

/**
 * Get the tag of the CTA data block.
 */
enum di_cta_data_block_tag
di_cta_data_block_get_tag(const struct di_cta_data_block *block);

/**
 * Audio formats, defined in tables 37 and 39.
 *
 * Note, the enum values don't match the specification.
 */
enum di_cta_audio_format {
	/* L-PCM */
	DI_CTA_AUDIO_FORMAT_LPCM = 1,
	/* AC-3 */
	DI_CTA_AUDIO_FORMAT_AC3,
	/* MPEG-1 (layers 1 & 2) */
	DI_CTA_AUDIO_FORMAT_MPEG1,
	/* MP3 */
	DI_CTA_AUDIO_FORMAT_MP3,
	/* MPEG-2 */
	DI_CTA_AUDIO_FORMAT_MPEG2,
	/* AAC LC */
	DI_CTA_AUDIO_FORMAT_AAC_LC,
	/* DTS */
	DI_CTA_AUDIO_FORMAT_DTS,
	/* ATRAC */
	DI_CTA_AUDIO_FORMAT_ATRAC,
	/* One Bit Audio */
	DI_CTA_AUDIO_FORMAT_ONE_BIT_AUDIO,
	/* Enhanced AC-3 */
	DI_CTA_AUDIO_FORMAT_ENHANCED_AC3,
	/* DTS-HD and DTS-UHD */
	DI_CTA_AUDIO_FORMAT_DTS_HD,
	/* MAT */
	DI_CTA_AUDIO_FORMAT_MAT,
	/* DST */
	DI_CTA_AUDIO_FORMAT_DST,
	/* WMA Pro */
	DI_CTA_AUDIO_FORMAT_WMA_PRO,

	/* MPEG-4 HE AAC */
	DI_CTA_AUDIO_FORMAT_MPEG4_HE_AAC,
	/* MPEG-4 HE AAC v2 */
	DI_CTA_AUDIO_FORMAT_MPEG4_HE_AAC_V2,
	/* MPEG-4 AAC LC */
	DI_CTA_AUDIO_FORMAT_MPEG4_AAC_LC,
	/* DRA */
	DI_CTA_AUDIO_FORMAT_DRA,
	/* MPEG-4 HE AAC + MPEG Surround */
	DI_CTA_AUDIO_FORMAT_MPEG4_HE_AAC_MPEG_SURROUND,
	/* MPEG-4 AAC LC + MPEG Surround */
	DI_CTA_AUDIO_FORMAT_MPEG4_AAC_LC_MPEG_SURROUND,
	/* MPEG-H 3D Audio */
	DI_CTA_AUDIO_FORMAT_MPEGH_3D,
	/* AC-4 */
	DI_CTA_AUDIO_FORMAT_AC4,
	/* L-PCM 3D Audio */
	DI_CTA_AUDIO_FORMAT_LPCM_3D,
};

struct di_cta_sad_sample_rates {
	bool has_192_khz; /* 192 kHz */
	bool has_176_4_khz; /* 176.4 kHz */
	bool has_96_khz; /* 96 kHz */
	bool has_88_2_khz; /* 88.2 kHz */
	bool has_48_khz; /* 48 kHz */
	bool has_44_1_khz; /* 44.1 kHz */
	bool has_32_khz; /* 32 kHz */
};

enum di_cta_sad_mpegh_3d_level {
	DI_CTA_SAD_MPEGH_3D_LEVEL_UNSPECIFIED = 0,
	DI_CTA_SAD_MPEGH_3D_LEVEL_1 = 1,
	DI_CTA_SAD_MPEGH_3D_LEVEL_2 = 2,
	DI_CTA_SAD_MPEGH_3D_LEVEL_3 = 3,
	DI_CTA_SAD_MPEGH_3D_LEVEL_4 = 4,
	DI_CTA_SAD_MPEGH_3D_LEVEL_5 = 5,
};

struct di_cta_sad_mpegh_3d {
	/* Maximum supported MPEG-H 3D level, zero if unspecified */
	enum di_cta_sad_mpegh_3d_level level;
	/* True if MPEG-H 3D Audio Low Complexity Profile is supported */
	bool low_complexity_profile;
	/* True if MPEG-H 3D Audio Baseline Profile is supported */
	bool baseline_profile;
};

struct di_cta_sad_mpeg_aac {
	/* True if AAC audio frame lengths of 960 samples is supported */
	bool has_frame_length_960;
	/* True if AAC audio frame lengths of 1024 samples is supported */
	bool has_frame_length_1024;
};

enum di_cta_sad_mpeg_surround_signaling {
	/* Only implicitly signaled MPEG Surround data supported */
	DI_CTA_SAD_MPEG_SURROUND_SIGNALING_IMPLICIT = 0,
	/* Implicitly and explicitly signaled MPEG Surround data supported */
	DI_CTA_SAD_MPEG_SURROUND_SIGNALING_IMPLICIT_AND_EXPLICIT = 1,
};

struct di_cta_sad_mpeg_surround {
	/* MPEG Surround signaling */
	enum di_cta_sad_mpeg_surround_signaling signaling;
};

struct di_cta_sad_mpeg_aac_le {
	/* True if Rec. ITU-R BS.2051 System H 22.2 multichannel sound is supported */
	bool supports_multichannel_sound;
};

struct di_cta_sad_lpcm {
	bool has_sample_size_24_bits; /* 24 bits */
	bool has_sample_size_20_bits; /* 20 bits */
	bool has_sample_size_16_bits; /* 16 bits */
};

struct di_cta_sad_enhanced_ac3 {
	bool supports_joint_object_coding;
	bool supports_joint_object_coding_ACMOD28;
};

struct di_cta_sad_mat {
	bool supports_object_audio_and_channel_based;
	bool requires_hash_calculation;
};

struct di_cta_sad_wma_pro {
	int profile;
};

/**
 * A CTA short audio descriptor (SAD), defined in section 7.5.2.
 */
struct di_cta_sad {
	/* Format */
	enum di_cta_audio_format format;
	/* Maximum number of channels, zero if unset */
	int32_t max_channels;
	/* Supported sample rates */
	const struct di_cta_sad_sample_rates *supported_sample_rates;
	/* Maximum bitrate (kb/s), zero if unset */
	int32_t max_bitrate_kbs;
	/* Additional metadata for LPCM, NULL unless format is
	 * DI_CTA_AUDIO_FORMAT_LPCM or DI_CTA_AUDIO_FORMAT_LPCM_3D */
	const struct di_cta_sad_lpcm *lpcm;
	/* Additional metadata for MPEG-H 3D Audio, NULL unless format is
	 * DI_CTA_AUDIO_FORMAT_MPEGH_3D */
	const struct di_cta_sad_mpegh_3d *mpegh_3d;
	/* Additional metadata for MPEG4 AAC, NULL unless format is
	 * DI_CTA_AUDIO_FORMAT_MPEG4_HE_AAC or
	 * DI_CTA_AUDIO_FORMAT_MPEG4_HE_AAC_V2 or
	 * DI_CTA_AUDIO_FORMAT_MPEG4_AAC_LC or
	 * DI_CTA_AUDIO_FORMAT_MPEG4_HE_AAC_MPEG_SURROUND or
	 * DI_CTA_AUDIO_FORMAT_MPEG4_AAC_LC_MPEG_SURROUND */
	const struct di_cta_sad_mpeg_aac *mpeg_aac;
	/* Additional metadata for MPEG4 Surround, NULL unless format is
	 * DI_CTA_AUDIO_FORMAT_MPEG4_HE_AAC_MPEG_SURROUND or
	 * DI_CTA_AUDIO_FORMAT_MPEG4_AAC_LC_MPEG_SURROUND */
	const struct di_cta_sad_mpeg_surround *mpeg_surround;
	/* Additional metadata for MPEG4 AAC LC, NULL unless format is
	 * DI_CTA_AUDIO_FORMAT_MPEG4_AAC_LC */
	const struct di_cta_sad_mpeg_aac_le *mpeg_aac_le;
	/* Additional metadata for Enhanced AC-3, NULL unless format is
	 * DI_CTA_AUDIO_FORMAT_ENHANCED_AC3 */
	const struct di_cta_sad_enhanced_ac3 *enhanced_ac3;
	/* Additional metadata for Dolby MAT, NULL unless format is
	 * DI_CTA_AUDIO_FORMAT_MAT */
	const struct di_cta_sad_mat *mat;
	/* Additional metadata for WMA Pro, NULL unless format is
	 * DI_CTA_AUDIO_FORMAT_WMA_PRO */
	const struct di_cta_sad_wma_pro *wma_pro;
};

/**
 * Get an array of short audio descriptors from a CTA data block.
 *
 * Returns NULL if the data block tag is not DI_CTA_DATA_BLOCK_AUDIO.
 *
 * The returned array is NULL-terminated.
 */
const struct di_cta_sad *const *
di_cta_data_block_get_sads(const struct di_cta_data_block *data_block);

/**
 * Over- and underscan capability.
 */
enum di_cta_video_cap_over_underscan {
	/* No data */
	DI_CTA_VIDEO_CAP_UNKNOWN_OVER_UNDERSCAN = 0x00,
	/* Always overscanned */
	DI_CTA_VIDEO_CAP_ALWAYS_OVERSCAN = 0x01,
	/* Always underscanned */
	DI_CTA_VIDEO_CAP_ALWAYS_UNDERSCAN = 0x02,
	/* Supports both over- and underscan */
	DI_CTA_VIDEO_CAP_BOTH_OVER_UNDERSCAN = 0x03,
};

/**
 * Video capability data block (VCDB), defined in section 7.5.6.
 */
struct di_cta_video_cap_block {
	/* If set to true, YCC quantization range is selectable (via AVI YQ). */
	bool selectable_ycc_quantization_range;
	/* If set to true, RGB quantization range is selectable (via AVI Q). */
	bool selectable_rgb_quantization_range;
	/* Overscan/underscan behavior for PT video formats (if set to unknown,
	 * use the IT/CE behavior) */
	enum di_cta_video_cap_over_underscan pt_over_underscan;
	/* Overscan/underscan behavior for IT video formats */
	enum di_cta_video_cap_over_underscan it_over_underscan;
	/* Overscan/underscan behavior for CE video formats */
	enum di_cta_video_cap_over_underscan ce_over_underscan;
};

/**
 * Get the video capabilities from a CTA data block.
 *
 * Returns NULL if the data block tag is not DI_CTA_DATA_BLOCK_VIDEO_CAP.
 */
const struct di_cta_video_cap_block *
di_cta_data_block_get_video_cap(const struct di_cta_data_block *block);

/**
 * CTA colorimetry data block, defined in section 7.5.5.
 */
struct di_cta_colorimetry_block {
	/* Standard Definition Colorimetry based on IEC 61966-2-4 */
	bool xvycc_601;
	/* High Definition Colorimetry based on IEC 61966-2-4 */
	bool xvycc_709;
	/* Colorimetry based on IEC 61966-2-1/Amendment 1 */
	bool sycc_601;
	/* Colorimetry based on IEC 61966-2-5, Annex A */
	bool opycc_601;
	/* Colorimetry based on IEC 61966-2-5 */
	bool oprgb;
	/* Colorimetry based on Rec. ITU-R BT.2020 Y'cC'bcC'rc */
	bool bt2020_cycc;
	/* Colorimetry based on Rec. ITU-R BT.2020 Y'C'bC'r */
	bool bt2020_ycc;
	/* Colorimetry based on Rec. ITU-R BT.2020 R'G'B' */
	bool bt2020_rgb;
	/* Colorimetry based on SMPTE ST 2113 R'G'B' */
	bool st2113_rgb;
	/* Colorimetry based on Rec. ITU-R BT.2100 ICtCp */
	bool ictcp;
};

/**
 * Get the colorimetry data from a CTA data block.
 *
 * Returns NULL if the data block tag is not DI_CTA_DATA_BLOCK_COLORIMETRY.
 */
const struct di_cta_colorimetry_block *
di_cta_data_block_get_colorimetry(const struct di_cta_data_block *block);

/**
 * Supported Electro-Optical Transfer Functions for a CTA HDR static metadata
 * block.
 */
struct di_cta_hdr_static_metadata_block_eotfs {
	/* Traditional gamma - SDR luminance range */
	bool traditional_sdr;
	/* Traditional gamma - HDR luminance range */
	bool traditional_hdr;
	/* Perceptual Quantization (PQ) based on SMPTE ST 2084 */
	bool pq;
	/* Hybrid Log-Gamma (HLG) based on Rec. ITU-R BT.2100 */
	bool hlg;
};

/**
 * Supported static metadata descriptors for a CTA HDR static metadata block.
 */
struct di_cta_hdr_static_metadata_block_descriptors {
	/* Static Metadata Type 1 */
	bool type1;
};

/**
 * CTA HDR static metadata block, defined in section 7.5.13.
 */
struct di_cta_hdr_static_metadata_block {
	/* Desired content max luminance (cd/m²), zero if unset */
	float desired_content_max_luminance;
	/* Desired content max frame-average luminance (cd/m²), zero if unset */
	float desired_content_max_frame_avg_luminance;
	/* Desired content min luminance (cd/m²), zero if unset */
	float desired_content_min_luminance;
	/* Supported EOFTs */
	const struct di_cta_hdr_static_metadata_block_eotfs *eotfs;
	/* Supported descriptors */
	const struct di_cta_hdr_static_metadata_block_descriptors *descriptors;
};

/**
 * Get the HDR static metadata from a CTA data block.
 *
 * Returns NULL if the data block tag is not
 * DI_CTA_DATA_BLOCK_HDR_STATIC_METADATA.
 */
const struct di_cta_hdr_static_metadata_block *
di_cta_data_block_get_hdr_static_metadata(const struct di_cta_data_block *block);

/**
 * A Short Video Descriptor (SVD).
 */
struct di_cta_svd {
	/* Video Identification Code (VIC) */
	uint8_t vic;
	/* Whether this is a native video format */
	bool native;
};

/**
 * Get an array of short video descriptors from a CTA data block.
 *
 * Returns NULL if the data block tag is not DI_CTA_DATA_BLOCK_VIDEO.
 *
 * The returned array is NULL-terminated.
 */
const struct di_cta_svd *const *
di_cta_data_block_get_svds(const struct di_cta_data_block *block);

enum di_cta_vesa_transfer_characteristics_usage {
	/* White transfer characteristic */
	DI_CTA_VESA_TRANSFER_CHARACTERISTIC_USAGE_WHITE = 0,
	/* Red transfer characteristic */
	DI_CTA_VESA_TRANSFER_CHARACTERISTIC_USAGE_RED = 1,
	/* Green transfer characteristic */
	DI_CTA_VESA_TRANSFER_CHARACTERISTIC_USAGE_GREEN = 2,
	/* Blue transfer characteristic */
	DI_CTA_VESA_TRANSFER_CHARACTERISTIC_USAGE_BLUE = 3,
};

/**
 * VESA Display Transfer Characteristic Data Block, defined in VESA Display
 * Transfer Characteristics Data Block Standard Version 1.0
 *
 * Contains 8, 16 or 32 evenly distributed points on the input axis describing
 * the normalized relative luminance at that input. The first value includes the
 * relative black level luminance.
 */
struct di_cta_vesa_transfer_characteristics {
	enum di_cta_vesa_transfer_characteristics_usage usage;
	uint8_t points_len;
	float points[32];
};

/**
 * Get the Display Transfer Characteristic from a CTA data block.
 *
 * Returns NULL if the data block tag is not
 * DI_CTA_DATA_BLOCK_VESA_DISPLAY_TRANSFER_CHARACTERISTIC.
 *
 * Upstream is not aware of any EDID blob containing a Display Transfer
 * Characteristic data block.
 * If such a blob is found, please share it with upstream!
 */
const struct di_cta_vesa_transfer_characteristics *
di_cta_data_block_get_vesa_transfer_characteristics(const struct di_cta_data_block *block);

/**
 * Get a list of EDID detailed timing definitions.
 *
 * The returned array is NULL-terminated.
 */
const struct di_edid_detailed_timing_def *const *
di_edid_cta_get_detailed_timing_defs(const struct di_edid_cta *cta);

#endif
