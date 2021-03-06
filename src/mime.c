/*
 *  This file is part of pom-ng.
 *  Copyright (C) 2013-2014 Guy Martin <gmsoft@tuxicoman.be>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include "common.h"
#include <pom-ng/mime.h>
#include <pom-ng/ptype_string.h>
#include <pom-ng/decoder.h>

#include "analyzer.h"

struct mime_top_type_str {
	enum mime_top_type top_type;
	char *str;
};

static struct mime_top_type_str mime_top_types_str[] = {
	{ mime_top_type_binary, "binary" },
	{ mime_top_type_text, "text" },
	{ mime_top_type_image, "image" },
	{ mime_top_type_audio, "audio" },
	{ mime_top_type_video, "video" },
	{ mime_top_type_application, "application" },
	{ mime_top_type_multipart, "multipart" },
	{ mime_top_type_message, "message" },
	{ mime_top_type_unknown, NULL },
};

struct mime_type *mime_type_parse(char *content_type) {

	if (!content_type)
		return NULL;

	while (*content_type == ' ')
		content_type++;


	struct mime_type *mime_type = malloc(sizeof(struct mime_type));
	if (!mime_type) {
		pom_oom(sizeof(struct mime_type));
		return NULL;
	}
	memset(mime_type, 0, sizeof(struct mime_type));

	// First, copy the filtered content_type
	
	char *sc = strchr(content_type, ';');

	size_t type_len;
	if (sc)
		type_len = sc - content_type;
	else
		type_len = strlen(content_type);

	while (type_len > 0 && content_type[type_len - 1] == ' ')
		type_len--;

	mime_type->name = strndup(content_type, type_len);
	if (!mime_type->name) {
		pom_oom(type_len);
		free(mime_type);
		return NULL;
	}

	// Lowercase the name
	int i;
	for (i = 0; i < type_len; i++) {
		if (mime_type->name[i] >= 'A' && mime_type->name[i] <= 'Z')
			mime_type->name[i] += 'a' - 'A';
	}
	

	// Find the top type
	int found = 0;
	for (i = 0; mime_top_types_str[i].str; i++) {
		if (!strncmp(mime_top_types_str[i].str, mime_type->name, strlen(mime_top_types_str[i].str))) {
			mime_type->top_type = mime_top_types_str[i].top_type;
			found = 1;
			break;
		}
	}

	if (!found) {
		mime_type->top_type = mime_top_type_unknown;
		pomlog(POMLOG_DEBUG "Top type of '%s' now known", mime_type->name);
	}

	if (!sc) // No parameters
		return mime_type;

	// Parse parameters
	char *p = sc + 1;
	unsigned int param_num;
	for (param_num = 0; p && param_num < MIME_MAX_PARAMETERS; param_num++) {

		// Trim left
		while (*p == ' ')
			p++;

		char *eq = strchr(p, '=');
		if (!eq) {
			// Parameter without value, abort parsing
			return mime_type;
		}

		char *param_name = strndup(p, eq - p);
		if (!param_name) {
			mime_type_cleanup(mime_type);
			pom_oom(eq - p);
			return NULL;
		}
		
		// Parse the value
		char *pv = eq + 1;
		char pv_end = ' ';
		if (*pv == '"') { // The parameters is enclosed between ""
			pv_end = '"';
			pv++;
		}

		char *param_value = NULL;
		char *next_p = strchr(pv, pv_end);
		if (!next_p) { // Last parameters
			param_value = strdup(pv);
		} else {
			param_value = strndup(pv, next_p - pv);
		}

		if (!param_value) {
			mime_type_cleanup(mime_type);
			pom_oom(strlen(pv));
			return NULL;
		}

		mime_type->params[param_num].name = param_name;
		mime_type->params[param_num].value = param_value;

		p = next_p;
	}
	

	return mime_type;
}

void mime_type_cleanup(struct mime_type *mime_type) {

	if (mime_type->name)
		free(mime_type->name);
	int i;
	for (i = 0; i < MIME_MAX_PARAMETERS && mime_type->params[i].name; i++) {
		if (mime_type->params[i].value)
			free(mime_type->params[i].value);
		free(mime_type->params[i].name);
	}

	free(mime_type);
}

char *mime_type_get_param(struct mime_type *mime_type, char *param_name) {

	int i;
	for (i = 0; i < MIME_MAX_PARAMETERS && mime_type->params[i].name; i++) {
		if (!strcmp(mime_type->params[i].name, param_name))
			return mime_type->params[i].value;
	}
	return NULL;
}


static int mime_header_parse_value_token(char *buff, size_t in_len, size_t *out_len) {
	
	// Parse =?charset?encoding?encoded_text?= where encoding is either B or Q
	// See RFC 2047 for details

	if (in_len < 9) // strlen("=?c?e?t?=")
		return POM_ERR;

	// We overwrite the original content
	char *out = buff;

	// Check for begining =? and ending ?=
	if (buff[0] != '=' || buff[1] != '?' || buff[in_len - 2] != '?' || buff[in_len - 1] != '=')
		return POM_ERR;
	
	buff += 2;
	in_len -= 4;

	char *charset = buff;

	// Find the encoding
	char *eq = memchr(buff, '?', in_len);
	if (!eq)
		return POM_ERR;

	size_t charset_len = eq - charset;

	in_len -= charset_len;

	if (in_len < 4) // strlen("?e?t");
		return POM_ERR;
	
	eq++;
	in_len--;

	char *qb = eq;

	if (*qb != 'q' && *qb != 'Q' && *qb != 'b' && *qb != 'B')
		return POM_ERR;

	eq++;
	in_len--;

	if (*eq != '?')
		return POM_ERR;
	
	eq++;
	in_len--;

	char *content = eq;

	char *encoding = "base64";
	if (*qb == 'q' || *qb == 'Q') {
		encoding = "quoted-printable";

		// Change _ into ' '
		// See RFC 2047 4.2 (2)
		char *u = NULL;
		while ((u = memchr(content, '_', in_len)))
			*u = ' ';
	}

	struct decoder *dec = decoder_alloc(encoding);
	if (!dec)
		return POM_ERR;


	dec->avail_out = in_len;
	dec->next_out = out;
	dec->avail_in = in_len;
	dec->next_in = content;

	decoder_decode(dec);

	*out_len = (in_len - dec->avail_out);

	decoder_cleanup(dec);

	return POM_OK;
}

static char *mime_header_parse_value(char *data, size_t len) {
	
	if (!data)
		return NULL;

	// The value will always be smaller or equal in length to the input
	char *value = strndup(data, len);
	if (!value) {
		pom_oom(len);
		return NULL;
	}

	char *output = value;
	char *input = value;

	while (len > 1) {
		char *eq = memchr(input, '=', len);
		if (!eq)
			break;
	
		len -= eq - input;
		size_t in_len = len;
		output = eq;
		input = eq;
		char *end = memchr(eq, ' ', in_len);
		if (end)
			in_len = end - eq;
		
		size_t out_len = 0;
		if (mime_header_parse_value_token(eq, in_len, &out_len) == POM_OK) {
			output += out_len;
		} else {
			output += in_len;
		}

		input += in_len;
		len -= in_len;

	}

	return value;

}

int mime_header_parse(struct data *data, char *line, size_t line_len) {

	if (!line_len)
		return POM_OK;

	if (*line == ' ' || *line == '\t') {
		while (line_len > 0 && (*line == ' ' || *line == '\t')) {
			line_len--;
			line++;
		}

		if (!line_len)
			return POM_OK;

		// Find the last item we added and append the string
		struct data_item *itm = data->items;
		if (!itm) {
			pomlog(POMLOG_DEBUG "Last header not found, cannot append value");
			return POM_ERR;
		}

		// The last one is pushed at the top of the stack by data_item_add_ptype

		char *last_hdr_value = PTYPE_STRING_GETVAL(itm->value);
		if (!last_hdr_value)
			return POM_ERR;


		size_t new_len = strlen(last_hdr_value) + line_len + 2;
		char *new_value = malloc(new_len);
		if (!new_value) {
			pom_oom(new_len);
			return POM_ERR;
		}

		char *param_tail = mime_header_parse_value(line, line_len);
		if (!param_tail) {
			free(new_value);
			return POM_ERR;
		}

		strcpy(new_value, last_hdr_value);
		strcat(new_value, " ");
		strcat(new_value, param_tail);
		new_value[new_len - 1] = 0;
		free(param_tail);

		PTYPE_STRING_SETVAL_P(itm->value, new_value);

	} else {
		char *colon = memchr(line, ':', line_len);
		if (!colon) {
			pomlog(POMLOG_DEBUG "No colon in header line, invalid content");
			return POM_ERR;
		}

		char *hdr_name = strndup(line, colon - line);
		if (!hdr_name) {
			pom_oom(colon - line);
			return POM_ERR;
		}

		colon++;
		
		size_t value_len = line_len - (colon - line);
		while (value_len > 0 && (*colon == ' ' || *colon == '\t')) {
			colon++;
			value_len--;
		}

		char *hdr_value = mime_header_parse_value(colon, value_len);
		if (!hdr_value) {
			free(hdr_name);
			return POM_ERR;
		}

		struct ptype *hdr_value_pt = ptype_alloc("string");
		if (!hdr_value_pt) {
			free(hdr_name);
			free(hdr_value);
			return POM_ERR;
		}

		PTYPE_STRING_SETVAL_P(hdr_value_pt, hdr_value);

		if (data_item_add_ptype(data, 0, hdr_name, hdr_value_pt) != POM_OK) {
			free(hdr_name);
			ptype_cleanup(hdr_value_pt);
			return POM_ERR;

		}

	}

	return POM_OK;
}


