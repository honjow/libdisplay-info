#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "edid.h"

/**
 * The size of an EDID block, defined in section 2.2.
 */
#define EDID_BLOCK_SIZE 128
/**
 * The size of an EDID standard timing, defined in section 3.9.
 */
#define EDID_STANDARD_TIMING_SIZE 2
/**
 * The size of an EDID byte descriptor, defined in section 3.10.
 */
#define EDID_BYTE_DESCRIPTOR_SIZE 18

/**
 * Fixed EDID header, defined in section 3.1.
 */
static const uint8_t header[] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };

/**
 * Check whether a byte has a bit set.
 */
static bool
has_bit(uint8_t val, size_t index)
{
	return val & (1 << index);
}

/**
 * Extract a bit range from a byte.
 *
 * Both offsets are inclusive, start from zero, and high must be greater than low.
 */
static uint8_t
get_bit_range(uint8_t val, size_t high, size_t low)
{
	size_t n;
	uint8_t bitmask;

	assert(high <= 7 && high >= low);

	n = high - low + 1;
	bitmask = (uint8_t) ((1 << n) - 1);
	return (uint8_t) (val >> low) & bitmask;
}

static void
va_add_failure(struct di_edid *edid, const char fmt[], va_list args)
{
	if (!edid->failure_msg_file) {
		edid->failure_msg_file = open_memstream(&edid->failure_msg,
						        &edid->failure_msg_size);
		if (!edid->failure_msg_file) {
			return;
		}

		fprintf(edid->failure_msg_file, "Block 0, Base EDID:\n");
	}

	fprintf(edid->failure_msg_file, "  ");
	vfprintf(edid->failure_msg_file, fmt, args);
	fprintf(edid->failure_msg_file, "\n");
}

static void
add_failure(struct di_edid *edid, const char fmt[], ...)
{
	va_list args;

	va_start(args, fmt);
	va_add_failure(edid, fmt, args);
	va_end(args);
}

static void
add_failure_until(struct di_edid *edid, int revision, const char fmt[], ...)
{
	va_list args;

	if (edid->revision > revision) {
		return;
	}

	va_start(args, fmt);
	va_add_failure(edid, fmt, args);
	va_end(args);
}

static void
parse_version_revision(const uint8_t data[static EDID_BLOCK_SIZE],
		       int *version, int *revision)
{
	*version = (int) data[0x12];
	*revision = (int) data[0x13];
}

static size_t
parse_ext_count(const uint8_t data[static EDID_BLOCK_SIZE])
{
	return data[0x7E];
}

static bool
validate_block_checksum(const uint8_t data[static EDID_BLOCK_SIZE])
{
	uint8_t sum = 0;
	size_t i;

	for (i = 0; i < EDID_BLOCK_SIZE; i++) {
		sum += data[i];
	}

	return sum == 0;
}

static void
parse_vendor_product(const uint8_t data[static EDID_BLOCK_SIZE],
		     struct di_edid_vendor_product *out)
{
	uint16_t man;
	int year = 0;

	/* The ASCII 3-letter manufacturer code is encoded in 5-bit codes. */
	man = (uint16_t) ((data[0x08] << 8) | data[0x09]);
	out->manufacturer[0] = ((man >> 10) & 0x1F) + '@';
	out->manufacturer[1] = ((man >> 5) & 0x1F) + '@';
	out->manufacturer[2] = ((man >> 0) & 0x1F) + '@';

	out->product = (uint16_t) (data[0x0A] | (data[0x0B] << 8));
	out->serial = (uint32_t) (data[0x0C] |
				  (data[0x0D] << 8) |
				  (data[0x0E] << 16) |
				  (data[0x0F] << 24));

	if (data[0x11] >= 0x10) {
		year = data[0x11] + 1990;
	}

	if (data[0x10] == 0xFF) {
		/* Special flag for model year */
		out->model_year = year;
	} else {
		out->manufacture_year = year;
		if (data[0x10] > 0 && data[0x10] <= 54) {
			out->manufacture_week = data[0x10];
		}
	}
}

