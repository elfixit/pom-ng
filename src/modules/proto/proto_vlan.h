/*
 *  This file is part of pom-ng.
 *  Copyright (C) 2010-2012 Guy Martin <gmsoft@tuxicoman.be>
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

#ifndef __PROTO_VLAN_H__
#define __PROTO_VLAN_H__

struct vlan_header {

#if __BYTE_ORDER == __LITTLE_ENDIAN
	uint16_t vid:12;
	uint16_t de:1;
	uint16_t pcp:3;
#elif __BYTE_ORDER == __BIG_ENDIAN
	uint16_t user_priority:3;
	uint16_t de:1;
	uint16_t pcp:12;
#else
# error "Please fix <endian.h>"
#endif

	uint16_t ether_type;
};

#define PROTO_VLAN_FIELD_NUM 3

enum proto_ethernet_fields {
	proto_vlan_field_vid = 0,
	proto_vlan_field_de,
	proto_vlan_field_pcp,
};

struct mod_reg_info* proto_vlan_reg_info();
static int proto_vlan_init(struct proto *proto, struct registry_instance *i);
static int proto_vlan_mod_register(struct mod_reg *mod);
static int proto_vlan_process(struct proto *proto, struct packet *p, struct proto_process_stack *stack, unsigned int stack_index);
static int proto_vlan_mod_unregister();

#endif