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
 * Authors: Hans Petter Jansson <hpj@helixcode.com> and Arturo Espinosa <arturo@helixcode.com>.
 */

#include <gnome.h>
#include <tree.h>

typedef enum {
	EF_ALLOW_NONE  = 0,
	EF_ALLOW_TEXT  = 1,
	EF_ALLOW_ENTER = 2,
	EF_STATIC_HOST = 4,
	EF_ALLOW_SPACE = 8
} EditableFilterRules;

extern xmlDocPtr doc;

void init_hint_entries (void);
void on_network_admin_show (GtkWidget *w, gpointer null);

extern void on_network_notebook_switch_page (GtkWidget *notebook, 
					     GtkNotebookPage *page,
					     gint page_num, gpointer user_data);

void filter_editable (GtkEditable *e, const gchar *text, 
		      gint length, gint *pos, gpointer data);

#define connect_editable_filter(w, r) gtk_signal_connect (GTK_OBJECT (w), "insert_text", \
							   GTK_SIGNAL_FUNC (filter_editable), \
							   GINT_TO_POINTER (r))