static bool
parse_video_input_digital(struct di_edid *edid, uint8_t video_input)
{
	uint8_t color_bit_depth, interface;
	struct di_edid_video_input_digital *digital = &edid->video_input_digital;

	if (edid->revision < 4) {
		/* TODO: parse EDID 1.3- fields */
		return true;
	}

	color_bit_depth = get_bit_range(video_input, 6, 4);
	if (color_bit_depth == 0x07) {
		/* Reserved */
		add_failure_until(edid, 4, "Color Bit Depth set to reserved value.");
	} else if (color_bit_depth != 0) {
		digital->color_bit_depth = 2 * color_bit_depth + 4;
	}

	interface = get_bit_range(video_input, 3, 0);
	switch (interface) {
	case DI_EDID_VIDEO_INPUT_DIGITAL_UNDEFINED:
	case DI_EDID_VIDEO_INPUT_DIGITAL_DVI:
	case DI_EDID_VIDEO_INPUT_DIGITAL_HDMI_A:
	case DI_EDID_VIDEO_INPUT_DIGITAL_HDMI_B:
	case DI_EDID_VIDEO_INPUT_DIGITAL_MDDI:
	case DI_EDID_VIDEO_INPUT_DIGITAL_DISPLAYPORT:
		digital->interface = interface;
		break;
	default:
		add_failure_until(edid, 4,
				  "Digital Video Interface Standard set to reserved value 0x%02x.",
				  interface);
		digital->interface = DI_EDID_VIDEO_INPUT_DIGITAL_UNDEFINED;
		break;
	}

	return true;
}

static bool
parse_basic_params_features(struct di_edid *edid,
			    const uint8_t data[static EDID_BLOCK_SIZE])
{
	uint8_t video_input, width, height, features;
	struct di_edid_screen_size *screen_size = &edid->screen_size;

	video_input = data[0x14];
	edid->is_digital = has_bit(video_input, 7);

	/* TODO: parse analog fields */
	if (edid->is_digital) {
		if (!parse_video_input_digital(edid, video_input)) {
			return false;
		}
	}

	/* v1.3 says screen size is undefined if either byte is zero, v1.4 says
	 * screen size and aspect ratio are undefined if both bytes are zero and
	 * encodes the aspect ratio if either byte is zero. */
	width = data[0x15];
	height = data[0x16];
	if (width > 0 && height > 0) {
		screen_size->width_cm = width;
		screen_size->height_cm = height;
	} else if (edid->revision >= 4) {
		if (width > 0) {
			screen_size->landscape_aspect_ratio = ((float) width + 99) / 100;
		} else if (height > 0) {
			screen_size->portait_aspect_ratio = ((float) height + 99) / 100;
		}
	}

	if (data[0x17] != 0xFF) {
		edid->gamma = ((float) data[0x17] + 100) / 100;
	} else {
		edid->gamma = 0;
	}

	features = data[0x18];

	edid->dpms.standby = has_bit(features, 7);
	edid->dpms.suspend = has_bit(features, 6);
	edid->dpms.off = has_bit(features, 5);

	if (edid->is_digital && edid->revision >= 4) {
		edid->color_encoding_formats.rgb444 = true;
		edid->color_encoding_formats.ycrcb444 = has_bit(features, 3);
		edid->color_encoding_formats.ycrcb422 = has_bit(features, 4);
		edid->display_color_type = DI_EDID_DISPLAY_COLOR_UNDEFINED;
	} else {
		edid->display_color_type = get_bit_range(features, 4, 3);
	}

	if (edid->revision >= 4) {
		edid->misc_features.has_preferred_timing = true;
		edid->misc_features.continuous_freq = has_bit(features, 0);
		edid->misc_features.preferred_timing_is_native = has_bit(features, 1);
	} else {
		edid->misc_features.default_gtf = has_bit(features, 0);
		edid->misc_features.has_preferred_timing = has_bit(features, 1);
	}
	edid->misc_features.srgb_is_primary = has_bit(features, 2);

	return true;
}

static float
decode_chromaticity_coord(uint8_t hi, uint8_t lo)
{
	uint16_t raw; /* only 10 bits are used */

	raw = (uint16_t) (hi << 2) | lo;
	return (float) raw / 1024;
}

