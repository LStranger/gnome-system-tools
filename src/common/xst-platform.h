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

#ifndef XST_PLATFORM_H
#define XST_PLATFORM_H

#include "xst-types.h"
#include "xst-tool.h"

struct _XstPlatform {
	gchar *name;
	gchar *key;
};

XstPlatform     *xst_platform_new                  (const gchar *key, const gchar *name);
XstPlatform     *xst_platform_new_from_node        (xmlNodePtr node);
XstPlatform     *xst_platform_new_from_report_line (XstReportLine *rline);
XstPlatform     *xst_platform_dup                  (XstPlatform *platform);
gint             xst_platform_cmp                  (XstPlatform *a, XstPlatform *b);

void             xst_platform_free                 (XstPlatform *platform);

const gchar     *xst_platform_get_key              (XstPlatform *platform);
const GdkPixbuf	*xst_platform_get_pixmap	   (XstPlatform *platform);
const gchar     *xst_platform_get_name             (XstPlatform *platform);

#endif /* XST_PLATFORM_H */
