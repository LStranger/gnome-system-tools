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

#ifndef GST_WIDGET_H
#define GST_WIDGET_H

#include <gtk/gtk.h>

#include "gst-types.h"

struct _GstWidget {
	GtkWidget     *widget;
	GstDialog     *dialog;

	GstWidgetMode  basic;
	GstWidgetMode  advanced;
	GstWidgetMode  user;
	gboolean       need_access;
};

struct _GstWidgetPolicy {
	const gchar   *widget;

	GstWidgetMode  basic;
	GstWidgetMode  advanced;
	gboolean       need_access;
	gboolean       user_sensitive;
};

struct _GstWidgetUserPolicy {
	const gchar *widget;

	GstWidgetMode mode;
};

GstWidget * gst_widget_new      (GstDialog *dialog, GstWidgetPolicy policy);
GstWidget * gst_widget_new_full (GtkWidget *gtk_widget, GstDialog *dialog,
				 GstWidgetMode basic, GstWidgetMode advanced,
				 gboolean need_access, gboolean user_sensitive);


void       gst_widget_apply_policy       (GstWidget *xw);

void       gst_widget_set_user_mode      (GstWidget *xw, GstWidgetMode mode);
void       gst_widget_set_user_sensitive (GstWidget *xw, gboolean user_sensitive);

#endif /* GST_WIDGET_H */
