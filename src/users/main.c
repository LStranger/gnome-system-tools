/* main.c: this file is part of users-admin, a helix-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2000 Helix Code, Inc.
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
 * Authors: Tambet Ingo <tambeti@sa.ee> and Arturo Espinosa <arturo@helixcode.com>.
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

#include <time.h>
#include <stdlib.h>

#include "global.h"

#include "transfer.h"
#include "e-table.h"
#include "callbacks.h"

static void set_access_sensitivity (void)
{
	char *access_no[] = {"user_new", "user_chpasswd", "group_new", 
		                   "user_settings_basic", "user_settings_advanced",
		                   "group_settings_name_label", "group_settings_add", 
		                   "group_settings_remove", NULL};
	char *access_yes[] = {"users_holder", "groups_holder", NULL};
	char *unsensitive[] = {"user_delete", "user_settings", "user_chpasswd", "group_delete",
		                     "group_settings", NULL};
	int i;
	
	/* Those widgets that won't be available if you don't have the access. */
	for (i = 0; access_no[i]; i++)
		gtk_widget_set_sensitive (tool_widget_get (access_no[i]), tool_get_access());
	
	/* Those widgets that will be available, even if you don't have the access. */
	for (i = 0; access_yes[i]; i++)
		gtk_widget_set_sensitive (tool_widget_get (access_yes[i]), TRUE);
	
	/* Those widgets you should never have access to, and will be activated later on. */
	for (i = 0; unsensitive[i]; i++)
		gtk_widget_set_sensitive (tool_widget_get (unsensitive[i]), FALSE);
}

int
main (int argc, char *argv[])
{
	
	/* For random password generation. */
	
	srand (time (NULL));
	
	tool_init("users", argc, argv);

	tool_set_frozen(TRUE);
	transfer_xml_to_gui (xml_doc_get_root (tool_config_get_xml()));
	e_table_create ();
	tool_set_frozen(FALSE);

  
	gtk_widget_show (tool_get_top_window ());
	set_access_sensitivity ();
	gtk_main ();

	return 0;
}
