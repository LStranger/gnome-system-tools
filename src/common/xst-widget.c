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

#include "xst-widget.h"
#include "xst-dialog.h"


void
xst_widget_apply_policy (XstWidget *xw)
{
	XstDialogComplexity complexity;
	gboolean have_access;
	XstWidgetMode mode;

	g_return_if_fail (xw != NULL);

	complexity = xst_dialog_get_complexity (xw->dialog);
	have_access = xst_tool_get_access (xw->dialog->tool);

	if (complexity == XST_DIALOG_BASIC)
		mode = xw->basic;
	else if (complexity == XST_DIALOG_ADVANCED)
		mode = xw->advanced;
	else
	{
		mode = xw->basic;
		g_error ("Unhandled complexity.");
	}

	/* Show or hide the widget. */

	if (mode == XST_WIDGET_MODE_HIDDEN)
		gtk_widget_hide (xw->widget);
	else if (mode == XST_WIDGET_MODE_INSENSITIVE ||
		 mode == XST_WIDGET_MODE_SENSITIVE)
		gtk_widget_show (xw->widget);
	else
		g_error ("Unhandled widget mode.");

	/* Sensitize or desensitize the widget. Done separately for readability. */

	if (!xw->user_sensitive || mode == XST_WIDGET_MODE_INSENSITIVE ||
	    (have_access == FALSE && xw->need_access == TRUE))
		gtk_widget_set_sensitive (xw->widget, FALSE);
	else
		gtk_widget_set_sensitive (xw->widget, TRUE);
}


XstWidget *
xst_widget_new (GtkWidget *w, XstDialog *d, XstWidgetMode basic, XstWidgetMode advanced,
		gboolean need_access, gboolean user_sensitive)
{
	XstWidget *xw;

	g_return_val_if_fail (w != NULL, NULL);
	g_return_val_if_fail (d != NULL, NULL);

	xw = g_new0 (XstWidget, 1);

	xw->widget         = w;
	xw->dialog         = d;
	xw->basic          = basic;
	xw->advanced       = advanced;
	xw->need_access    = need_access;
	xw->user_sensitive = user_sensitive;

	return (xw);
}


void
xst_widget_set_user_sensitive (XstWidget *xw, gboolean user_sensitive)
{
	xw->user_sensitive = user_sensitive;
	xst_widget_apply_policy (xw);
}
