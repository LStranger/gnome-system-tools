/* Copyright (C) 2000 Helix Code, Inc.
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
 * Authors: Tambet Ingo <tambeti@sa.ee>.
 */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gnome.h>
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glade/glade.h>

#include "global.h"

#include "transfer.h"
#include "callbacks.h"


void delete_event (GtkWidget * widget, GdkEvent * event, gpointer gdata);


void
delete_event (GtkWidget * widget, GdkEvent * event, gpointer gdata)
{
	gtk_main_quit ();
}


int
main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);
#endif

	tool_init("users", argc, argv);

	tool_set_frozen(TRUE);
	transfer_xml_to_gui (xml_doc_get_root (tool_config_get_xml()));
	tool_set_frozen(FALSE);

  
	gtk_widget_show (tool_get_top_window ());
	gtk_main ();

	return 0;
}
