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

#ifndef XST_CONF_H
#define XST_CONF_H

#include <gnome.h>
#include "xst-types.h"
#include "xst-tool.h"

BEGIN_GNOME_DECLS

void     xst_conf_set_boolean (XstTool *tool, const gchar *key, gboolean value);
gboolean xst_conf_get_boolean (XstTool *tool, const gchar *key);
void     xst_conf_set_integer (XstTool *tool, const gchar *key, gint value);
gint     xst_conf_get_integer (XstTool *tool, const gchar *key);
void     xst_conf_set_string  (XstTool *tool, const gchar *key, const gchar *value);
gchar *  xst_conf_get_string  (XstTool *tool, const gchar *key);

END_GNOME_DECLS

#endif /* XST_CONF_H */
