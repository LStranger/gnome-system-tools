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
 * Authors: Hans Petter Jansson <hpj@ximian.com>
 */

#ifndef GST_PLATFORM_H
#define GST_PLATFORM_H

#include "gst-types.h"
#include "gst-tool.h"

struct _GstPlatform {
	gchar *name;
	gchar *key;
};

GstPlatform     *gst_platform_new                  (const gchar *key, const gchar *name);
GstPlatform     *gst_platform_new_from_node        (xmlNodePtr node);
GstPlatform     *gst_platform_new_from_report_line (GstReportLine *rline);
GstPlatform     *gst_platform_dup                  (GstPlatform *platform);
gint             gst_platform_cmp                  (GstPlatform *a, GstPlatform *b);

void             gst_platform_free                 (GstPlatform *platform);

const gchar     *gst_platform_get_key              (GstPlatform *platform);
const GdkPixbuf	*gst_platform_get_pixmap	   (GstPlatform *platform);
const gchar     *gst_platform_get_name             (GstPlatform *platform);

#endif /* GST_PLATFORM_H */
