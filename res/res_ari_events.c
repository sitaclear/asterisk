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

#include "asterisk/app.h"
#include "asterisk/module.h"
#include "asterisk/stasis_app.h"
#include "ari/resource_events.h"
#if defined(AST_DEVMODE)
#include "ari/ari_model_validators.h"
#endif
#include "asterisk/http_websocket.h"

#define MAX_VALS 128

static void ast_ari_events_event_websocket_ws_cb(struct ast_websocket *ws_session,
	struct ast_variable *get_params, struct ast_variable *headers)
{
	struct ast_ari_events_event_websocket_args args = {};
	RAII_VAR(struct ast_ari_response *, response, NULL, ast_free);
	struct ast_variable *i;
	RAII_VAR(struct ast_websocket *, s, ws_session, ast_websocket_unref);
	RAII_VAR(struct ast_ari_websocket_session *, session, NULL, ao2_cleanup);

	response = ast_calloc(1, sizeof(*response));
	if (!response) {
		ast_log(LOG_ERROR, "Failed to create response.\n");
		goto fin;
	}

#if defined(AST_DEVMODE)
	session = ast_ari_websocket_session_create(ws_session,
		ast_ari_validate_message_fn());
#else
	session = ast_ari_websocket_session_create(ws_session, NULL);
#endif
	if (!session) {
		ast_log(LOG_ERROR, "Failed to create ARI session\n");
		goto fin;
	}

	for (i = get_params; i; i = i->next) {
		if (strcmp(i->name, "app") == 0) {
			/* Parse comma separated list */
			char *vals[MAX_VALS];
			size_t j;

			args.app_parse = ast_strdup(i->value);
			if (!args.app_parse) {
				ast_ari_response_alloc_failed(response);
				goto fin;
			}

			if (strlen(args.app_parse) == 0) {
				/* ast_app_separate_args can't handle "" */
				args.app_count = 1;
				vals[0] = args.app_parse;
			} else {
				args.app_count = ast_app_separate_args(
					args.app_parse, ',', vals,
					ARRAY_LEN(vals));
			}

			if (args.app_count == 0) {
				ast_ari_response_alloc_failed(response);
				goto fin;
			}

			if (args.app_count >= MAX_VALS) {
				ast_ari_response_error(response, 400,
					"Bad Request",
					"Too many values for app");
				goto fin;
			}

			args.app = ast_malloc(sizeof(*args.app) * args.app_count);
			if (!args.app) {
				ast_ari_response_alloc_failed(response);
				goto fin;
			}

			for (j = 0; j < args.app_count; ++j) {
				args.app[j] = (vals[j]);
			}
		} else
		{}
	}

	ast_ari_websocket_events_event_websocket(session, headers, &args);

fin: __attribute__((unused))
	if (response && response->response_code != 0) {
		/* Param parsing failure */
		/* TODO - ideally, this would return the error code to the
		 * HTTP client; but we've already done the WebSocket
		 * negotiation. Param parsing should happen earlier, but we
		 * need a way to pass it through the WebSocket code to the
		 * callback */
		RAII_VAR(char *, msg, NULL, ast_json_free);
		if (response->message) {
			msg = ast_json_dump_string(response->message);
		} else {
			ast_log(LOG_ERROR, "Missing response message\n");
		}
		if (msg) {
			ast_websocket_write(ws_session,
				AST_WEBSOCKET_OPCODE_TEXT, msg,	strlen(msg));
		}
	}
	ast_free(args.app_parse);
	ast_free(args.app);
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
		"ari", ast_ari_events_event_websocket_ws_cb);
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