static bool
parse_chromaticity_coords(struct di_edid *edid,
			  const uint8_t data[static EDID_BLOCK_SIZE])
{
	uint8_t lo;
	bool all_set, any_set;
	struct di_edid_chromaticity_coords *coords;

	coords = &edid->chromaticity_coords;

	lo = data[0x19];
	coords->red_x = decode_chromaticity_coord(data[0x1B], get_bit_range(lo, 7, 6));
	coords->red_y = decode_chromaticity_coord(data[0x1C], get_bit_range(lo, 5, 4));
	coords->green_x = decode_chromaticity_coord(data[0x1D], get_bit_range(lo, 3, 2));
	coords->green_y = decode_chromaticity_coord(data[0x1E], get_bit_range(lo, 1, 0));

	lo = data[0x1A];
	coords->blue_x = decode_chromaticity_coord(data[0x1F], get_bit_range(lo, 7, 6));
	coords->blue_y = decode_chromaticity_coord(data[0x20], get_bit_range(lo, 5, 4));
	coords->white_x = decode_chromaticity_coord(data[0x21], get_bit_range(lo, 3, 2));
	coords->white_y = decode_chromaticity_coord(data[0x22], get_bit_range(lo, 1, 0));

	/* Either all primaries coords must be set, either none must be set */
	any_set = coords->red_x != 0 || coords->red_y != 0
		  || coords->green_x != 0 || coords->green_y != 0
		  || coords->blue_x != 0 || coords->blue_y != 0;
	all_set = coords->red_x != 0 && coords->red_y != 0
		  && coords->green_x != 0 && coords->green_y != 0
		  && coords->blue_x != 0 && coords->blue_y != 0;
	if (any_set && !all_set) {
		add_failure(edid, "Some but not all primaries coordinates are unset.");
	}

	/* Both white-point coords must be set */
	if (coords->white_x == 0 || coords->white_y == 0) {
		add_failure(edid, "White-point coordinates are unset.");
	}

	return true;
}

static bool
parse_standard_timing(struct di_edid *edid,
		      const uint8_t data[static EDID_STANDARD_TIMING_SIZE])
{
	struct di_edid_standard_timing *t;

	if (data[0] == 0x01 && data[1] == 0x01) {
		/* Unused */
		return true;
	}
	if (data[0] == 0x00) {
		add_failure_until(edid, 4,
				  "Use 0x0101 as the invalid Standard Timings code, not 0x%02x%02x.",
				  data[0], data[1]);
		return true;
	}

	t = calloc(1, sizeof(*t));
	if (!t) {
		return false;
	}

	t->horiz_video = ((int32_t) data[0] + 31) * 8;
	t->aspect_ratio = get_bit_range(data[1], 7, 6);
	t->refresh_rate_hz = (int32_t) get_bit_range(data[1], 5, 0) + 60;

	edid->standard_timings[edid->standard_timings_len++] = t;
	return true;
}

