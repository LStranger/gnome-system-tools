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
 * Authors: Jacob Berkman <jacob@ximian.com>
 */

#ifndef XST_DIALOG_H
#define XST_DIALOG_H

#include <libgnomeui/gnome-app.h>
#include <glade/glade.h>
#include "xst-types.h"
#include "xst-tool.h"

#define XST_TYPE_DIALOG        (xst_dialog_get_type ())
#define XST_DIALOG(o)          (GTK_CHECK_CAST ((o),  XST_TYPE_DIALOG, XstDialog))
#define XST_DIALOG_CLASS(c)    (GTK_CHECK_CLASS_CAST ((c), XST_TYPE_DIALOG, XstDialogClass))
#define XST_IS_DIALOG(o)       (GTK_CHECK_TYPE ((o), XST_TYPE_DIALOG))
#define XST_IS_DIALOG_CLASS(c) (GTK_CHECK_CLASS_TYPE ((c), XST_TYPE_DIALOG))

typedef gboolean (*XstDialogHookFunc) (XstDialog *dialog, gpointer data);

typedef struct {
	gpointer data;
	XstDialogHookFunc func;
} XstDialogHookEntry;

struct _XstDialogSignal {
	const char    *widget;
	const char    *signal_name;
	GtkSignalFunc  func;
};

struct _XstDialog {
	GnomeApp app;
	XstTool *tool;

	/* Glade files */
	GladeXML  *gui;
	GtkWidget *child;

	/* Common widgets */
	GtkWidget *apply_button;
	GtkWidget *complexity_button;

	XstDialogComplexity complexity;
	gint frozen;

	const XstWidgetPolicy **widget_policies;
	GList *apply_hook_list;
};

struct _XstDialogClass {
	GnomeAppClass parent_class;

	void (*apply)             (XstDialog *);
	void (*complexity_change) (XstDialog *);
};

GtkType             xst_dialog_get_type            (void);

XstDialog          *xst_dialog_new                 (XstTool *tool, 
						    const char *widget, 
						    const char *title);
void                xst_dialog_construct           (XstDialog *dialog,
						    XstTool *tool, 
						    const char *widget, 
						    const char *title);

void                xst_dialog_connect_signals     (XstDialog *xd, XstDialogSignal *signals);

XstDialogComplexity xst_dialog_get_complexity      (XstDialog *xd);
void                xst_dialog_set_complexity      (XstDialog *xd, XstDialogComplexity c);
void                xst_dialog_enable_complexity   (XstDialog *xd);

void                xst_dialog_freeze              (XstDialog *xd);
void                xst_dialog_thaw                (XstDialog *xd);

gboolean            xst_dialog_get_modified        (XstDialog *xd);
void                xst_dialog_modify              (XstDialog *xd);
void                xst_dialog_modify_cb           (GtkWidget *w, gpointer data);

GtkWidget          *xst_dialog_get_widget          (XstDialog *xd, const char *widget);
void                xst_dialog_add_apply_hook      (XstDialog *xd, XstDialogHookFunc *func, gpointer data);
void                xst_dialog_set_widget_policies (XstDialog *xd, const XstWidgetPolicy **xdw);


#endif /* XST_DIALOG_H */
