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


#include <pom-ng/conntrack.h>
#include <pom-ng/ptype.h>
#include <pom-ng/ptype_uint8.h>
#include <pom-ng/ptype_uint16.h>


#include "proto_mpeg.h"
#include "proto_mpeg_ts.h"
#include "proto_mpeg_sect.h"
#include "proto_mpeg_dvb_mpe.h"

struct mod_reg_info* proto_mpeg_reg_info() {

	static struct mod_reg_info reg_info = { 0 };
	reg_info.api_ver = MOD_API_VER;
	reg_info.register_func = proto_mpeg_mod_register;
	reg_info.unregister_func = proto_mpeg_mod_unregister;
	reg_info.dependencies = "proto_ipv4, proto_docsis, ptype_mac, ptype_uint8, ptype_uint16";

	return &reg_info;
}

static int proto_mpeg_mod_register(struct mod_reg *mod) {

	static struct proto_pkt_field proto_mpeg_dvb_mpe_fields[PROTO_MPEG_DVB_MPE_FIELD_NUM + 1] = { { 0 } };
	proto_mpeg_dvb_mpe_fields[0].name = "dst";
	proto_mpeg_dvb_mpe_fields[0].value_type = ptype_get_type("mac");
	proto_mpeg_dvb_mpe_fields[0].description = "Destination MAC address";

	static struct proto_reg_info proto_mpeg_dvb_mpe = { 0 };
	proto_mpeg_dvb_mpe.name = "mpeg_dvb_mpe";
	proto_mpeg_dvb_mpe.api_ver = PROTO_API_VER;
	proto_mpeg_dvb_mpe.mod = mod;
	proto_mpeg_dvb_mpe.pkt_fields = proto_mpeg_dvb_mpe_fields;

	proto_mpeg_dvb_mpe.init = proto_mpeg_dvb_mpe_init;
	proto_mpeg_dvb_mpe.process = proto_mpeg_dvb_mpe_process;

	if (proto_register(&proto_mpeg_dvb_mpe) != POM_OK) 
		return POM_ERR;

	static struct proto_pkt_field proto_mpeg_sect_fields[PROTO_MPEG_SECT_FIELD_NUM + 1] = { { 0 } };
	proto_mpeg_sect_fields[0].name = "table_id";
	proto_mpeg_sect_fields[0].value_type = ptype_get_type("uint8");
	proto_mpeg_sect_fields[0].description = "Table ID";

	static struct proto_reg_info proto_mpeg_sect = { 0 };
	proto_mpeg_sect.name = "mpeg_sect";
	proto_mpeg_sect.api_ver = PROTO_API_VER;
	proto_mpeg_sect.mod = mod;
	proto_mpeg_sect.pkt_fields = proto_mpeg_sect_fields;

	proto_mpeg_sect.init = proto_mpeg_sect_init;
	proto_mpeg_sect.process = proto_mpeg_sect_process;

	if (proto_register(&proto_mpeg_sect) != POM_OK) {
		proto_mpeg_mod_unregister();
		return POM_ERR;
	}

	static struct proto_reg_info proto_mpeg_ts = { 0 };
	proto_mpeg_ts.name = "mpeg_ts";
	proto_mpeg_ts.api_ver = PROTO_API_VER;
	proto_mpeg_ts.mod = mod;

	static struct proto_pkt_field proto_mpeg_ts_fields[PROTO_MPEG_TS_FIELD_NUM + 1] = { { 0 } };
	proto_mpeg_ts_fields[0].name = "pid";
	proto_mpeg_ts_fields[0].value_type = ptype_get_type("uint16");
	proto_mpeg_ts_fields[0].description = "PID";
	proto_mpeg_ts.pkt_fields = proto_mpeg_ts_fields;

	static struct conntrack_info ct_info = { 0 };
	ct_info.default_table_size = 256;
	ct_info.fwd_pkt_field_id = proto_mpeg_ts_field_pid;
	ct_info.rev_pkt_field_id = CONNTRACK_PKT_FIELD_NONE;
	ct_info.cleanup_handler = proto_mpeg_ts_conntrack_cleanup;
	proto_mpeg_ts.ct_info = &ct_info;

	proto_mpeg_ts.init = proto_mpeg_ts_init;
	proto_mpeg_ts.process = proto_mpeg_ts_process;
	proto_mpeg_ts.cleanup = proto_mpeg_ts_cleanup;

	if (proto_register(&proto_mpeg_ts) != POM_OK) {
		proto_mpeg_mod_unregister();
		return POM_ERR;
	}

	return POM_OK;

}


static int proto_mpeg_mod_unregister() {

	int res = POM_OK;
	
	res += proto_unregister("mpeg_ts");
	res += proto_unregister("mpeg_sect");
	res += proto_unregister("mpeg_dvb_mpe");

	return res;
}
