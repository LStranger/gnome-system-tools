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
#include <gnome.h>

#include "xst-conf.h"
#include "xst-tool.h"


static gboolean
xst_conf_error_handler (GError **error)
{
	gboolean value = FALSE;
	
	if (*error) {
		g_warning ("GConf error: %s", (*error)->message);
		value = TRUE;
		g_error_free (*error);
		*error = NULL;
	}

	return value;
}

static gchar *
xst_conf_make_key (XstTool *tool, const gchar *local_key)
{
	gchar *key;

	key = g_strjoin ("/", XST_CONF_ROOT, tool->name, local_key, NULL);

	return key;
}

void
xst_conf_set_boolean (XstTool *tool, const gchar *key, gboolean value)
{
	GError *error = NULL;
	gchar *main_key;
	
	g_return_if_fail (tool != NULL);
	g_return_if_fail (XST_IS_TOOL (tool));
	g_return_if_fail (key != NULL);

	main_key = xst_conf_make_key (tool, key);
	
	gconf_client_set_bool (tool->client, main_key, value, &error);
	xst_conf_error_handler (&error);

	g_free (main_key);
}

gboolean
xst_conf_get_boolean (XstTool *tool, const gchar *key)
{
	gboolean value;
	gchar *main_key;
	GError *error = NULL;
	
	g_return_val_if_fail (tool != NULL, FALSE);
	g_return_val_if_fail (XST_IS_TOOL (tool), FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	main_key = xst_conf_make_key (tool, key);
	
	value = gconf_client_get_bool (tool->client, main_key, &error);
	if (xst_conf_error_handler (&error))
		value = FALSE;

	g_free (main_key);
	
	return value;
}

void
xst_conf_set_integer (XstTool *tool, const gchar *key, gint value)
{
	gchar *main_key;
	GError *error = NULL;
	
	g_return_if_fail (tool != NULL);
	g_return_if_fail (XST_IS_TOOL (tool));
	g_return_if_fail (key != NULL);

	main_key = xst_conf_make_key (tool, key);
	
	gconf_client_set_int (tool->client, main_key, value, &error);
	xst_conf_error_handler (&error);

	g_free (main_key);
}

gint
xst_conf_get_integer (XstTool *tool, const gchar *key)
{
	gint value;
	gchar *main_key;
	GError *error = NULL;
	
	g_return_val_if_fail (tool != NULL, -1);
	g_return_val_if_fail (XST_IS_TOOL (tool), -1);
	g_return_val_if_fail (key != NULL, -1);

	main_key = xst_conf_make_key (tool, key);
	
	value = gconf_client_get_int (tool->client, main_key, &error);
	if (xst_conf_error_handler (&error))
		value = -1;

	g_free (main_key);
	
	return value;
}

void
xst_conf_set_string (XstTool *tool, const gchar *key, const gchar *value)
{
	gchar *main_key;
	GError *error = NULL;
	
	g_return_if_fail (tool != NULL);
	g_return_if_fail (XST_IS_TOOL (tool));
	g_return_if_fail (key != NULL);

	main_key = xst_conf_make_key (tool, key);
	
	gconf_client_set_string (tool->client, main_key, value, &error);
	xst_conf_error_handler (&error);

	g_free (main_key);
}

gchar *
xst_conf_get_string (XstTool *tool, const gchar *key)
{
	gchar *value;
	gchar *main_key;
	GError *error = NULL;
	
	g_return_val_if_fail (tool != NULL, NULL);
	g_return_val_if_fail (XST_IS_TOOL (tool), NULL);
	g_return_val_if_fail (key != NULL, NULL);

	main_key = xst_conf_make_key (tool, key);
	
	value = gconf_client_get_string (tool->client, main_key, &error);
	if (xst_conf_error_handler (&error))
		value = g_strdup ("");

	g_free (main_key);
	
	return value;
}
