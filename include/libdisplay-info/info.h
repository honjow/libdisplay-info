#ifndef DI_INFO_H
#define DI_INFO_H

#include <stddef.h>
#include <stdbool.h>

/**
 * libdisplay-info's high-level API.
 */

/**
 * Information about a display device.
 *
 * This includes at least one EDID or DisplayID blob.
 *
 * Use di_info_parse_edid() to create a struct di_info from an EDID blob.
 * DisplayID blobs are not yet supported.
 */
struct di_info;

/**
 * Parse an EDID blob.
 *
 * Callers do not need to keep the provided data pointer valid after calling
 * this function. Callers should destroy the returned pointer via
 * di_info_destroy().
 */
struct di_info *
di_info_parse_edid(const void *data, size_t size);

/**
 * Destroy a display device information structure.
 */
void
di_info_destroy(struct di_info *info);

/**
 * Returns the EDID the display device information was constructed with.
 *
 * The returned struct di_edid can be used to query low-level EDID information,
 * see <libdisplay-info/edid.h>. Users should prefer the high-level API if
 * possible.
 *
 * NULL is returned if the struct di_info doesn't contain an EDID. The returned
 * struct di_edid is valid until di_info_destroy().
 */
const struct di_edid *
di_info_get_edid(const struct di_info *info);

/**
 * Get the failure messages for this blob.
 *
 * NULL is returned if the blob conforms to the relevant specifications.
 */
const char *
di_info_get_failure_msg(const struct di_info *info);

/**
 * Get the make of the display device.
 *
 * This is the manufacturer name, either company name or PNP ID.
 * This string is informational and not meant to be used in programmatic
 * decisions, configuration keys, etc.
 *
 * The string is in UTF-8 and may contain any characters except ASCII control
 * codes.
 *
 * The caller is responsible for free'ing the returned string.
 * NULL is returned if the information is not available.
 */
char *
di_info_get_make(const struct di_info *info);

/**
 * Get the model of the display device.
 *
 * This is the product name/model string or product number.
 * This string is informational and not meant to be used in programmatic
 * decisions, configuration keys, etc.
 *
 * The string is in UTF-8 and may contain any characters except ASCII control
 * codes.
 *
 * The caller is responsible for free'ing the returned string.
 * NULL is returned if the information is not available.
 */
char *
di_info_get_model(const struct di_info *info);

/**
 * Get the serial of the display device.
 *
 * This is the product serial string or the serial number.
 * This string is informational and not meant to be used in programmatic
 * decisions, configuration keys, etc.
 *
 * The string is in UTF-8 and may contain any characters except ASCII control
 * codes.
 *
 * The caller is responsible for free'ing the returned string.
 * NULL is returned if the information is not available.
 */
char *
di_info_get_serial(const struct di_info *info);

/**
 * Display HDR static metadata
 */
struct di_hdr_static_metadata {
	/* Desired content max luminance (cd/m²), zero if unset */
	float desired_content_max_luminance;
	/* Desired content max frame-average luminance (cd/m²), zero if unset */
	float desired_content_max_frame_avg_luminance;
	/* Desired content min luminance (cd/m²), zero if unset */
	float desired_content_min_luminance;

	/* Supported metadata types: */

	/* Static Metadata Type 1 */
	bool type1;

	/* Supported EOFTs: */

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
 * Get HDR static metadata support information as defined in ANSI/CTA-861-H
 * as HDR Static Metadata Data Block.
 *
 * The returned pointer is owned by the struct di_info passed in. It remains
 * valid only as long as the di_info exists, and must not be freed by the
 * caller.
 *
 * This function does not return NULL. When HDR static metadata does not exist,
 * all luminance fields are zero and only traditional_sdr is flagged as
 * supported.
 */
const struct di_hdr_static_metadata *
di_info_get_hdr_static_metadata(const struct di_info *info);

/** CIE 1931 2-degree observer chromaticity coordinates */
struct di_chromaticity_cie1931 {
	float x;
	float y;
};

/** Display color primaries and default white point */
struct di_color_primaries {
	/* Are primary[] given */
	bool has_primaries;

	/* Is default_white given */
	bool has_default_white_point;

	/* Chromaticities of the primaries in RGB order
	 *
	 * Either all values are given, or they are all zeroes.
	 */
	struct di_chromaticity_cie1931 primary[3];

	/* Default white point chromaticity
	 *
	 * If non-zero, this should be the display white point when it has
	 * been reset to its factory defaults. Either both x and y are given,
	 * or they are both zero.
	 */
	struct di_chromaticity_cie1931 default_white;
};

/**
 * Get display color primaries and default white point
 *
 * Get the parameters of the default RGB colorimetry mode which is always
 * supported. Primaries for monochrome displays might be all zeroes.
 *
 * These primaries might not be display's physical primaries, but only the
 * primaries of the default RGB colorimetry signal when using IT Video Format
 * (ANSI/CTA-861-H, Section 5).
 *
 * The returned pointer is owned by the struct di_info passed in. It remains
 * valid only as long as the di_info exists, and must not be freed by the
 * caller.
 *
 * This function does not return NULL.
 */
const struct di_color_primaries *
di_info_get_default_color_primaries(const struct di_info *info);

#endif
