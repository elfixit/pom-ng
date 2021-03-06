/*
 *  This file is part of pom-ng.
 *  Copyright (C) 2011-2014 Guy Martin <gmsoft@tuxicoman.be>
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

#ifndef __OUTPUT_PCAP_H__
#define __OUTPUT_PCAP_H__


#include <pom-ng/output.h>
#include <pom-ng/proto.h>
#include <pom-ng/filter.h>
#include <pom-ng/event.h>

#include <pcap.h>

#define OUTPUT_PCAP_FLOW_FILE_DATA_COUNT 5

struct output_pcap_file_priv {

	pcap_dumper_t *pdump;
	pcap_t *p;
	struct filter_node *filter;

	struct proto_packet_listener *listener;

	struct ptype *p_filename;
	struct ptype *p_snaplen;
	struct ptype *p_link_type;
	struct ptype *p_unbuffered;
	struct ptype *p_filter;

	pthread_mutex_t lock;

	struct registry_perf *perf_pkts_out;
	struct registry_perf *perf_bytes_out;

};

struct output_pcap_flow_priv {

	struct ptype *p_link_type;
	struct ptype *p_flow_proto;
	struct ptype *p_snaplen;
	struct ptype *p_unbuffered;
	struct ptype *p_prefix;

	struct proto *proto;
	struct proto_packet_listener *listener;
	int link_type;

	struct registry_perf *perf_pkts_out;
	struct registry_perf *perf_bytes_out;
	struct registry_perf *perf_flows_cur;
	struct registry_perf *perf_flows_tot;

	char *output_name;

	pthread_mutex_t lock;

	struct output_pcap_flow_ce_priv *flows;

};

struct output_pcap_flow_ce_priv {

	pcap_dumper_t *pdump;
	pcap_t *p;
	char *filename;
	struct conntrack_entry *ce;

	pthread_mutex_t lock;

	struct event *evt;

	struct output_pcap_flow_ce_priv *prev, *next;

};

enum {
	output_pcap_flow_file_output = 0,
	output_pcap_flow_file_filename,
	output_pcap_flow_file_bytes,
	output_pcap_flow_file_packets,
	output_pcap_flow_file_info,
};

struct output_pcap_link_type {
	char *name;
	int dlt;
};

struct mod_reg_info *output_pcap_reg_info();
static int output_pcap_mod_register(struct mod_reg *mod);
static int output_pcap_mod_unregister();

static int output_pcap_linktype_to_dlt(char *link_type);

static int output_pcap_file_init(struct output *o);
static int output_pcap_file_cleanup(void *output_priv);
static int output_pcap_file_open(void *output_priv);
static int output_pcap_file_close(void *output_priv);
static int output_pcap_file_process(void *obj, struct packet *p, struct proto_process_stack *s, unsigned int stack_index);
static int output_pcap_filter_parse(void *priv, struct registry_param *param, char *value);
static int output_pcap_filter_update(void *priv, struct registry_param *param, struct ptype *value);

static int output_pcap_flow_register();
static int output_pcap_flow_unregister();
static int output_pcap_flow_init(struct output *o);
static int output_pcap_flow_cleanup(void *output_priv);
static int output_pcap_flow_process(void *obj, struct packet *p, struct proto_process_stack *s, unsigned int stack_index);
static int output_pcap_flow_ce_cleanup(void *obj, void *priv);
static int output_pcap_flow_open(void *output_priv);
static int output_pcap_flow_close(void *output_priv);
static int output_pcap_flow_parse_filename(struct proto_process_stack *s, struct packet *p, char *format, char *filename, size_t filename_len);

#endif
