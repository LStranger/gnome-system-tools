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

#ifndef GST_TOOL_H
#define GST_TOOL_H

#include <glade/glade.h>
#include <gtk/gtk.h>

#include "gst-types.h"

#define GST_TYPE_TOOL        (gst_tool_get_type ())
#define GST_TOOL(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o),  GST_TYPE_TOOL, GstTool))
#define GST_TOOL_CLASS(c)    (G_TYPE_CHECK_CLASS_CAST ((c), GST_TYPE_TOOL, GstToolClass))
#define GST_IS_TOOL(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_TOOL))
#define GST_IS_TOOL_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), GST_TYPE_TOOL))

typedef void (*GstXmlFunc)   (GstTool *tool, gpointer data);
typedef void (*GstCloseFunc) (GstTool *tool, gpointer data);

typedef enum {
	ROOT_ACCESS_NONE,
	ROOT_ACCESS_SIMULATED,
	ROOT_ACCESS_SIMULATED_DISABLED,
	ROOT_ACCESS_REAL
} RootAccess;

struct _GstTool {
	GObject object;
	char *name;

	RootAccess root_access;

	char *glade_path;
	char *glade_common_path;
	char *script_path;

	char *script_name;

	/* backend process */
	int backend_pid;
	int write_fd;
	int read_fd;

	FILE *write_stream;
	FILE *read_stream;

	/* configuration */
	xmlDoc  *config;
	xmlDoc  *original_config;

	GtkIconTheme *icon_theme;
	GstDialog *main_dialog;

	/* Remote configuration stuff */
	gboolean remote_config;
	gchar **remote_hosts;

	/* id of the g_timeout that is waiting for the backend to die */
	guint timeout_id;

	/* Platform selection */
	GtkWidget *platform_dialog;
	GtkWidget *platform_list;
	GtkWidget *platform_ok_button;

	/* Remote configuration dialog */
	GtkWidget *remote_dialog;
	GtkWidget *remote_hosts_list;

	/* Progress report widgets */
	GtkWidget *report_window;
	GtkWidget *report_label;
	GtkWidget *report_progress;
	GtkWidget *report_pixmap;
	guint      report_timeout_id;
	guint      report_animate_id;

	GString *line;
	GString *xml_document;
	
	guint     input_id;
	gboolean  input_block;
	GSList   *report_line_list;
	gboolean  report_dispatch_pending;
	gboolean  report_finished;
	gboolean  run_again;
	
	gboolean  directive_running; /* locked when a directive is running */
	GSList   *directive_queue;
	guint     directive_queue_idle_id;

	GstReportHookType  report_hook_type;
	GSList            *report_hook_list;
	GstReportHook     *report_hook_defaults[GST_MAJOR_MAX];

	GstPlatform *current_platform;         /* Always set from backend report  */
	GSList      *supported_platforms_list; /* Gets set only if backend breaks */
};

struct _GstToolClass {
	GObjectClass parent_class;

	void (*fill_gui) (GstTool *xt);
	void (*fill_xml) (GstTool *xt);
	void (*close)    (GstTool *xt);
};

GtkType      gst_tool_get_type            (void);


void         gst_init                     (const gchar *app_name,
					   int argc, char *argv [],
					   GOptionEntry *entries);

void         gst_tool_main                (GstTool *tool, gboolean no_main_loop);
void         gst_tool_main_with_hidden_dialog (GstTool *tool, gboolean no_main_loop);
gboolean     gst_tool_get_access          (GstTool *tool);

GstTool     *gst_tool_new                 (void);
void         gst_tool_construct           (GstTool *tool, 
					   const char *name, const char *title);

gboolean     gst_tool_save                (GstTool*, gboolean);
void         gst_tool_save_cb             (GtkWidget *w, GstTool *tool);
gboolean     gst_tool_load                (GstTool *tool);
void         gst_tool_load_try            (GstTool *tool);


/* All undefined arguments in these directive functions must be (const gchar *) type.
   They will become arguments in the directive line passed to the backend. Last arg must be NULL. */
xmlDoc      *gst_tool_run_get_directive   (GstTool *tool, const gchar *report_sign,
					   const gchar *directive, ...);

/* xml can be NULL, in which case no XML will be sent to the backend. */
xmlDoc      *gst_tool_run_set_directive   (GstTool *tool, xmlDoc *xml,
					   const gchar *report_sign, const gchar *directive, ...);

/* This is for async directive calls. See .c file for directions. */
void         gst_tool_queue_directive     (GstTool *tool, GstDirectiveFunc callback, gpointer data,
				           xmlDoc *in_xml, gchar *report_sign, gchar *directive);

void         gst_tool_set_xml_funcs       (GstTool *tool, GstXmlFunc load_cb, GstXmlFunc save_cb, gpointer data);
void         gst_tool_set_close_func      (GstTool *tool, GstCloseFunc close_cb, gpointer data);

GladeXML    *gst_tool_load_glade_common   (GstTool *tool, const gchar *widget);
GladeXML    *gst_tool_load_glade          (GstTool *tool, const gchar *widget);

GstDialog   *gst_tool_get_dialog          (GstTool *tool);

void         gst_tool_add_report_hooks    (GstTool *tool, GstReportHookEntry *report_hook_table);
void         gst_tool_set_default_hook    (GstTool *tool, GstReportHookEntry *entry, GstReportMajor major);
void         gst_tool_invoke_report_hooks (GstTool *tool, GstReportHookType type, GstReportLine *rline);
void         gst_tool_reset_report_hooks  (GstTool *tool);

void         gst_tool_add_supported_platform    (GstTool *tool, GstPlatform *platform);
void         gst_tool_clear_supported_platforms (GstTool *tool);

void         gst_tool_process_startup (GstTool*);

gchar*       gst_tool_read_from_backend (GstTool*, gchar*, ...);
void         gst_tool_read_junk_from_backend (GstTool*, gchar*);
void         gst_tool_write_to_backend (GstTool*, gchar*);
void         gst_tool_write_xml_to_backend (GstTool*, xmlDoc*);
void         gst_tool_show_help (GstTool*, gchar*);

#define __full_tool_name(parameter) "Gst" #parameter "Tool"

/*
 * Handy define to reduce the boilerplate code
 */
#define GST_TOOL_MAKE_TYPE(name, Name) \
\
static GstToolClass *gst_tool_parent_class = NULL; \
\
static void \
gst_foo_tool_class_init (GtkObjectClass *object_class)\
{\
/*	GstIshareToolClass *ishare_class = GST_ISHARE_TOOL_CLASS (object_class);\
 */\
\
	gst_tool_parent_class = gtk_type_class (gst_tool_get_type ());\
}\
\
static void gst_ ## name ## _tool_type_init (Gst ## Name ## Tool *tool);\
/*static void gst_ ## name ## _tool_class_init (GtkObjectClass *object_class);*/\
\
GtkType \
gst_ ## name ## _tool_get_type (void)\
{\
	static GType type = 0;\
	if (!type) {\
		GTypeInfo info = {\
			sizeof (Gst ## Name ## ToolClass),\
			NULL,\
			NULL,\
			(GClassInitFunc)  gst_foo_tool_class_init,\
			NULL,\
			NULL,\
			sizeof (Gst ## Name ## Tool),\
			0,\
			(GInstanceInitFunc) gst_ ## name ## _tool_type_init,\
		};\
		\
		type = g_type_register_static (GST_TYPE_TOOL, __full_tool_name (Name), &info, 0);\
	}\
	return type;\
}

#endif /* GST_TOOL_H */
