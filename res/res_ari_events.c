/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 2012 - 2013, Digium, Inc.
 *
 * David M. Lee, II <dlee@digium.com>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * !!!!!                               DO NOT EDIT                        !!!!!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * This file is generated by a mustache template. Please see the original
 * template in rest-api-templates/res_ari_resource.c.mustache
 */

/*! \file
 *
 * \brief WebSocket resource
 *
 * \author David M. Lee, II <dlee@digium.com>
 */

/*** MODULEINFO
	<depend type="module">res_ari</depend>
	<depend type="module">res_stasis</depend>
	<support_level>core</support_level>
 ***/

#include "asterisk.h"

ASTERISK_FILE_VERSION(__FILE__, "$Revision$")

#include "asterisk/module.h"
#include "asterisk/stasis_app.h"
#include "ari/resource_events.h"
#if defined(AST_DEVMODE)
#include "ari/ari_model_validators.h"
#endif

static void ast_ari_event_websocket_ws_cb(struct ast_websocket *ws_session,
	struct ast_variable *get_params, struct ast_variable *headers)
{
	RAII_VAR(struct ast_websocket *, s, ws_session, ast_websocket_unref);
	RAII_VAR(struct ast_ari_websocket_session *, session, NULL, ao2_cleanup);
	struct ast_event_websocket_args args = {};
	struct ast_variable *i;

	for (i = get_params; i; i = i->next) {
		if (strcmp(i->name, "app") == 0) {
			args.app = (i->value);
		} else
		{}
	}
#if defined(AST_DEVMODE)
	session = ast_ari_websocket_session_create(ws_session,
		ast_ari_validate_message_fn());
#else
	session = ast_ari_websocket_session_create(ws_session, NULL);
#endif
	if (!session) {
		ast_log(LOG_ERROR, "Failed to create ARI session\n");
		return;
	}
	ast_ari_websocket_event_websocket(session, headers, &args);
}

/*! \brief REST handler for /api-docs/events.{format} */
static struct stasis_rest_handlers events = {
	.path_segment = "events",
	.callbacks = {
	},
	.num_children = 0,
	.children = {  }
};

static int load_module(void)
{
	int res = 0;
	events.ws_server = ast_websocket_server_create();
	if (!events.ws_server) {
		return AST_MODULE_LOAD_FAILURE;
	}
	res |= ast_websocket_server_add_protocol(events.ws_server,
		"ari", ast_ari_event_websocket_ws_cb);
	stasis_app_ref();
	res |= ast_ari_add_handler(&events);
	return res;
}

static int unload_module(void)
{
	ast_ari_remove_handler(&events);
	ao2_cleanup(events.ws_server);
	events.ws_server = NULL;
	stasis_app_unref();
	return 0;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_DEFAULT, "RESTful API module - WebSocket resource",
	.load = load_module,
	.unload = unload_module,
	.nonoptreq = "res_ari,res_stasis",
	);