static bool
parse_detailed_timing_def(struct di_edid *edid,
			  const uint8_t data[static EDID_BYTE_DESCRIPTOR_SIZE])
{
	struct di_edid_detailed_timing_def *def;
	int raw;
	uint8_t flags, stereo_hi, stereo_lo;

	def = calloc(1, sizeof(*def));
	if (!def) {
		return false;
	}

	raw = (data[1] << 8) | data[0];
	def->pixel_clock_hz = raw * 10 * 1000;

	def->horiz_video = (get_bit_range(data[4], 7, 4) << 8) | data[2];
	def->horiz_blank = (get_bit_range(data[4], 3, 0) << 8) | data[3];

	def->vert_video = (get_bit_range(data[7], 7, 4) << 8) | data[5];
	def->vert_blank = (get_bit_range(data[7], 3, 0) << 8) | data[6];

	def->horiz_front_porch = (get_bit_range(data[11], 7, 6) << 8) | data[8];
	def->horiz_sync_pulse = (get_bit_range(data[11], 5, 4) << 8) | data[9];
	def->vert_front_porch = (get_bit_range(data[11], 3, 2) << 4)
				| get_bit_range(data[10], 7, 4);
	def->vert_sync_pulse = (get_bit_range(data[11], 1, 0) << 4)
			       | get_bit_range(data[10], 3, 0);

	def->horiz_image_mm = (get_bit_range(data[14], 7, 4) << 8) | data[12];
	def->vert_image_mm = (get_bit_range(data[14], 3, 0) << 8) | data[13];
	if ((def->horiz_image_mm == 16 && def->vert_image_mm == 9)
	    || (def->horiz_image_mm == 4 && def->vert_image_mm == 3)) {
		/* Table 3.21 note 18.2: these are special cases and define the
		 * aspect ratio rather than the size in mm.
		 * TODO: expose these values */
		def->horiz_image_mm = def->vert_image_mm = 0;
	}

	def->horiz_border = data[15];
	def->vert_border = data[16];

	flags = data[17];

	def->interlaced = has_bit(flags, 7);

	stereo_hi = get_bit_range(flags, 6, 5);
	stereo_lo = get_bit_range(flags, 0, 0);
	if (stereo_hi == 0) {
		def->stereo = DI_EDID_DETAILED_TIMING_DEF_STEREO_NONE;
	} else {
		switch ((stereo_hi << 1) | stereo_lo) {
		case (1 << 1) | 0:
			def->stereo = DI_EDID_DETAILED_TIMING_DEF_STEREO_FIELD_SEQ_RIGHT;
			break;
		case (2 << 1) | 0:
			def->stereo = DI_EDID_DETAILED_TIMING_DEF_STEREO_FIELD_SEQ_LEFT;
			break;
		case (1 << 1) | 1:
			def->stereo = DI_EDID_DETAILED_TIMING_DEF_STEREO_2_WAY_INTERLEAVED_RIGHT;
			break;
		case (2 << 1) | 1:
			def->stereo = DI_EDID_DETAILED_TIMING_DEF_STEREO_2_WAY_INTERLEAVED_LEFT;
			break;
		case (3 << 1) | 0:
			def->stereo = DI_EDID_DETAILED_TIMING_DEF_STEREO_4_WAY_INTERLEAVED;
			break;
		case (3 << 1) | 1:
			def->stereo = DI_EDID_DETAILED_TIMING_DEF_STEREO_SIDE_BY_SIDE_INTERLEAVED;
			break;
		default:
			abort(); /* unreachable */
		}
	}

	/* TODO: parse analog/digital flags */

	edid->detailed_timing_defs[edid->detailed_timing_defs_len++] = def;
	return true;
}

static bool
decode_display_range_limits_offset(struct di_edid *edid, uint8_t flags,
				  int *max_offset, int *min_offset)
{
	switch (flags) {
	case 0x00:
		/* No offset */
		break;
	case 0x02:
		*max_offset = 255;
		break;
	case 0x03:
		*max_offset = 255;
		*min_offset = 255;
		break;
	default:
		add_failure_until(edid, 4,
				  "Range offset flags set to reserved value 0x%02x.",
				  flags);
		return false;
	}

	return true;
}

static bool
parse_display_range_limits(struct di_edid *edid,
			   const uint8_t data[static EDID_BYTE_DESCRIPTOR_SIZE],
			   struct di_edid_display_range_limits *out)
{
	uint8_t offset_flags, vert_offset_flags, horiz_offset_flags;
	int max_vert_offset = 0, min_vert_offset = 0;
	int max_horiz_offset = 0, min_horiz_offset = 0;

	offset_flags = data[4];
	if (edid->revision >= 4) {
		vert_offset_flags = get_bit_range(offset_flags, 1, 0);
		horiz_offset_flags = get_bit_range(offset_flags, 3, 2);

		if (!decode_display_range_limits_offset(edid,
							vert_offset_flags,
							&max_vert_offset,
							&min_vert_offset)) {
			return false;
		}
		if (!decode_display_range_limits_offset(edid,
							horiz_offset_flags,
							&max_horiz_offset,
							&min_horiz_offset)) {
			return false;
		}

		if (edid->revision <= 4 &&
		    get_bit_range(offset_flags, 7, 4) != 0) {
			add_failure(edid, "Bits 7:4 of the range offset flags are reserved.");
		}
	} else if (offset_flags != 0) {
		add_failure(edid, "Range offset flags are unsupported in EDID 1.3.");
	}

