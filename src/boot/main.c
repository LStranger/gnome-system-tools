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
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glade/glade.h>

#include <stdlib.h>

#include "global.h"

#include "transfer.h"
#include "e-table.h"
#include "callbacks.h"

XstTool *tool;

static XstDialogSignal signals[] = {
	{ "boot_delete",   "clicked", on_boot_delete_clicked },
	{ "boot_settings", "clicked", on_boot_settings_clicked },
	{ "boot_add",      "clicked", on_boot_add_clicked },
	{ "boot_default",  "clicked", on_boot_default_clicked },
	{ "boot_prompt",   "toggled", on_boot_prompt_toggled },
	{ "boot_timeout",  "changed", xst_dialog_modify_cb },
	{ NULL }
};

static void set_access_sensitivity (void)
{
}

static void
update_complexity ()
{
	XstDialogComplexity complexity;

	complexity = tool->main_dialog->complexity;

	boot_table_update_state ();

	if (complexity == XST_DIALOG_BASIC)
	{
		gtk_widget_hide (xst_dialog_get_widget (tool->main_dialog, "boot_add"));
		gtk_widget_hide (xst_dialog_get_widget (tool->main_dialog, "boot_delete"));
	}

	else
	{
		gtk_widget_show (xst_dialog_get_widget (tool->main_dialog, "boot_add"));
		gtk_widget_show (xst_dialog_get_widget (tool->main_dialog, "boot_delete"));
	}
}

static void
connect_signals ()
{
	gtk_signal_connect (GTK_OBJECT (tool), "fill_gui",
					GTK_SIGNAL_FUNC (transfer_xml_to_gui),
					NULL);

	gtk_signal_connect (GTK_OBJECT (tool), "fill_xml",
					GTK_SIGNAL_FUNC (transfer_gui_to_xml),
					NULL);

	gtk_signal_connect (GTK_OBJECT (tool->main_dialog), "complexity_change",
					GTK_SIGNAL_FUNC (update_complexity),
					NULL);

	xst_dialog_connect_signals (tool->main_dialog, signals);
}

int
main (int argc, char *argv[])
{
	tool = xst_tool_init ("boot", _("Boot Manager Settings - Ximian Setup Tools"), argc, argv, NULL);

	xst_dialog_freeze (tool->main_dialog);

	connect_signals ();

	xst_dialog_enable_complexity (tool->main_dialog);

	xst_dialog_thaw (tool->main_dialog);
	
	xst_tool_main (tool);
	
	return 0;
}
