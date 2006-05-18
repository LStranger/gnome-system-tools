/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Tambet Ingo <tambet@ximian.com>
 */

#include <config.h>
#include <gconf/gconf-client.h>

#include "gst-conf.h"
#include "gst-tool.h"


static gchar *
gst_conf_make_key (GstTool *tool, const gchar *local_key)
{
	gchar *key;

	key = g_strjoin ("/", GST_GCONF_ROOT, tool->name, local_key, NULL);

	return key;
}

/**
 * gst_conf_set_boolean:
 * @tool: An GstTool object
 * @key: A string key (path) for an item
 * @value: gboolean value to set
 * 
 * Store boolean @value to @key for @tool. @key is just the name of the variable
 * which will be stored in form of "/GST_CONF_ROOT/$tool_name/@key".
 **/
void
gst_conf_set_boolean (GstTool *tool, const gchar *key, gboolean value)
{
	gchar *main_key;
	GConfClient* client;
        GError *error = NULL;

	g_return_if_fail (tool != NULL);
	g_return_if_fail (GST_IS_TOOL (tool));
	g_return_if_fail (key != NULL);

	client = gconf_client_get_default ();

	main_key = gst_conf_make_key (tool, key);

	gconf_client_set_bool (client, main_key, value, &error);

	g_free (main_key);
}

/**
 * gst_conf_get_boolean:
 * @tool: An GstTool object
 * @key: A string key (path) for an item
 * 
 * Retrieve boolean value from given @tool and @key. Use @key in form of
 * "mystuff=1" to give default return value if @key isn't stored.
 * @key is name of variable WITHOUT any preffix.
 * 
 * Return Value: gboolean value if key is stored or default value if provided
 * by @key or FALSE otherwise.
 **/
gboolean
gst_conf_get_boolean (GstTool *tool, const gchar *key)
{
	gboolean value;
	gchar *main_key;
	GConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (tool != NULL, FALSE);
	g_return_val_if_fail (GST_IS_TOOL (tool), FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	client = gconf_client_get_default ();

	main_key = gst_conf_make_key (tool, key);
	
	value = gconf_client_get_bool (client, main_key, &error);

	g_free (main_key);
	
	return value;
}

/**
 * gst_conf_set_integer:
 * @tool: An GstTool object
 * @key: A string key (path) for an item
 * @value: integer value to store
 * 
 * Store integer @value to @key for @tool. @key is just the name of the variable
 * which will be stored in form of "/GST_CONF_ROOT/$tool_name/@key".
 **/
void
gst_conf_set_integer (GstTool *tool, const gchar *key, gint value)
{
	gchar *main_key;
	GConfClient *client;
	GError *error = NULL;
	
	g_return_if_fail (tool != NULL);
	g_return_if_fail (GST_IS_TOOL (tool));
	g_return_if_fail (key != NULL);

	client = gconf_client_get_default ();

	main_key = gst_conf_make_key (tool, key);
	
	gconf_client_set_int (client, main_key, value, &error);

	g_free (main_key);
}

/**
 * gst_conf_get_integer:
 * @tool: An GstTool object
 * @key: A string key (path) for an item
 * 
 * Retrieve integer value from given @tool and @key. Use @key in form of
 * "mystuff=1" to give default return value if @key isn't stored.
 * @key is name of variable WITHOUT any preffix.
 * 
 * Return Value: Integer value if key is stored or default value if provided
 * by @key or 0 otherwise.
 **/
gint
gst_conf_get_integer (GstTool *tool, const gchar *key)
{
	gint value;
	gchar *main_key;
	GConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (tool != NULL, -1);
	g_return_val_if_fail (GST_IS_TOOL (tool), -1);
	g_return_val_if_fail (key != NULL, -1);

	client = gconf_client_get_default ();

	main_key = gst_conf_make_key (tool, key);
	
	value = gconf_client_get_int (client, main_key, &error);

	g_free (main_key);
	
	return value;
}

/**
 * gst_conf_set_string:
 * @tool: An GstTool object
 * @key: A string key (path) for an item
 * @value: String value to set
 * 
 * Store string @value to @key for @tool. @key is just the name of the variable
 * which will be stored in form of "/GST_CONF_ROOT/$tool_name/@key".
 **/
void
gst_conf_set_string (GstTool *tool, const gchar *key, const gchar *value)
{
	gchar *main_key;
	GConfClient *client;
	GError *error = NULL;
	
	g_return_if_fail (tool != NULL);
	g_return_if_fail (GST_IS_TOOL (tool));
	g_return_if_fail (key != NULL);

	client = gconf_client_get_default ();

	main_key = gst_conf_make_key (tool, key);
	
	gconf_client_set_string (client, main_key, value, &error);

	g_free (main_key);
}

/**
 * gst_conf_get_string:
 * @tool: An GstTool object
 * @key: A string key (path) for an item
 * 
 * Retrieve string value from given @tool and @key. Use @key in form of
 * "mystuff=foo" to give default return value if @key isn't stored.
 * @key is name of variable WITHOUT any preffix.
 * 
 * Return Value: Pointer to gchar value if key is stored or default value if
 * provided by @key or FALSE otherwise. Must be g_free()'d.
 **/
gchar *
gst_conf_get_string (GstTool *tool, const gchar *key)
{
	gchar *value;
	gchar *main_key;
	GConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (tool != NULL, NULL);
	g_return_val_if_fail (GST_IS_TOOL (tool), NULL);
	g_return_val_if_fail (key != NULL, NULL);

	client = gconf_client_get_default ();

	main_key = gst_conf_make_key (tool, key);
	
	value = gconf_client_get_string (client, main_key, &error);

	g_free (main_key);
	
	return value;
}

void
gst_conf_add_notify (GstTool               *tool,
		     const gchar           *key,
		     GConfClientNotifyFunc  func,
		     gpointer               data)
{
	gchar *main_key;

	main_key = gst_conf_make_key (tool, key);

	gconf_client_add_dir (tool->gconf_client, main_key,
			      GCONF_CLIENT_PRELOAD_NONE,
			      NULL);

	gconf_client_notify_add (tool->gconf_client,
				 main_key, func, data,
				 NULL, NULL);
	g_free (main_key);
}
