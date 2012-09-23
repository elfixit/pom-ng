/*
 *  This file is part of pom-ng.
 *  Copyright (C) 2012 Guy Martin <gmsoft@tuxicoman.be>
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

#include "addon.h"
#include "addon_output.h"

#include <pom-ng/event.h>
#include "addon_event.h"
#include "addon_ptype.h"

struct addon_output *addon_output_head = NULL;

// Called from lua to create a new output class
static int addon_output_new(lua_State *L) {

	// Args should be :
	// 1) name
	// 2) parameter table

	luaL_checkstring(L, 1);

	// Create the new instance
	lua_newtable(L);

	// Assign the metatable
	luaL_getmetatable(L, ADDON_OUTPUT_METATABLE);
	lua_setmetatable(L, -2);

	// Set its name
	lua_pushliteral(L, "name");
	lua_pushvalue(L, 1);
	lua_settable(L, -3);

	// Save the parameter table
	lua_pushliteral(L, "params");
	lua_pushvalue(L, 2);
	lua_settable(L, -3);

	// TODO make fields read-only

	return 1;
}

// Helper function to get the output priv
static struct addon_instance_priv *addon_output_get_priv(lua_State *L, int t) {

	lua_pushliteral(L, "__priv");
	lua_gettable(L, t);
	return luaL_checkudata(L, -1, ADDON_OUTPUT_PRIV_METATABLE);
}

// Called from lua to listen to a new event from an instance
static int addon_output_event_listen_start(lua_State *L) {
	
	// Args should be :
	// 1) self
	// 2) event name
	// 3) process_begin
	// 4) process_end
	
	// Find the event
	const char *evt_name = luaL_checkstring(L, 2);
	
	struct event_reg *evt = event_find(evt_name);
	if (!evt)
		luaL_error(L, "Event %s does not exists", evt_name);

	// Check which function we should register
	int (*process_begin) (struct event *evt, void *obj, struct proto_process_stack *stack, unsigned int stack_index) = NULL;
	int (*process_end) (struct event *evt, void *obj) = NULL;

	if (lua_isfunction(L, 3))
		process_begin = addon_event_process_begin;
	if (lua_isfunction(L, 4))
		process_end = addon_event_process_end;

	if (!process_begin && !process_end)
		luaL_error(L, "No processing function provided");

	// Get the output
	struct addon_instance_priv *p = addon_output_get_priv(L, 1);

	if (event_listener_register(evt, p, process_begin, process_end) != POM_OK)
		luaL_error(L, "Error while listening to event %s", evt_name);

	// Add a table to self for the processing functions of this event
	lua_newtable(L);
	lua_pushlightuserdata(L, evt);
	lua_pushvalue(L, -2);
	lua_settable(L, 1);

	// Add the processing function
	if (process_begin) {
		lua_pushliteral(L, "begin");
		lua_pushvalue(L, 3);
		lua_settable(L, -3);
	}

	if (process_end) {
		lua_pushliteral(L, "end");
		lua_pushvalue(L, 4);
		lua_settable(L, -3);
	}

	pomlog(POMLOG_DEBUG "Output listening to event %s", evt_name);

	return 0;
}

// Called from lua to stop listening to a particular event
static int addon_output_event_listen_stop(lua_State *L) {
	// Args should be :
	// 1) self
	// 2) event name

	// Find the event
	const char *evt_name = luaL_checkstring(L, 2);

	struct event_reg *evt = event_find(evt_name);
	if (!evt)
		luaL_error(L, "Event %s does not exists", evt_name);

	// Get the output
	struct addon_instance_priv *p = addon_output_get_priv(L, 1);
	
	if (event_listener_unregister(evt, p) != POM_OK)
		luaL_error(L, "Error while unregistering event listener");

	// Forget about listening to the event
	lua_pushlightuserdata(L, evt);
	lua_pushnil(L);
	lua_settable(L, 1);

	return 0;
}

// Called from lua to get a parameter value

static int addon_output_param_get(lua_State *L) {

	struct addon_instance_priv *p = addon_output_get_priv(L, 1);

	const char *name = luaL_checkstring(L, 2);

	struct addon_param *tmp;
	for (tmp = p->params; tmp && strcmp(tmp->name, name); tmp = tmp->next);
	
	if (!tmp)
		luaL_error(L, "No such parameter %s", name);

	addon_ptype_push(L, tmp->value);

	return 1;
}

// Garbage collector function for an output parameter
static int addon_output_priv_gc(lua_State *L) {
	struct addon_instance_priv *priv = luaL_checkudata(L, 1, ADDON_OUTPUT_PRIV_METATABLE);

	while (priv->params) {
		struct addon_param *tmp = priv->params;
		priv->params = tmp->next;
		free(tmp->name);
		ptype_cleanup(tmp->value);
		free(tmp);
	}

	return 0;
}

// Garbage collector function for output class
static int addon_output_gc(lua_State *L) {
	struct output_reg_info *output_reg = luaL_checkudata(L, 1, ADDON_OUTPUT_REG_METATABLE);
	if (output_reg)
		free(output_reg->name);
	return 0;
}

int addon_output_lua_register(lua_State *L) {

	// Register the output functions
	struct luaL_Reg l[] = {
		{ "new", addon_output_new },
		{ "register", addon_output_register },
		{ 0 }
	};
	luaL_register(L, ADDON_POM_OUTPUT_LIB, l);


	// Create the output instance metatable
	struct luaL_Reg m[] = {
		{ "event_listen_start", addon_output_event_listen_start },
		{ "event_listen_stop", addon_output_event_listen_stop },
		{ "param_get", addon_output_param_get },

		{ 0 }
	};

	luaL_newmetatable(L, ADDON_OUTPUT_METATABLE);
	// Assign __index to itself
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);
	luaL_register(L, NULL, m);


	// Create the output_reg metatable
	struct luaL_Reg m_reg[] = {
		{ "__gc", addon_output_gc },
		{ 0 }
	};
	luaL_newmetatable(L, ADDON_OUTPUT_REG_METATABLE);
	luaL_register(L, NULL, m_reg);

	
	// Ceate the output_priv metatable
	struct luaL_Reg m_priv[] = {
		{ "__gc", addon_output_priv_gc },
		{ 0 }
	};
	luaL_newmetatable(L, ADDON_OUTPUT_PRIV_METATABLE);
	luaL_register(L, NULL, m_priv);


	return POM_OK;
}

int addon_output_register(lua_State *L) {

	struct addon *addon = addon_get_from_registry(L);

	// Get the name of the output
	lua_pushliteral(L, "name");
	lua_gettable(L, 1);
	const char *name = lua_tostring(L, -1);

	pomlog(POMLOG_DEBUG "Registering addon output %s ...", name);

	struct output_reg_info *output_info = lua_newuserdata(L, sizeof(struct output_reg_info));
	memset(output_info, 0, sizeof(struct output_reg_info));

	// Add the output_reg metatable
	luaL_getmetatable(L, ADDON_OUTPUT_REG_METATABLE);
	lua_setmetatable(L, -2);
	
	output_info->name = strdup(name);
	if (!output_info->name)
		addon_oom(L, strlen(name) + 1);

	output_info->api_ver = OUTPUT_API_VER;
	output_info->mod = addon->mod;
	output_info->init = addon_output_init;
	output_info->open = addon_output_open;
	output_info->close = addon_output_close;
	output_info->cleanup = addon_output_cleanup;

	if (output_register(output_info) != POM_OK)
		luaL_error(L, "Error while registering addon input %s", name);

	// Save our template in the registry
	lua_pushlightuserdata(L, output_info);
	lua_pushvalue(L, 1);
	lua_settable(L, LUA_REGISTRYINDEX);

	pomlog(POMLOG_DEBUG "Registered addon output %s", name);

	return 0;
}

int addon_output_init(struct output *o) {

	struct addon *addon = o->info->reg_info->mod->priv;

	lua_State *L = lua_newthread(addon->L);

	if (!L) {
		pomlog(POMLOG_ERR "Error while creating new lua state for output %s", o->info->reg_info->name);
		return POM_ERR;
	}

	// Create a new global table for this thread to avoid memory corruption
	lua_newtable(L);
	// Create a new metatable to 'inherit' the main thread global
	lua_newtable(L);
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	lua_settable(L, -3);
	// Set the metatable to the new global table
	lua_setmetatable(L, -2);
	// Replace the old table
	lua_replace(L, LUA_GLOBALSINDEX);


	// Create a new instance of the class
	lua_newtable(L);

	// Do the inheritence
	lua_newtable(L);
	lua_pushliteral(L, "__index");
	lua_pushlightuserdata(L, o->info->reg_info);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_settable(L, -3);
	lua_setmetatable(L, -2);

	// Create the private data
	lua_pushliteral(L, "__priv");
	// TODO make __priv read-only
	struct addon_instance_priv *p = lua_newuserdata(L, sizeof(struct addon_instance_priv));
	memset(p, 0, sizeof(struct addon_instance_priv));
	o->priv = p;
	p->instance = o;
	p->L = L;
	// Assign the output_priv metatable
	luaL_getmetatable(L, ADDON_OUTPUT_PRIV_METATABLE);
	lua_setmetatable(L, -2);
	// Add it to __priv
	lua_settable(L, -3);

	// Add the new instance in the registry
	lua_pushlightuserdata(L, p);
	lua_pushvalue(L, -2);
	lua_settable(L, LUA_REGISTRYINDEX);
	
	// Fetch the parameters table from the class
	lua_pushlightuserdata(L, o->info->reg_info);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushliteral(L, "params");
	lua_gettable(L, -2);

	// Parse each param from the class
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		if (!lua_istable(L, -1))
			pomlog(POMLOG_ERR "Parameters should be described in tables");

		// Fetch parameter data
		// Stack at this point :
		// instance (table)
		// params (table) // table from the class
		// key
		// param (table) // current parameter
	
		// Fetch the name
		lua_pushinteger(L, 1);
		lua_gettable(L, -2);
		if (!lua_isstring(L, -1)) {
			pomlog(POMLOG_ERR "Parameter name is not a string");
			return POM_ERR;
		}
		const char *name = luaL_checkstring(L, -1);
		lua_pop(L, 1);

		// Fetch the ptype type
		lua_pushinteger(L, 2);
		lua_gettable(L, -2);
		if (!lua_isstring(L, -1)) {
			pomlog(POMLOG_ERR "Parameter type is not a string");
			// Add it to __priv
			return POM_ERR;
		}
		const char *type = lua_tostring(L, -1);
		lua_pop(L, 1);

		// Fetch the default value
		lua_pushinteger(L, 3);
		lua_gettable(L, -2);
		if (!lua_isstring(L, -1)) {
			pomlog(POMLOG_ERR "Parameter default value is not a string");
			return POM_ERR;
		}
		const char *defval = lua_tostring(L, -1);
		lua_pop(L, 1);

		// Fetch the description
		lua_pushinteger(L, 4);
		lua_gettable(L, -2);
		if (!lua_isstring(L, -1)) {
			pomlog(POMLOG_ERR "Parameter description is not a string");
			return POM_ERR;
		}
		const char *descr = lua_tostring(L, -1);
		lua_pop(L, 1);

		// Allocate it
		struct addon_param *param = malloc(sizeof(struct addon_param));
		if (!param) {
			pom_oom(sizeof(struct addon_param));
			return POM_ERR;
		}
		param->name = strdup(name);
		if (!param->name) {
			free(param);
			pom_oom(strlen(name) + 1);
			return POM_ERR;
		}
		param->value = ptype_alloc(type);
		if (!param->value) {
			free(param->name);
			free(param);
			return POM_ERR;
		}
		
		struct registry_param *reg_param = registry_new_param((char*)name, (char*)defval, param->value, (char*)descr, 0);
		if (output_instance_add_param(o, reg_param) != POM_OK) {
			if (p)
				registry_cleanup_param(reg_param);
			free(param->name);
			ptype_cleanup(param->value);
			free(param);
			return POM_ERR;
		}

		param->next = p->params;
		p->params = param;


		// Pop the value (the param table)
		lua_pop(L, 1);
	}

	pomlog(POMLOG_DEBUG "Output %s created", o->name);
	return POM_OK;
}

int addon_output_cleanup(void *output_priv) {
	
	struct addon_instance_priv *p = output_priv;

	lua_pushlightuserdata(p->L, p);
	lua_pushnil(p->L);
	lua_settable(p->L, LUA_REGISTRYINDEX);

	return POM_OK;
}

int addon_output_open(void *output_priv) {

	struct addon_instance_priv *p = output_priv;
	if (addon_get_instance(p) != POM_OK)
		return POM_ERR;
	return addon_call(p->L, "open", 0);
}

int addon_output_close(void *output_priv) {

	struct addon_instance_priv *p = output_priv;
	if (addon_get_instance(p) != POM_OK)
		return POM_ERR;
	return addon_call(p->L, "close", 0);
}
