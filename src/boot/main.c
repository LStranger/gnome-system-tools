/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* main.c: this file is part of boot-admin, a ximian-setup-tool frontend 
 * for boot administration.
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
 * Authors: Tambet Ingo <tambet@ximian.com>.
 */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glade/glade.h>

#include <stdlib.h>

#include "xst.h"
#include "xst-report-hook.h"

#include "transfer.h"
#include "e-table.h"
#include "callbacks.h"
#include "boot-settings.h"

XstTool *tool;

static void
connect_signals (XstTool *tool)
{
	XstDialogSignal signals[] = {
		{ "boot_delete",   "clicked", G_CALLBACK (on_boot_delete_clicked) },
		{ "boot_settings", "clicked", G_CALLBACK (on_boot_settings_clicked) },
		{ "boot_add",      "clicked", G_CALLBACK (on_boot_add_clicked) },
		{ "boot_default",  "clicked", G_CALLBACK (on_boot_default_clicked) },
		{ "boot_prompt",   "toggled", G_CALLBACK (on_boot_prompt_toggled) },
		{ "boot_timeout",  "changed", G_CALLBACK (xst_dialog_modify_cb) },
		{ NULL }
	};

	g_signal_connect (G_OBJECT (tool->main_dialog), "complexity_change",
			  G_CALLBACK (on_main_dialog_update_complexity), tool);

	xst_dialog_connect_signals (tool->main_dialog, signals);
}

int
main (int argc, char *argv[])
{
	XstReportHookEntry report_hooks[] = {
		{ "boot_conf_read_failed", callbacks_conf_read_failed_hook,
		  XST_REPORT_HOOK_LOAD, FALSE, NULL },
		{ NULL, NULL, -1, FALSE, NULL }
	};

	xst_init ("boot-admin", argc, argv, NULL);
	tool = xst_tool_new ();
	xst_tool_construct (tool, "boot", _("Boot Manager Settings"));
	table_construct (tool);

	xst_tool_set_xml_funcs    (tool, transfer_xml_to_gui, transfer_gui_to_xml, NULL);
	xst_tool_add_report_hooks (tool, report_hooks);

	connect_signals (tool);

	xst_dialog_enable_complexity (tool->main_dialog);
	on_main_dialog_update_complexity (tool->main_dialog, tool);

	xst_tool_main (tool, FALSE);

	return 0;
}
