/*
 *  This file is part of pom-ng.
 *  Copyright (C) 2012-2014 Guy Martin <gmsoft@tuxicoman.be>
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


#ifndef __FILTER_H__
#define __FILTER_H__

#include <pom-ng/ptype.h>

#define FILTER_OP_NOP	PTYPE_OP_RSVD
#define FILTER_OP_EQ	PTYPE_OP_EQ
#define FILTER_OP_GT	PTYPE_OP_GT
#define FILTER_OP_GE	PTYPE_OP_GE
#define FILTER_OP_LT	PTYPE_OP_LT
#define FILTER_OP_LE	PTYPE_OP_LE
#define FILTER_OP_NEQ	PTYPE_OP_NEQ

#define FILTER_OP_AND	(PTYPE_OP_ALL + 1)
#define FILTER_OP_OR	(PTYPE_OP_ALL + 2)

// Remove this one when merge complete
#define FILTER_OP_NOT	(PTYPE_OP_ALL + 3)

#include <pom-ng/proto.h>
#include <pom-ng/filter.h>
#include "pload.h"
#include "event.h"
#include "core.h"

enum filter_evt_prop {
	filter_evt_prop_time,
	filter_evt_prop_name,
	filter_evt_prop_source,
};

enum filter_value_type {
	filter_value_type_none = 0,
	filter_value_type_node,
	filter_value_type_data,
	filter_value_type_ptype,
	filter_value_type_string,
	filter_value_type_integer,
	filter_value_type_evt_prop,
	filter_value_type_pload_data,
	filter_value_type_pload_evt_data,
	filter_value_type_proto,
};

struct filter_data_raw {
	char *field_name;
	char *key;
};

struct filter_data {
	unsigned int field_id;
	char *key;
	struct ptype_reg *pt_reg;
};

struct filter_packet {
	struct proto *proto;
	int field_id;
	struct ptype_reg *pt_reg;
};

union filter_value {
	struct filter_node *node;
	struct filter_data data;
	struct filter_data_raw data_raw;
	struct filter_packet proto;
	struct ptype *ptype;
	char *string;
	uint64_t integer;
};

struct filter_node {

	int op;
	int not;

	enum filter_value_type type[2];
	union filter_value value[2];

};


// Raw filter structure used to parse strings
struct filter_raw_data {
	char *op;
	char *value[2];
};

struct filter_raw_branch {

	struct filter_raw_node *a;
	struct filter_raw_node *b;
	int op;

};

struct filter_raw_node {

	int isbranch;
	union {
		struct filter_raw_data data;
		struct filter_raw_branch branch;
	};
	int not;

};


int filter_raw_parse(char *expr, unsigned int len, struct filter_raw_node **n);
int filter_raw_parse_block(char *expr, unsigned int len, struct filter_raw_node **n);
void filter_raw_cleanup(struct filter_raw_node *fr);

int filter_data_compile(struct filter_data *d, struct data_reg *dr, char *value);
int filter_data_raw_compile(struct filter_data_raw *d, char *value);
int filter_op_compile(struct filter_node *n, struct filter_raw_node *fr);

int filter_packet_compile(struct filter_node **filter, struct filter_raw_node *filter_raw);
int filter_event_compile(struct filter_node **filter, struct event_reg *evt, struct filter_raw_node *filter_raw);
int filter_pload_compile(struct filter_node **filter, struct filter_raw_node *filter_raw);

int filter_node_data_match(struct filter_node *n, struct data *d);

int filter_packet_match(struct filter_node *n, struct proto_process_stack *stack);
int filter_event_match(struct filter_node *n, struct event *evt);
int filter_pload_match(struct filter_node *n, struct pload *p);


#endif

