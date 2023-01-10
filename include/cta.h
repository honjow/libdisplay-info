#ifndef CTA_H
#define CTA_H

/**
 * Private header for the low-level CTA API.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <libdisplay-info/cta.h>

/**
 * The maximum number of data blocks in an EDID CTA block.
 *
 * Each data block takes at least 1 byte, the CTA block can hold 128 bytes, and
 * the mandatory fields take up 5 bytes (4 header bytes + checksum).
 */
#define EDID_CTA_MAX_DATA_BLOCKS 123
/**
 * The maximum number of detailed timing definitions included in an EDID CTA
 * block.
 *
 * The CTA extension leaves at most 122 bytes for timings, and each timing takes
 * 18 bytes.
 */
#define EDID_CTA_MAX_DETAILED_TIMING_DEFS 6
/**
 * The maximum number of SVD entries in a video data block.
 *
 * Each data block has its size described in a 5-bit field, so its maximum size
 * is 63 bytes, and each SVD uses 1 byte.
 */
#define EDID_CTA_MAX_VIDEO_BLOCK_ENTRIES 63
/**
 * The maximum number of SAD entries in an audio data block.
 *
 * Each data block has its size described in a 5-bit field, so its maximum size
 * is 63 bytes, and each SAD uses 3 bytes.
 */
#define EDID_CTA_MAX_AUDIO_BLOCK_ENTRIES 21

struct di_edid_cta {
	int revision;
	struct di_edid_cta_flags flags;

	/* NULL-terminated */
	struct di_cta_data_block *data_blocks[EDID_CTA_MAX_DATA_BLOCKS + 1];
	size_t data_blocks_len;

	/* NULL-terminated */
	struct di_edid_detailed_timing_def_priv *detailed_timing_defs[EDID_CTA_MAX_DETAILED_TIMING_DEFS + 1];
	size_t detailed_timing_defs_len;

	struct di_logger *logger;
};

struct di_cta_hdr_static_metadata_block_priv {
	struct di_cta_hdr_static_metadata_block base;
	struct di_cta_hdr_static_metadata_block_eotfs eotfs;
	struct di_cta_hdr_static_metadata_block_descriptors descriptors;
};

struct di_cta_hdr_dynamic_metadata_block_type3 {
	uint8_t unused;
};

struct di_cta_hdr_dynamic_metadata_block_priv {
	struct di_cta_hdr_dynamic_metadata_block base;
	struct di_cta_hdr_dynamic_metadata_block_type1 type1;
	struct di_cta_hdr_dynamic_metadata_block_type2 type2;
	struct di_cta_hdr_dynamic_metadata_block_type3 type3;
	struct di_cta_hdr_dynamic_metadata_block_type4 type4;
	struct di_cta_hdr_dynamic_metadata_block_type256 type256;
};

struct di_cta_video_block {
	/* NULL-terminated */
	struct di_cta_svd *svds[EDID_CTA_MAX_VIDEO_BLOCK_ENTRIES + 1];
	size_t svds_len;
};

struct di_cta_sad_priv {
	struct di_cta_sad base;
	struct di_cta_sad_sample_rates supported_sample_rates;
	struct di_cta_sad_lpcm lpcm;
	struct di_cta_sad_mpegh_3d mpegh_3d;
	struct di_cta_sad_mpeg_aac mpeg_aac;
	struct di_cta_sad_mpeg_surround mpeg_surround;
	struct di_cta_sad_mpeg_aac_le mpeg_aac_le;
	struct di_cta_sad_enhanced_ac3 enhanced_ac3;
	struct di_cta_sad_mat mat;
	struct di_cta_sad_wma_pro wma_pro;
};

struct di_cta_audio_block {
	/* NULL-terminated */
	struct di_cta_sad_priv *sads[EDID_CTA_MAX_AUDIO_BLOCK_ENTRIES + 1];
	size_t sads_len;
};

struct di_cta_data_block {
	enum di_cta_data_block_tag tag;

	/* Used for DI_CTA_DATA_BLOCK_VIDEO */
	struct di_cta_video_block video;
	/* used for DI_CTA_DATA_BLOCK_AUDIO */
	struct di_cta_audio_block audio;
	/* Used for DI_CTA_DATA_BLOCK_VIDEO_CAP */
	struct di_cta_video_cap_block video_cap;
	/* Used for DI_CTA_DATA_BLOCK_VESA_DISPLAY_DEVICE */
	struct di_cta_vesa_dddb vesa_dddb;
	/* Used for DI_CTA_DATA_BLOCK_COLORIMETRY */
	struct di_cta_colorimetry_block colorimetry;
	/* Used for DI_CTA_DATA_BLOCK_HDR_STATIC_METADATA */
	struct di_cta_hdr_static_metadata_block_priv hdr_static_metadata;
	/* Used for DI_CTA_DATA_BLOCK_HDR_DYNAMIC_METADATA */
	struct di_cta_hdr_dynamic_metadata_block_priv hdr_dynamic_metadata;
	/* Used for DI_CTA_DATA_BLOCK_VESA_DISPLAY_TRANSFER_CHARACTERISTIC */
	struct di_cta_vesa_transfer_characteristics vesa_transfer_characteristics;
};

bool
_di_edid_cta_parse(struct di_edid_cta *cta, const uint8_t *data, size_t size,
		   struct di_logger *logger);

void
_di_edid_cta_finish(struct di_edid_cta *cta);

#endif
