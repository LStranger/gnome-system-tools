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

#ifndef XST_TOOL_H
#define XST_TOOL_H

#include <tree.h>
#include <gtk/gtkobject.h>
#include <glade/glade.h>
#include <popt.h>

#include "xst-types.h"

#define XST_TYPE_TOOL        (xst_tool_get_type ())
#define XST_TOOL(o)          (GTK_CHECK_CAST ((o),  XST_TYPE_TOOL, XstTool))
#define XST_TOOL_CLASS(c)    (GTK_CHECK_CLASS_CAST ((c), XST_TYPE_TOOL, XstToolClass))
#define XST_IS_TOOL(o)       (GTK_CHECK_TYPE ((o), XST_TYPE_TOOL))
#define XST_IS_TOOL_CLASS(c) (GTK_CHECK_CLASS_TYPE ((c), XST_TYPE_TOOL))

typedef void (*XstXmlFunc)   (XstTool *tool, gpointer data);
typedef void (*XstCloseFunc) (XstTool *tool, gpointer data);

struct _XstTool {
	GtkObject object;
	char *name;

	char *glade_path;
	char *glade_common_path;
	char *script_path;

	/* configuration */
	xmlDoc  *config;

	XstDialog *main_dialog;

	/* Platform selection */
	GtkWidget *platform_dialog;
	GtkWidget *platform_list;
	GtkWidget *platform_ok_button;

	gint platform_selected_row;

	/* Progress report widgets */
	GtkWidget *report_arrow;

	GtkWidget *report_window;
	GtkWidget *report_scrolled;
	GtkWidget *report_progress;
	GtkWidget *report_label;
	GtkWidget *report_list;
	GtkWidget *report_entry;
	GtkWidget *report_visibility;
	GtkWidget *report_notebook;

	gboolean timeout_done;
	gboolean report_list_visible;

	char *line;
	int line_len;
	guint input_id;
	GSList *report_line_list;
	gboolean report_dispatch_pending;
	gboolean report_finished;
	gboolean run_again;

	XstReportHookType report_hook_type;
	GSList *report_hook_list;

	XstPlatform *current_platform;     /* Always set from backend report */
	GSList *supported_platforms_list;  /* Gets set only if backend breaks */
};

struct _XstToolClass {
	GtkObjectClass parent_class;

	void (*fill_gui) (XstTool *xt);
	void (*fill_xml) (XstTool *xt);
};

GtkType      xst_tool_get_type            (void);

void         xst_init                     (const gchar *app_name,
					   int argc, char *argv [],
					   const poptOption options);

void         xst_tool_main                (XstTool *tool);
gboolean     xst_tool_get_access          (XstTool *tool);

XstTool     *xst_tool_new                 (void);
void         xst_tool_construct           (XstTool *tool, 
					   const char *name, const char *title);

gboolean     xst_tool_save                (XstTool *tool);
void         xst_tool_save_cb             (GtkWidget *w, XstTool *tool);
gboolean     xst_tool_load                (XstTool *tool);
void         xst_tool_load_try            (XstTool *tool);
void         xst_tool_set_xml_funcs       (XstTool *tool, XstXmlFunc load_cb, XstXmlFunc save_cb, gpointer data);
void         xst_tool_set_close_func      (XstTool *tool, XstCloseFunc close_cb, gpointer data);

GladeXML    *xst_tool_load_glade_common   (XstTool *tool, const gchar *widget);
GladeXML    *xst_tool_load_glade          (XstTool *tool, const gchar *widget);

XstDialog   *xst_tool_get_dialog          (XstTool *tool);

void         xst_tool_add_report_hooks    (XstTool *tool, XstReportHookEntry *report_hook_table);
void         xst_tool_invoke_report_hooks (XstTool *tool, XstReportHookType type, XstReportLine *rline);
void         xst_tool_reset_report_hooks  (XstTool *tool);

void         xst_tool_add_supported_platform    (XstTool *tool, XstPlatform *platform);
void         xst_tool_clear_supported_platforms (XstTool *tool);

#endif /* XST_TOOL_H */
