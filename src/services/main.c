/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* main.c: this file is part of services-admin, a gnome-system-tool frontend 
 * for run level services administration.
 * 
 * Copyright (C) 2002 Ximian, Inc.
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
 * Authors: Carlos Garnacho Parro <garnacho@tuxerver.net>.
 */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

/* #include <stdlib.h> */
#include "transfer.h"
#include "table.h"
#include "gst.h"
#include "gst-report-hook.h"
#include "callbacks.h"

GstTool *tool;

static GstDialogSignal signals [] = {
	{ "settings_button", "clicked", G_CALLBACK (on_settings_button_clicked) },
	{ "dialog_service_priority", "value-changed", G_CALLBACK (on_service_priority_changed) },
	{ "sequence_ordering", "clicked", G_CALLBACK (on_sequence_ordering_changed) },
	{ NULL }
};

static GstWidgetPolicy policies [] = {
	/* Name                     Basic                        Advanced                   Root   User */
	{ "settings_button",        GST_WIDGET_MODE_HIDDEN,      GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "runlevels_menu",         GST_WIDGET_MODE_HIDDEN,      GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE },
	{ "sequence_ordering",      GST_WIDGET_MODE_HIDDEN,      GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE },
	{NULL}
};

int 
main (int argc, char *argv[])
{
	gst_init ("services-admin", argc, argv, NULL);
	
	tool = gst_tool_new();
	gst_tool_construct (tool, "services", _("Services Settings"));
	table_create ();

	gst_dialog_set_widget_policies (tool->main_dialog, policies);
	gst_tool_set_xml_funcs (tool, transfer_xml_to_gui, NULL, NULL);
	gst_dialog_connect_signals (tool->main_dialog, signals);
	
	gst_dialog_enable_complexity (tool->main_dialog);

	gst_tool_main (tool,FALSE);
	
	return 0;
}
