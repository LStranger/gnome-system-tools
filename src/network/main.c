/* Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Hans Petter Jansson <hpj@ximian.com>.
 */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <glade/glade.h>

#include "global.h"

#include "transfer.h"
#include "callbacks.h"

int
main (int argc, char *argv[])
{
	int i;
	char *s[] = { 
		"wins_ip",
		"dns_list",
		"search_list",
		"ip",
		"alias",
		NULL
	};
	EditableFilterRules e[] = {
		EF_ALLOW_NONE,
		EF_ALLOW_ENTER,
		EF_ALLOW_ENTER | EF_ALLOW_TEXT| EF_ALLOW_SPACE,
		EF_STATIC_HOST,
		EF_STATIC_HOST | EF_ALLOW_ENTER | EF_ALLOW_TEXT | EF_ALLOW_SPACE
	};
	
	init_hint_entries ();
	
	tool_init ("network", argc, argv);
	tool_set_xml_funcs (transfer_xml_to_gui, transfer_gui_to_xml);

	init_icons ();

	for (i=0; s[i]; i++)
		connect_editable_filter (tool_widget_get (s[i]), e[i]);

	tool_set_frozen (TRUE);
	transfer_xml_to_gui (xml_doc_get_root (tool_config_get_xml()));
	tool_set_frozen (FALSE);

	gtk_widget_show (tool_get_top_window ());
	/*add_connections_to_list ();*/
	
	on_network_admin_show (NULL, NULL);

	gtk_main ();

	return 0;
}
