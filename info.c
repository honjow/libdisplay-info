#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edid.h"
#include "info.h"
#include "memory-stream.h"

/* Generated file pnp-id-table.c: */
const char *
pnp_id_table(const char *key);

static void
derive_edid_color_primaries(const struct di_edid *edid,
			    struct di_color_primaries *cc)
{
	const struct di_edid_chromaticity_coords *cm;
	const struct di_edid_misc_features *misc;

	/* Trust the flag more than the fields. */
	misc = di_edid_get_misc_features(edid);
	if (misc->srgb_is_primary) {
		/*
		 * https://www.w3.org/Graphics/Color/sRGB.html
		 * for lack of access to IEC 61966-2-1
		 */
		cc->primary[0].x = 0.640f; /* red */
		cc->primary[0].y = 0.330f;
		cc->primary[1].x = 0.300f; /* green */
		cc->primary[1].y = 0.600f;
		cc->primary[2].x = 0.150f; /* blue */
		cc->primary[2].y = 0.060f;
		cc->has_primaries = true;
		cc->default_white.x = 0.3127f; /* D65 */
		cc->default_white.y = 0.3290f;
		cc->has_default_white_point = true;

		return;
	}

	cm = di_edid_get_chromaticity_coords(edid);

	/*
	 * Broken EDID might have only partial values.
	 * Require all values to report anything.
	 */
	if (cm->red_x > 0.0f &&
	    cm->red_y > 0.0f &&
	    cm->green_x > 0.0f &&
	    cm->green_y > 0.0f &&
	    cm->blue_x > 0.0f &&
	    cm->blue_y > 0.0f) {
		cc->primary[0].x = cm->red_x;
		cc->primary[0].y = cm->red_y;
		cc->primary[1].x = cm->green_x;
		cc->primary[1].y = cm->green_y;
		cc->primary[2].x = cm->blue_x;
		cc->primary[2].y = cm->blue_y;
		cc->has_primaries = true;
	}
	if (cm->white_x > 0.0f && cm->white_y > 0.0f) {
		cc->default_white.x = cm->white_x;
		cc->default_white.y = cm->white_y;
		cc->has_default_white_point = true;
	}
}

struct di_info *
di_info_parse_edid(const void *data, size_t size)
{
	struct memory_stream failure_msg;
	struct di_edid *edid;
	struct di_info *info;
	char *failure_msg_str = NULL;

	if (!memory_stream_open(&failure_msg))
		return NULL;

	edid = _di_edid_parse(data, size, failure_msg.fp);
	if (!edid)
		goto err_failure_msg_file;

	info = calloc(1, sizeof(*info));
	if (!info)
		goto err_edid;

	info->edid = edid;

	failure_msg_str = memory_stream_close(&failure_msg);
	if (failure_msg_str && failure_msg_str[0] != '\0')
		info->failure_msg = failure_msg_str;
	else
		free(failure_msg_str);

	derive_edid_color_primaries(info->edid, &info->derived.color_primaries);

	return info;

err_edid:
	_di_edid_destroy(edid);
err_failure_msg_file:
	memory_stream_cleanup(&failure_msg);
	return NULL;
}

void
di_info_destroy(struct di_info *info)
{
	_di_edid_destroy(info->edid);
	free(info->failure_msg);
	free(info);
}

const struct di_edid *
di_info_get_edid(const struct di_info *info)
{
	return info->edid;
}

const char *
di_info_get_failure_msg(const struct di_info *info)
{
	return info->failure_msg;
}

static void
encode_ascii_byte(FILE *out, char ch)
{
	uint8_t c = (uint8_t)ch;

	/*
	 * Replace ASCII control codes and non-7-bit codes
	 * with an escape string. The result is guaranteed to be valid
	 * UTF-8.
	 */
	if (c < 0x20 || c >= 0x7f)
		fprintf(out, "\\x%02x", c);
	else
		fputc(c, out);
}

static void
encode_ascii_string(FILE *out, const char *str)
{
	size_t len = strlen(str);
	size_t i;

	for (i = 0; i < len; i++)
		encode_ascii_byte(out, str[i]);
}

char *
di_info_get_make(const struct di_info *info)
{
	const struct di_edid_vendor_product *evp;
	char pnp_id[(sizeof(evp->manufacturer)) + 1] = { 0, };
	const char *manuf;
	struct memory_stream m;

	if (!info->edid)
		return NULL;

	if (!memory_stream_open(&m))
		return NULL;

	evp = di_edid_get_vendor_product(info->edid);
	memcpy(pnp_id, evp->manufacturer, sizeof(evp->manufacturer));

	manuf = pnp_id_table(pnp_id);
	if (manuf) {
		encode_ascii_string(m.fp, manuf);
		return memory_stream_close(&m);
	}

	fputs("PNP(", m.fp);
	encode_ascii_string(m.fp, pnp_id);
	fputs(")", m.fp);

	return memory_stream_close(&m);
}

char *
di_info_get_model(const struct di_info *info)
{
	const struct di_edid_vendor_product *evp;
	const struct di_edid_display_descriptor *const *desc;
	struct memory_stream m;
	size_t i;
	enum di_edid_display_descriptor_tag tag;
	const char *str;

	if (!info->edid)
		return NULL;

	if (!memory_stream_open(&m))
		return NULL;

	desc = di_edid_get_display_descriptors(info->edid);
	for (i = 0; desc[i]; i++) {
		tag = di_edid_display_descriptor_get_tag(desc[i]);
		if (tag != DI_EDID_DISPLAY_DESCRIPTOR_PRODUCT_NAME)
			continue;
		str = di_edid_display_descriptor_get_string(desc[i]);
		if (str[0] == '\0')
			continue;
		encode_ascii_string(m.fp, str);
		return memory_stream_close(&m);
	}

	evp = di_edid_get_vendor_product(info->edid);
	fprintf(m.fp, "0x%04" PRIX16, evp->product);

	return memory_stream_close(&m);
}

char *
di_info_get_serial(const struct di_info *info)
{
	const struct di_edid_display_descriptor *const *desc;
	const struct di_edid_vendor_product *evp;
	struct memory_stream m;
	size_t i;
	enum di_edid_display_descriptor_tag tag;
	const char *str;

	if (!info->edid)
		return NULL;

	if (!memory_stream_open(&m))
		return NULL;

	desc = di_edid_get_display_descriptors(info->edid);
	for (i = 0; desc[i]; i++) {
		tag = di_edid_display_descriptor_get_tag(desc[i]);
		if (tag != DI_EDID_DISPLAY_DESCRIPTOR_PRODUCT_SERIAL)
			continue;
		str = di_edid_display_descriptor_get_string(desc[i]);
		if (str[0] == '\0')
			continue;
		encode_ascii_string(m.fp, str);
		return memory_stream_close(&m);
	}

	evp = di_edid_get_vendor_product(info->edid);
	if (evp->serial != 0) {
		fprintf(m.fp, "0x%08" PRIX32, evp->serial);
		return memory_stream_close(&m);
	}

	memory_stream_cleanup(&m);
	return NULL;
}

const struct di_color_primaries *
di_info_get_default_color_primaries(const struct di_info *info)
{
	return &info->derived.color_primaries;
}
