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

#include <config.h>

#include "gst-widget.h"
#include "gst-dialog.h"


void
gst_widget_apply_policy (GstWidget *xw)
{
	GstDialogComplexity complexity;
	gboolean have_access;
	GstWidgetMode mode;

	g_return_if_fail (xw != NULL);

	complexity = GST_DIALOG_ADVANCED;
	have_access = gst_tool_is_authenticated (xw->dialog->tool);

	if (complexity == GST_DIALOG_BASIC)
		mode = xw->basic;
	else if (complexity == GST_DIALOG_ADVANCED)
		mode = xw->advanced;
	else
	{
		mode = xw->basic;
		g_error ("Unhandled complexity.");
	}

	if (xw->user < mode)
		mode = xw->user;

	/* Show or hide the widget. */

	if (mode == GST_WIDGET_MODE_HIDDEN)
		gtk_widget_hide (xw->widget);
	else if (mode == GST_WIDGET_MODE_INSENSITIVE ||
		 mode == GST_WIDGET_MODE_SENSITIVE)
		gtk_widget_show (xw->widget);
	else
		g_error ("Unhandled widget mode.");

	/* Sensitize or desensitize the widget. Done separately for readability. */

	if (mode == GST_WIDGET_MODE_INSENSITIVE ||
	    (have_access == FALSE && xw->need_access == TRUE))
		gtk_widget_set_sensitive (xw->widget, FALSE);
	else
		gtk_widget_set_sensitive (xw->widget, TRUE);
}


GstWidget *
gst_widget_new_full (GtkWidget *w, GstDialog *d, GstWidgetMode basic, GstWidgetMode advanced,
		     gboolean need_access, gboolean user_sensitive)
{
	GstWidget *xw;

	g_return_val_if_fail (w != NULL, NULL);
	g_return_val_if_fail (d != NULL, NULL);

	xw = g_new0 (GstWidget, 1);

	xw->widget         = w;
	xw->dialog         = d;
	xw->basic          = basic;
	xw->advanced       = advanced;
	xw->need_access    = need_access;

	if (user_sensitive)
		xw->user = GST_WIDGET_MODE_SENSITIVE;
	else
		xw->user = GST_WIDGET_MODE_INSENSITIVE;

	d->gst_widget_list = g_slist_prepend (d->gst_widget_list, xw);
	
	return xw;
}

GstWidget *
gst_widget_new (GstDialog *dialog, GstWidgetPolicy policy)
{
	return gst_widget_new_full (gst_dialog_get_widget (dialog, policy.widget),
				    dialog,
				    policy.basic, policy.advanced,
				    policy.need_access, policy.user_sensitive);
}

void
gst_widget_set_user_mode (GstWidget *xw, GstWidgetMode mode)
{
	xw->user = mode;
	gst_widget_apply_policy (xw);
}


/* Backwards compatibility function. Will be removed as soon as all references to
 * it are cleaned out. */

void
gst_widget_set_user_sensitive (GstWidget *xw, gboolean user_sensitive)
{
	if (user_sensitive)
		gst_widget_set_user_mode (xw, GST_WIDGET_MODE_SENSITIVE);
	else
		gst_widget_set_user_mode (xw, GST_WIDGET_MODE_INSENSITIVE);
}
