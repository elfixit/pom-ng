/*
 *  This file is part of pom-ng.
 *  Copyright (C) 2014 Guy Martin <gmsoft@tuxicoman.be>
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

#ifndef __OUTPUT_INJECT_H__
#define __OUTPUT_INJECT_H__


#include <pom-ng/output.h>
#include <pom-ng/proto.h>
#include <pom-ng/filter.h>

#include <pcap.h>


struct output_inject_priv {

	pcap_t *p;
	struct filter_node *filter;

	struct proto_packet_listener *listener;

	struct ptype *p_interface;
	struct ptype *p_filter;

	struct registry_perf *perf_pkts_out;
	struct registry_perf *perf_bytes_out;

};

struct mod_reg_info *output_inject_reg_info();
static int output_inject_mod_register(struct mod_reg *mod);
static int output_inject_mod_unregister();

static int output_inject_init(struct output *o);
static int output_inject_cleanup(void *output_priv);
static int output_inject_open(void *output_priv);
static int output_inject_close(void *output_priv);
static int output_inject_process(void *obj, struct packet *p, struct proto_process_stack *s, unsigned int stack_index);
static int output_inject_filter_parse(void *priv, struct registry_param *param, char *value);
static int output_inject_filter_update(void *priv, struct registry_param *param, struct ptype *value);

#endif
