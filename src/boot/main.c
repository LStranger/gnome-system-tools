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

static void set_access_sensitivity (void)
{
}

static void
update_complexity ()
{
	XstDialogComplexity complexity;

	complexity = tool->main_dialog->complexity;
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

	gtk_signal_connect_object (GTK_OBJECT (tool->main_dialog), "apply",
						  GTK_SIGNAL_FUNC (xst_tool_save),
						  GTK_OBJECT (tool));

	gtk_signal_connect (GTK_OBJECT (tool->main_dialog), "complexity_change",
					GTK_SIGNAL_FUNC (update_complexity),
					NULL);

	
	/* Why not in xst_dialog ? */
	glade_xml_signal_autoconnect (tool->main_dialog->gui);
}

int
main (int argc, char *argv[])
{
	tool = xst_tool_init ("boot", _("Boot Manager Settings - Ximian Setup Tools"), argc, argv);

	xst_dialog_freeze (tool->main_dialog);

	connect_signals ();

	/* Not yet */
/*	xst_dialog_enable_complexity (tool->main_dialog); */

	xst_dialog_thaw (tool->main_dialog);
	
	xst_tool_main (tool);
	
	return 0;
}
