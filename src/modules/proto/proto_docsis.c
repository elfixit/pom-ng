/*
 *  This file is part of pom-ng.
 *  Copyright (C) 2011-2012 Guy Martin <gmsoft@tuxicoman.be>
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

#include <pom-ng/ptype.h>
#include <pom-ng/proto.h>
#include <pom-ng/ptype_bool.h>
#include <pom-ng/ptype_uint8.h>

#include <string.h>
#include <stddef.h>
#include <arpa/inet.h>

#include "proto_docsis.h"

static struct proto *proto_ethernet = NULL;

struct mod_reg_info* proto_docsis_reg_info() {

	static struct mod_reg_info reg_info = { 0 };
	reg_info.api_ver = MOD_API_VER;
	reg_info.register_func = proto_docsis_mod_register;
	reg_info.unregister_func = proto_docsis_mod_unregister;
	reg_info.dependencies = "proto_ethernet, ptype_bool, ptype_uint8";

	return &reg_info;
}


static int proto_docsis_mod_register(struct mod_reg *mod) {

	static struct proto_pkt_field fields[PROTO_DOCSIS_FIELD_NUM + 1] = { { 0 } };
	fields[0].name = "fc_type";
	fields[0].value_type = ptype_get_type("uint8");
	fields[0].description = "Frame control type";
	fields[1].name = "fc_parm";
	fields[1].value_type = ptype_get_type("uint8");
	fields[1].description = "Frame parameters";
	fields[2].name = "ehdr_on";
	fields[2].value_type = ptype_get_type("bool");
	fields[2].description = "Extended header present";

	static struct proto_reg_info proto_docsis = { 0 };
	proto_docsis.name = "docsis";
	proto_docsis.api_ver = PROTO_API_VER;
	proto_docsis.mod = mod;
	proto_docsis.pkt_fields = fields;

	// No contrack here

	proto_docsis.init = proto_docsis_init;
	proto_docsis.process = proto_docsis_process;

	return proto_register(&proto_docsis);
}


static int proto_docsis_init(struct proto *proto, struct registry_instance *i) {

	proto_ethernet = proto_get("ethernet");

	if (!proto_ethernet)
		return POM_ERR;

	return POM_OK;

}

static int proto_docsis_process(struct proto *proto, struct packet *p, struct proto_process_stack *stack, unsigned int stack_index) {

	struct proto_process_stack *s = &stack[stack_index];
	struct proto_process_stack *s_next = &stack[stack_index + 1];
	struct docsis_hdr *dhdr = s->pload;

	if (s->plen < sizeof(struct docsis_hdr) || ntohs(dhdr->len) > s->plen)
		return PROTO_INVALID;

	uint16_t hdr_len = sizeof(struct docsis_hdr);


	PTYPE_UINT8_SETVAL(s->pkt_info->fields_value[proto_docsis_field_fc_type], dhdr->fc_type);
	PTYPE_UINT8_SETVAL(s->pkt_info->fields_value[proto_docsis_field_fc_parm], dhdr->fc_parm);
	PTYPE_BOOL_SETVAL(s->pkt_info->fields_value[proto_docsis_field_ehdr_on], dhdr->ehdr_on);

	if (dhdr->ehdr_on) {
		
		if (dhdr->mac_parm > ntohs(dhdr->len))
			return PROTO_INVALID;

		hdr_len += dhdr->mac_parm;

		// Don't process crypted packets any further
		struct docsis_ehdr *ehdr = (struct docsis_ehdr*) (dhdr + offsetof(struct docsis_hdr, hcs));
		if (ehdr->eh_type == EH_TYPE_BP_DOWN || ehdr->eh_type == EH_TYPE_BP_DOWN)
			return PROTO_OK;
			
	}

	s_next->pload = s->pload + hdr_len;
	s_next->plen = s->plen - hdr_len;

	switch (dhdr->fc_type) {
		case FC_TYPE_PKT_MAC:
		case FC_TYPE_ISOLATION_PKT_MAC:
			// We don't need the 4 bytes of ethernet checksum
			s_next->plen -= 4;
			s_next->proto = proto_ethernet;
			break;
/*		case FC_TYPE_MAC_SPC:
			if (dhdr->fc_parm == FCP_MGMT) {
				s_next->proto = priv->proto_docsis_mgmt;
				break;
			}
			break;
*/
		default:
			s_next->proto = NULL;
			break;

	}

	return PROTO_OK;

}

static int proto_docsis_mod_unregister() {

	return proto_unregister("docsis");
}