	if (edid->revision <= 4 && (data[5] == 0 || data[6] == 0 ||
				    data[7] == 0 || data[8] == 0)) {
		add_failure(edid, "Range limits set to reserved values.");
		errno = EINVAL;
		return false;
	}

	out->min_vert_rate_hz = data[5] + min_vert_offset;
	out->max_vert_rate_hz = data[6] + max_vert_offset;
	out->min_horiz_rate_hz = (data[7] + min_horiz_offset) * 1000;
	out->max_horiz_rate_hz = (data[8] + max_horiz_offset) * 1000;

	if (out->min_vert_rate_hz > out->max_vert_rate_hz) {
		add_failure(edid, "Min vertical rate > max vertical rate.");
		errno = EINVAL;
		return false;
	}
	if (out->min_horiz_rate_hz > out->max_horiz_rate_hz) {
		add_failure(edid, "Min horizontal freq > max horizontal freq.");
		errno = EINVAL;
		return false;
	}

	out->max_pixel_clock_hz = (int32_t) data[9] * 10 * 1000 * 1000;
	if (edid->revision == 4 && out->max_pixel_clock_hz == 0) {
		add_failure(edid, "EDID 1.4 block does not set max dotclock.");
	}

	/* TODO: parse video timing support flags */

	return true;
}

static bool
parse_byte_descriptor(struct di_edid *edid,
		      const uint8_t data[static EDID_BYTE_DESCRIPTOR_SIZE])
{
	struct di_edid_display_descriptor *desc;
	uint8_t tag;
	char *newline;

	if (data[0] || data[1]) {
		if (edid->display_descriptors_len > 0) {
			/* A detailed timing descriptor is not allowed after a
			 * display descriptor per note 3 of table 3.20. */
			add_failure(edid, "Invalid detailed timing descriptor ordering.");
		}

		return parse_detailed_timing_def(edid, data);
	}

	/* TODO: check we got at least one detailed timing descriptor, per note
	 * 4 of table 3.20. */

	desc = calloc(1, sizeof(*desc));
	if (!desc) {
		return false;
	}

	tag = data[3];
	switch (tag) {
	case DI_EDID_DISPLAY_DESCRIPTOR_PRODUCT_SERIAL:
	case DI_EDID_DISPLAY_DESCRIPTOR_DATA_STRING:
	case DI_EDID_DISPLAY_DESCRIPTOR_PRODUCT_NAME:
		memcpy(desc->str, &data[5], 13);

		/* A newline (if any) indicates the end of the string. */
		newline = strchr(desc->str, '\n');
		if (newline) {
			newline[0] = '\0';
		}
		break;
	case DI_EDID_DISPLAY_DESCRIPTOR_RANGE_LIMITS:
		if (!parse_display_range_limits(edid, data, &desc->range_limits)) {
			free(desc);
			return true;
		}
		break;
	case DI_EDID_DISPLAY_DESCRIPTOR_COLOR_POINT:
	case DI_EDID_DISPLAY_DESCRIPTOR_STD_TIMING_IDS:
	case DI_EDID_DISPLAY_DESCRIPTOR_DCM_DATA:
	case DI_EDID_DISPLAY_DESCRIPTOR_CVT_TIMING_CODES:
	case DI_EDID_DISPLAY_DESCRIPTOR_ESTABLISHED_TIMINGS_III:
	case DI_EDID_DISPLAY_DESCRIPTOR_DUMMY:
		break; /* Ignore */
	default:
		free(desc);
		if (tag <= 0x0F) {
			/* Manufacturer-specific */
		} else {
			add_failure_until(edid, 4, "Unknown Type 0x%02hhx.", tag);
		}
		return true;
	}

	desc->tag = tag;
	edid->display_descriptors[edid->display_descriptors_len++] = desc;
	return true;
}

