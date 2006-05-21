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

#ifndef GST_DIALOG_H
#define GST_DIALOG_H

#include <glade/glade.h>
#include "gst-types.h"
#include "gst-tool.h"

#define GST_TYPE_DIALOG        (gst_dialog_get_type ())
#define GST_DIALOG(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o),  GST_TYPE_DIALOG, GstDialog))
#define GST_DIALOG_CLASS(c)    (G_TYPE_CHECK_CLASS_CAST ((c), GST_TYPE_DIALOG, GstDialogClass))
#define GST_IS_DIALOG(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_DIALOG))
#define GST_IS_DIALOG_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), GST_TYPE_DIALOG))

typedef gboolean (*GstDialogHookFunc) (GstDialog *dialog, gpointer data);

typedef struct {
	gpointer data;
	GstDialogHookFunc func;
} GstDialogHookEntry;

struct _GstDialogSignal {
	const char    *widget;
	const char    *signal_name;
	GCallback  func;
};

struct _GstDialog {
	GtkDialog dialog;
	GstTool *tool;

	/* Glade files */
	GladeXML  *gui;
	GtkWidget *child;

	gboolean is_modified;

	GstDialogComplexity complexity;
	gboolean frozen;

	GSList *gst_widget_list;
	GList *apply_hook_list;
};

struct _GstDialogClass {
	GtkDialogClass parent_class;
};

GType               gst_dialog_get_type            (void);

GstDialog          *gst_dialog_new                 (GstTool *tool, 
						    const char *widget, 
						    const char *title);
void                gst_dialog_construct           (GstDialog *dialog,
						    GstTool *tool, 
						    const char *widget, 
						    const char *title);

void                gst_dialog_connect_signals     (GstDialog *xd, GstDialogSignal *signals);
void                gst_dialog_connect_signals_after (GstDialog *xd, GstDialogSignal *signals);

GstDialogComplexity gst_dialog_get_complexity      (GstDialog *xd);
void                gst_dialog_set_complexity      (GstDialog *xd, GstDialogComplexity c);
void                gst_dialog_enable_complexity   (GstDialog *xd);

void                gst_dialog_freeze              (GstDialog *xd);
void                gst_dialog_thaw                (GstDialog *xd);

void                gst_dialog_freeze_visible      (GstDialog *xd);
void                gst_dialog_thaw_visible        (GstDialog *xd);

gboolean            gst_dialog_get_modified        (GstDialog *xd);
void                gst_dialog_set_modified        (GstDialog *xd, gboolean state);
void                gst_dialog_modify              (GstDialog *xd);
void                gst_dialog_modify_cb           (GtkWidget *w, gpointer data);

GtkWidget          *gst_dialog_get_widget          (GstDialog *xd, const char *widget);
GstWidget          *gst_dialog_get_gst_widget      (GstDialog *xd, const gchar *name);

GstTool            *gst_dialog_get_tool            (GstDialog *xd);

void                gst_dialog_widget_set_user_mode (GstDialog *xs, const gchar *name, GstWidgetMode mode);
void                gst_dialog_widget_set_user_sensitive (GstDialog *xd, const gchar *name, gboolean state);

void                gst_dialog_apply_widget_policies (GstDialog*);
void                gst_dialog_set_widget_policies (GstDialog *xd, const GstWidgetPolicy *xwp);
void                gst_dialog_set_widget_user_modes (GstDialog *xd, const GstWidgetUserPolicy *xwup);

void                gst_dialog_add_apply_hook      (GstDialog *xd, GstDialogHookFunc func, gpointer data);
gboolean            gst_dialog_run_apply_hooks     (GstDialog *xd);

void                gst_dialog_ask_apply           (GstDialog*);

#endif /* GST_DIALOG_H */
