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
 * Authors: Jacob Berkman   <jacob@ximian.com>
 *          Carlos Garnacho <carlosg@gnome.org>
 */

#ifndef __GST_TOOL_H
#define __GST_TOOL_H

G_BEGIN_DECLS

#include <glade/glade.h>
#include <gtk/gtk.h>
#include <oobs/oobs.h>
#include <gconf/gconf-client.h>

#include "gst-types.h"

#define GST_TYPE_TOOL         (gst_tool_get_type ())
#define GST_TOOL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o),  GST_TYPE_TOOL, GstTool))
#define GST_TOOL_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),     GST_TYPE_TOOL, GstToolClass))
#define GST_IS_TOOL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o),  GST_TYPE_TOOL))
#define GST_IS_TOOL_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),     GST_TYPE_TOOL))
#define GST_TOOL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),   GST_TYPE_TOOL, GstToolClass))

typedef struct _GstTool      GstTool;
typedef struct _GstToolClass GstToolClass;

struct _GstTool {
	GObject object;

	gchar *name;
	gchar *title;
	gchar *icon;

	OobsSession *session;
	GConfClient *gconf_client;

	char *glade_path;
	char *glade_common_path;

	GtkIconTheme *icon_theme;
	GstDialog *main_dialog;

	/* Remote configuration stuff */
	/*
	gboolean remote_config;
	gchar **remote_hosts;
	*/

	/* Platform selection */
	GtkWidget *platform_dialog;
	GtkWidget *platform_list;
	GtkWidget *platform_ok_button;

	/* Remote configuration dialog */
	/*
	GtkWidget *remote_dialog;
	GtkWidget *remote_hosts_list;
	*/

	/* Progress report widgets */
	GtkWidget *report_window;
	GtkWidget *report_label;
	GtkWidget *report_progress;
	GtkWidget *report_pixmap;
	guint      report_timeout_id;
	guint      report_animate_id;

	GstPlatform *current_platform;         /* Always set from backend report  */
	GSList      *supported_platforms_list; /* Gets set only if backend breaks */
};

struct _GstToolClass {
	GObjectClass parent_class;

	void (*close)         (GstTool *tool);

	/* virtual methods */
	void (*update_gui)    (GstTool *tool);
	void (*update_config) (GstTool *tool);
};


GType        gst_tool_get_type            (void);

void         gst_init                     (const gchar *app_name,
					   int argc, char *argv [],
					   GOptionEntry *entries);

gboolean     gst_tool_is_authenticated    (GstTool *tool);

gboolean     gst_tool_save                (GstTool *tool);

void         gst_tool_update_gui          (GstTool *tool);
void         gst_tool_update_config       (GstTool *tool);
void         gst_tool_close               (GstTool *tool);


/* FIXME: is it needed ?
gboolean     gst_tool_load                (GstTool *tool);
void         gst_tool_load_try            (GstTool *tool);
*/

GladeXML    *gst_tool_load_glade_common   (GstTool *tool, const gchar *widget);
GladeXML    *gst_tool_load_glade          (GstTool *tool, const gchar *widget);

GstDialog   *gst_tool_get_dialog          (GstTool *tool);

GtkIconTheme *gst_tool_get_icon_theme     (GstTool *tool);


/* FIXME: is it needed?
void         gst_tool_add_supported_platform    (GstTool *tool, GstPlatform *platform);
void         gst_tool_clear_supported_platforms (GstTool *tool);
*/

void         gst_tool_process_startup (GstTool*);

void         gst_tool_show_help       (GstTool*, gchar*);

void         gst_tool_commit_async    (GstTool     *tool,
				       OobsObject  *object,
				       const gchar *message);



G_END_DECLS

#endif /* __GST_TOOL_H */