static bool
parse_ext(struct di_edid *edid, const uint8_t data[static EDID_BLOCK_SIZE])
{
	struct di_edid_ext *ext;
	uint8_t tag;

	if (!validate_block_checksum(data)) {
		errno = EINVAL;
		return false;
	}

	tag = data[0x00];
	switch (tag) {
	case DI_EDID_EXT_CEA:
	case DI_EDID_EXT_VTB:
	case DI_EDID_EXT_DI:
	case DI_EDID_EXT_LS:
	case DI_EDID_EXT_DPVL:
	case DI_EDID_EXT_BLOCK_MAP:
	case DI_EDID_EXT_VENDOR:
		/* Supported */
		break;
	default:
		/* Unsupported */
		add_failure_until(edid, 4, "Unknown Extension Block.");
		return true;
	}

	ext = calloc(1, sizeof(*ext));
	if (!ext) {
		return false;
	}
	ext->tag = tag;
	edid->exts[edid->exts_len++] = ext;

	return true;
}

struct di_edid *
_di_edid_parse(const void *data, size_t size)
{
	struct di_edid *edid;
	int version, revision;
	size_t exts_len, i;
	const uint8_t *standard_timing_data, *byte_desc_data, *ext_data;

	if (size < EDID_BLOCK_SIZE ||
	    size > EDID_MAX_BLOCK_COUNT * EDID_BLOCK_SIZE ||
	    size % EDID_BLOCK_SIZE != 0) {
		errno = EINVAL;
		return NULL;
	}

	if (memcmp(data, header, sizeof(header)) != 0) {
		errno = EINVAL;
		return NULL;
	}

	parse_version_revision(data, &version, &revision);
	if (version != 1) {
		/* Only EDID version 1 is supported -- as per section 2.1.7
		 * subsequent versions break the structure */
		errno = ENOTSUP;
		return NULL;
	}

	if (!validate_block_checksum(data)) {
		errno = EINVAL;
		return NULL;
	}

	exts_len = size / EDID_BLOCK_SIZE - 1;
	if (exts_len != parse_ext_count(data)) {
		errno = -EINVAL;
		return NULL;
	}

	edid = calloc(1, sizeof(*edid));
	if (!edid) {
		return NULL;
	}

	edid->version = version;
	edid->revision = revision;

	parse_vendor_product(data, &edid->vendor_product);

	if (!parse_basic_params_features(edid, data)) {
		_di_edid_destroy(edid);
		return NULL;
	}

	if (!parse_chromaticity_coords(edid, data)) {
		_di_edid_destroy(edid);
		return NULL;
	}

	for (i = 0; i < EDID_MAX_STANDARD_TIMING_COUNT; i++) {
		standard_timing_data = (const uint8_t *) data
				       + 0x26 + i * EDID_STANDARD_TIMING_SIZE;
		if (!parse_standard_timing(edid, standard_timing_data)
		    && errno != ENOTSUP) {
			_di_edid_destroy(edid);
			return NULL;
		}
	}

	for (i = 0; i < EDID_BYTE_DESCRIPTOR_COUNT; i++) {
		byte_desc_data = (const uint8_t *) data
			       + 0x36 + i * EDID_BYTE_DESCRIPTOR_SIZE;
		if (!parse_byte_descriptor(edid, byte_desc_data)
		    && errno != ENOTSUP) {
			_di_edid_destroy(edid);
			return NULL;
		}
	}

	for (i = 0; i < exts_len; i++) {
		ext_data = (const uint8_t *) data + (i + 1) * EDID_BLOCK_SIZE;
		if (!parse_ext(edid, ext_data)) {
			_di_edid_destroy(edid);
			return NULL;
		}
	}

	if (edid->failure_msg_file) {
		if (fflush(edid->failure_msg_file) != 0) {
			_di_edid_destroy(edid);
			return NULL;
		}

		fclose(edid->failure_msg_file);
		edid->failure_msg_file = NULL;
	}

	return edid;
}

void
_di_edid_destroy(struct di_edid *edid)
{
	size_t i;

	if (edid->failure_msg_file) {
		fclose(edid->failure_msg_file);
	}
	free(edid->failure_msg);

	for (i = 0; i < edid->standard_timings_len; i++) {
		free(edid->standard_timings[i]);
	}

	for (i = 0; i < edid->detailed_timing_defs_len; i++) {
		free(edid->detailed_timing_defs[i]);
	}

	for (i = 0; i < edid->display_descriptors_len; i++) {
		free(edid->display_descriptors[i]);
	}

	for (i = 0; edid->exts[i] != NULL; i++) {
		free(edid->exts[i]);
	}

	free(edid);
}

