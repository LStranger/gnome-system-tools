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

#ifndef XST_WIDGET_H
#define XST_WIDGET_H

#include <gtk/gtk.h>

#include "xst-types.h"

struct _XstWidget {
	GtkWidget     *widget;
	XstDialog     *dialog;

	XstWidgetMode  basic;
	XstWidgetMode  advanced;
	gboolean       need_access;
	gboolean       user_sensitive;
};

struct _XstWidgetPolicy {
	const gchar   *widget;

	XstWidgetMode  basic;
	XstWidgetMode  advanced;
	gboolean       need_access;
	gboolean       user_sensitive;
};

XstWidget *xst_widget_new                (GtkWidget *w, XstDialog *d, XstWidgetMode basic,
			                  XstWidgetMode advanced, gboolean need_access,
			                  gboolean user_sensitive);

void       xst_widget_apply_policy       (XstWidget *xw);

void       xst_widget_set_user_sensitive (XstWidget *xw, gboolean user_sensitive);

#endif /* XST_WIDGET_H */
