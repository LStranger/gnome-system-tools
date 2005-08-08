/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* main.c: this file is part of disks-admin, a gnome-system-tool frontend 
 * for disks administration.
 * 
 * Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Alvaro Peña Gonzalez <apg@esware.com>
 *          Carlos García Campos <elkalmail@yahoo.es>
 */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib/gi18n.h>

#include "gst-report-hook.h"
#include "gst-disks-tool.h"

#include "callbacks.h"
#include "transfer.h"

GstTool *tool;

static void
connect_signals (GstTool *tool)
{
	GstDialogSignal signals[] = {
		{ NULL }
	};

	/*g_signal_connect (G_OBJECT (tool->main_dialog), "complexity_change",
	  G_CALLBACK (on_main_dialog_update_complexity), tool);*/

	gst_dialog_connect_signals (tool->main_dialog, signals);
}

gint
main (gint argc, gchar *argv[])
{
	GstReportHookEntry report_hooks[] = {
		{ NULL, NULL, -1, FALSE, NULL }
	};

	gst_init ("disks-admin", argc, argv, NULL);

	tool = gst_disks_tool_new ();
	gst_tool_construct (tool, "disks", _("Disks Manager"));

	gst_tool_set_xml_funcs (tool, transfer_xml_to_gui, transfer_gui_to_xml, NULL);
	gst_tool_add_report_hooks (tool, report_hooks);

	connect_signals (tool);

	/* gst_dialog_enable_complexity (tool->main_dialog); */
	/* on_main_dialog_update_complexity (tool->main_dialog, tool); */

	/* gtk_widget_hide (gst_dialog_get_widget (tool->main_dialog, "hbox_properties")); */
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (
					    gst_dialog_get_widget (tool->main_dialog,
								   "properties_notebook")),
				    FALSE);
									

	gst_tool_main (tool, FALSE);

	return 0;
}