int
di_edid_get_version(const struct di_edid *edid)
{
	return edid->version;
}

int
di_edid_get_revision(const struct di_edid *edid)
{
	return edid->revision;
}

const struct di_edid_vendor_product *
di_edid_get_vendor_product(const struct di_edid *edid)
{
	return &edid->vendor_product;
}

const struct di_edid_video_input_digital *
di_edid_get_video_input_digital(const struct di_edid *edid)
{
	return edid->is_digital ? &edid->video_input_digital : NULL;
}

const struct di_edid_screen_size *
di_edid_get_screen_size(const struct di_edid *edid)
{
	return &edid->screen_size;
}

float
di_edid_get_basic_gamma(const struct di_edid *edid)
{
	return edid->gamma;
}

const struct di_edid_dpms *
di_edid_get_dpms(const struct di_edid *edid)
{
	return &edid->dpms;
}

enum di_edid_display_color_type
di_edid_get_display_color_type(const struct di_edid *edid)
{
	return edid->display_color_type;
}

const struct di_edid_color_encoding_formats *
di_edid_get_color_encoding_formats(const struct di_edid *edid)
{
	/* If color encoding formats are specified, RGB 4:4:4 is always
	 * supported. */
	return edid->color_encoding_formats.rgb444 ? &edid->color_encoding_formats : NULL;
}

const struct di_edid_misc_features *
di_edid_get_misc_features(const struct di_edid *edid)
{
	return &edid->misc_features;
}

const struct di_edid_chromaticity_coords *
di_edid_get_chromaticity_coords(const struct di_edid *edid)
{
	return &edid->chromaticity_coords;
}

int32_t
di_edid_standard_timing_get_vert_video(const struct di_edid_standard_timing *t)
{
	switch (t->aspect_ratio) {
	case DI_EDID_STANDARD_TIMING_16_10:
		return t->horiz_video * 10 / 16;
	case DI_EDID_STANDARD_TIMING_4_3:
		return t->horiz_video * 3 / 4;
	case DI_EDID_STANDARD_TIMING_5_4:
		return t->horiz_video * 4 / 5;
	case DI_EDID_STANDARD_TIMING_16_9:
		return t->horiz_video * 9 / 16;
	}
	abort(); /* unreachable */
}

const struct di_edid_standard_timing *const *
di_edid_get_standard_timings(const struct di_edid *edid)
{
	return (const struct di_edid_standard_timing *const *) &edid->standard_timings;
}

const struct di_edid_detailed_timing_def *const *
di_edid_get_detailed_timing_defs(const struct di_edid *edid)
{
	return (const struct di_edid_detailed_timing_def *const *) &edid->detailed_timing_defs;
}

const struct di_edid_display_descriptor *const *
di_edid_get_display_descriptors(const struct di_edid *edid)
{
	return (const struct di_edid_display_descriptor *const *) &edid->display_descriptors;
}

enum di_edid_display_descriptor_tag
di_edid_display_descriptor_get_tag(const struct di_edid_display_descriptor *desc)
{
	return desc->tag;
}

const char *
di_edid_display_descriptor_get_string(const struct di_edid_display_descriptor *desc)
{
	switch (desc->tag) {
	case DI_EDID_DISPLAY_DESCRIPTOR_PRODUCT_SERIAL:
	case DI_EDID_DISPLAY_DESCRIPTOR_DATA_STRING:
	case DI_EDID_DISPLAY_DESCRIPTOR_PRODUCT_NAME:
		return desc->str;
	default:
		return NULL;
	}
}

const struct di_edid_display_range_limits *
di_edid_display_descriptor_get_range_limits(const struct di_edid_display_descriptor *desc)
{
	if (desc->tag != DI_EDID_DISPLAY_DESCRIPTOR_RANGE_LIMITS) {
		return NULL;
	}
	return &desc->range_limits;
}

const struct di_edid_ext *const *
di_edid_get_extensions(const struct di_edid *edid)
{
	return (const struct di_edid_ext *const *) edid->exts;
}

enum di_edid_ext_tag
di_edid_ext_get_tag(const struct di_edid_ext *ext)
{
	return ext->tag;
}
