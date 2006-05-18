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

#ifndef GST_CONF_H
#define GST_CONF_H

#include "gst-types.h"
#include "gst-tool.h"

G_BEGIN_DECLS

void     gst_conf_set_boolean (GstTool *tool, const gchar *key, gboolean value);
gboolean gst_conf_get_boolean (GstTool *tool, const gchar *key);
void     gst_conf_set_integer (GstTool *tool, const gchar *key, gint value);
gint     gst_conf_get_integer (GstTool *tool, const gchar *key);
void     gst_conf_set_string  (GstTool *tool, const gchar *key, const gchar *value);
gchar *  gst_conf_get_string  (GstTool *tool, const gchar *key);
void     gst_conf_add_notify  (GstTool *tool, const gchar *key, GConfClientNotifyFunc func, gpointer data);


G_END_DECLS

#endif /* GST_CONF_H */
