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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>

#include "global.h"

#include "callbacks.h"
#include "transfer.h"

#define d(x) x

static int reply;
GtkWidget *searchdomain_entry_selected = NULL;
GtkWidget *aliases_settings_entry_selected = NULL;
GtkWidget *dns_entry_selected = NULL;
int statichost_row_selected = -1;

#define SEARCHDOMAIN_ROW_SELECTED gtk_list_child_position (GTK_LIST (tool_widget_get ("searchdomain_list")), searchdomain_entry_selected)
#define ALIASES_SETTINGS_ROW_SELECTED gtk_list_child_position (GTK_LIST (tool_widget_get ("aliases_settings_list")), aliases_settings_entry_selected)
#define DNS_ROW_SELECTED gtk_list_child_position (GTK_LIST (tool_widget_get ("dns_list")), dns_entry_selected)


/* --- helpers */

static void reply_cb (gint val, gpointer data);
static void searchdomain_actions_set_sensitive (gboolean state);
static void statichost_actions_set_sensitive (gboolean state);
static void aliases_settings_actions_set_sensitive (gboolean state);
static void listitem_children_swap (GtkWidget *src, GtkWidget *dest);
static void searchdomain_entry_add (void);
static void aliases_settings_dialog_close (void);
static void aliases_settings_list_fill (void);
static void aliases_settings_entry_add (void);
static void scrolled_window_scroll_bottom (GtkWidget *sw);
static GtkWidget *list_delete_entry (GtkList *list, GtkWidget *entry);

extern void 
on_network_admin_show (GtkWidget *w, gpointer user_data)
{
	char *access_no[] = { 
		"general_vbox",
		"connections_bbox",
		"dns_vbox",
		"statichost_table",
		NULL };

	char *access_yes[] = {
		"dns_list", 
		"search_list", 
		NULL} ;

	char *unsensitive[] = {
		"connection_delete",
		"connection_configure",
		"statichost_add",
		"statichost_update",
		"statichost_delete",
		NULL };
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
	
	gtk_clist_set_column_auto_resize (GTK_CLIST (tool_widget_get ("statichost_list")), 0, TRUE);
	gtk_clist_set_column_auto_resize (GTK_CLIST (tool_widget_get ("statichost_list")), 1, TRUE);
}

extern void
on_network_notebook_switch_page (GtkWidget *notebook, GtkNotebookPage *page,
																				gint page_num, gpointer user_data)
{
	gchar *entry[] = { "hostname", "connection_list", "dns_dhcp", "statichost_list" };
	
	if (tool_get_access () && entry[page_num])
		gtk_widget_grab_focus (tool_widget_get (entry[page_num]));
}

#if 0
static void
dns_ip_add (void)
{
	gchar *txt;
	GtkWindow *parent;
	GnomeDialog *dialog;
	
	parent = GTK_WINDOW (tool_widget_get ("nameresolution_admin"));

	if (!list_add_ip (GTK_LIST (tool_widget_get ("dns_list")), tool_widget_get ("dns_ip")))
	{
		txt = _("The IP address must have the form of four digits\nsepparated by a dot (.), being these digits in the range of 0 to 255,\nexcept for the first and last ones, which must be in the range of 1 to 254.");
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (txt, parent));
		gnome_dialog_run (dialog);
	}
	
	scrolled_window_scroll_bottom (tool_widget_get ("dns_sw"));
}


extern void
on_dns_list_select_child (GtkList * list, GtkWidget * widget, gpointer user_data)
{
	dns_entry_selected = widget;
	gtk_widget_set_sensitive (tool_widget_get ("dns_delete"), tool_get_access ());
}


extern void
on_dns_list_unselect_child (GtkList * list, GtkWidget * widget, gpointer user_data)
{
	dns_entry_selected = NULL;
	gtk_widget_set_sensitive (tool_widget_get ("dns_delete"), FALSE);
}


extern void
on_dns_ip_activate (GtkEditable * editable, gpointer user_data)
{
	char *ip;
	
	ip = gtk_editable_get_chars (editable, 0, -1);
	if (strlen (ip) > 0)
		dns_ip_add ();
	g_free (ip);
}


extern void
on_dns_ip_add_clicked (GtkButton * button, gpointer user_data)
{
	dns_ip_add ();
}


extern void
on_dns_delete_clicked (GtkButton * button, gpointer user_data)
{
	g_return_if_fail (tool_get_access ());

	dns_entry_selected = list_delete_entry (GTK_LIST (tool_widget_get ("dns_list")), 
						dns_entry_selected);
}

extern void
on_wins_use_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	gtk_widget_set_sensitive (tool_widget_get ("wins_ip"),
				  gtk_toggle_button_get_active (togglebutton));
}


static void
searchdomain_entry_add (void)
{
	GtkWidget *item, *editable, *list;
	GList *list_add = NULL;
	gchar *text, *c, c2;
	gint max;
	
	g_return_if_fail (tool_get_access ());
	
	editable = tool_widget_get ("searchdomain");
	
	text = gtk_editable_get_chars (GTK_EDITABLE (editable), 0, -1);
	g_strstrip (text);
	g_return_if_fail (strlen (text));
	
	for (c2 = '.', c = text; *c; c2 = *c, c++)
		if (! ((*c >= 'a' && *c <= 'z') ||
					 (*c >= 'A' && *c <= 'Z') ||
					 (*c >= '0' && *c <= '9') ||
					 (*c == '.' && c2 != '.')))
		{
			GnomeDialog *dialog;
		  GtkWindow *parent;
			gchar *txt;
			
			parent = GTK_WINDOW (tool_widget_get ("nameresolution_admin"));
			txt = _("The domain may only contain letters, numbers and dots (.),\nwhich may work as sub-domain separators.");
			dialog = GNOME_DIALOG (gnome_error_dialog_parented (txt, parent));
			gnome_dialog_run (dialog);
			
			return;
		}

	gtk_editable_delete_text (GTK_EDITABLE (editable), 0, -1);

	list = tool_widget_get ("searchdomain_list");
	max = (gint) gtk_object_get_data (GTK_OBJECT (list), "max");
	gtk_object_set_data (GTK_OBJECT (list), "max", (gpointer) ++max);
											 
	item = gtk_list_item_new_with_label (text);
	gtk_widget_show (item);
	list_add = g_list_append (list_add, item);
	gtk_list_append_items (GTK_LIST (list), list_add);
	gtk_list_select_child (GTK_LIST (list), item);
	
	tool_set_modified (TRUE);
	
	scrolled_window_scroll_bottom (tool_widget_get ("searchdomain_sw"));
	
	gtk_widget_grab_focus (GTK_WIDGET (editable));
}


extern void
on_searchdomain_list_select_child (GtkList * list, GtkWidget * widget, gpointer user_data)
{
	searchdomain_entry_selected = widget;
	searchdomain_actions_set_sensitive (TRUE);
}


extern void
on_searchdomain_list_unselect_child (GtkList * list, GtkWidget * widget, gpointer user_data)
{
	searchdomain_entry_selected = NULL;
	searchdomain_actions_set_sensitive (FALSE);
}


extern void
on_searchdomain_activate (GtkEditable * editable, gpointer user_data)
{
	char *domain;
	
	domain = gtk_editable_get_chars (editable, 0, -1);
	if (strlen (domain) > 0)
		searchdomain_entry_add ();
	g_free (domain);
}


extern void
on_searchdomain_add_clicked (GtkButton * button, gpointer user_data)
{
	searchdomain_entry_add ();
}


extern void
on_searchdomain_delete_clicked (GtkButton * button, gpointer user_data)
{
	g_return_if_fail (tool_get_access ());

	searchdomain_entry_selected = list_delete_entry (GTK_LIST (tool_widget_get ("searchdomain_list")),
																									 searchdomain_entry_selected);
}


extern void
on_searchdomain_move_up_clicked        (GtkButton       *button,
																				gpointer         user_data)
{
	GtkList *w;
	GtkWidget *src, *dest;
	
	g_return_if_fail (tool_get_access ());
	g_return_if_fail (SEARCHDOMAIN_ROW_SELECTED != -1);
	g_return_if_fail (SEARCHDOMAIN_ROW_SELECTED != 0);
	
	w = GTK_LIST (tool_widget_get ("searchdomain_list"));

	src = g_list_nth (w->children, SEARCHDOMAIN_ROW_SELECTED)->data;
	dest = g_list_nth (w->children, SEARCHDOMAIN_ROW_SELECTED - 1)->data;
		
	listitem_children_swap (src, dest);
	gtk_list_select_child (w, dest);
	tool_set_modified (TRUE);
}


extern void
on_searchdomain_move_down_clicked      (GtkButton       *button,
																				gpointer         user_data)
{
	GtkList *w;
	GtkWidget *src, *dest;
	gint max;
	
	w = GTK_LIST (tool_widget_get ("searchdomain_list"));
	max = (gint) gtk_object_get_data (GTK_OBJECT (w), "max");
	
	g_return_if_fail (tool_get_access ());
	g_return_if_fail (SEARCHDOMAIN_ROW_SELECTED != -1);
	g_return_if_fail (SEARCHDOMAIN_ROW_SELECTED != max - 1);

	src = g_list_nth (w->children, SEARCHDOMAIN_ROW_SELECTED)->data;
	dest = g_list_nth (w->children, SEARCHDOMAIN_ROW_SELECTED + 1)->data;
		
	listitem_children_swap (src, dest);
	gtk_list_select_child (w, dest);
	tool_set_modified (TRUE);
}
#endif

extern void
on_statichost_list_select_row (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer user_data)
{
	gchar *row_data;
	gchar *label;
	GtkWidget *w;
	gint pos = 0;

	statichost_row_selected = row;
	statichost_actions_set_sensitive (TRUE);
	
	/* Load aliases into entry widget */

	pos = 0;
	gtk_clist_get_text (GTK_CLIST (tool_widget_get ("statichost_list")), row, 1, &row_data);
	w = tool_widget_get ("ip");
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
	gtk_editable_insert_text (GTK_EDITABLE (w), row_data, strlen (row_data), &pos);

	pos = 0;
	gtk_clist_get_text (GTK_CLIST (tool_widget_get ("statichost_list")), row, 2, &row_data);
	w = tool_widget_get ("alias");
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
	gtk_editable_insert_text (GTK_EDITABLE (w), row_data, strlen (row_data), &pos);
}


extern void
on_statichost_list_unselect_row (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer user_data)
{
	GtkWidget *w;

	statichost_row_selected = -1;
	statichost_actions_set_sensitive (FALSE);
	
	w = tool_widget_get ("ip");
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);

	w = tool_widget_get ("alias");
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
}

extern void
on_statichost_changed (GtkWidget *w, gpointer null)
{
	gboolean enabled;
	char *ip;

	enabled = tool_get_access () &&
		gtk_text_get_length (GTK_TEXT (tool_widget_get ("alias"))) &&
		check_ip_entry (GTK_ENTRY (tool_widget_get ("ip")), FALSE);
			
	gtk_widget_set_sensitive (tool_widget_get ("statichost_add"), enabled);
	gtk_widget_set_sensitive (tool_widget_get ("statichost_update"), enabled && 
				  GTK_WIDGET_SENSITIVE (tool_widget_get ("statichost_delete")));
}

extern void
on_statichost_add_clicked (GtkButton * button, gpointer user_data)
{
	GtkWidget *clist;
	char *entry[4];
	int row;

	g_return_if_fail (tool_get_access());

	entry[0] = entry[3] = NULL;
	
	entry[1] = gtk_editable_get_chars (
		GTK_EDITABLE (tool_widget_get ("ip")), 0, -1);

#warning FIXME: clean up this next string
	entry[2] = gtk_editable_get_chars (
		GTK_EDITABLE (tool_widget_get ("alias")), 0, -1);
		
	clist = tool_widget_get ("statichost_list");

	row = gtk_clist_append (GTK_CLIST (clist), entry);
		

	set_clist_checkmark (GTK_CLIST (clist), row, 0, TRUE);
}


extern void
on_statichost_delete_clicked (GtkButton * button, gpointer user_data)
{
	gchar *txt, *name;
	GtkWindow *parent;
	GnomeDialog *dialog;

	g_return_if_fail (tool_get_access());
	g_return_if_fail (statichost_row_selected != -1);

	parent = tool_get_top_window ();
	gtk_clist_get_text (GTK_CLIST (tool_widget_get ("statichost_list")),
			    statichost_row_selected, 2, &name);

	txt = g_strdup_printf (_("Are you sure you want to delete the aliases for %s?"), name);
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (txt, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);
	g_free (txt);

	if (reply) return;
		
	gtk_clist_remove (GTK_CLIST (tool_widget_get ("statichost_list")), statichost_row_selected);
	tool_set_modified (TRUE);
	statichost_actions_set_sensitive (FALSE);
}

void
on_statichost_update_clicked (GtkWidget *w, gpointer null)
{
	GtkWidget *clist;

	g_return_if_fail (tool_get_access());

	clist = tool_widget_get ("statichost_list");

	gtk_clist_set_text (GTK_CLIST (clist), statichost_row_selected, 1,
			    gtk_editable_get_chars (
				    GTK_EDITABLE (tool_widget_get ("ip")), 0, -1));
	
#warning FIXME: clean up this next string
	gtk_clist_set_text (GTK_CLIST (clist), statichost_row_selected, 2,
			    gtk_editable_get_chars (
				    GTK_EDITABLE (tool_widget_get ("alias")), 0, -1));

	set_clist_checkmark (GTK_CLIST (clist), statichost_row_selected, 0, TRUE);
}

#if 0
extern void
on_statichost_settings_clicked (GtkButton * button, gpointer user_data)
{
	GtkWidget *w0;
	gchar *txt, *ip;
	
	g_return_if_fail (statichost_row_selected != -1);

	w0 = tool_widget_get ("aliases_settings_ip");
	gtk_clist_get_text (GTK_CLIST (tool_widget_get ("statichost_list")), 
											statichost_row_selected, 0, &ip);
	gtk_entry_set_text (GTK_ENTRY (w0), ip);

	/* Show alias settings dialog */
	
	w0 = tool_widget_get ("aliases_settings_dialog");
	txt = g_strdup_printf (_("Settings for Alias %s"), ip);
	gtk_window_set_title (GTK_WINDOW (w0), txt);
	g_free (txt);
	gtk_widget_set_sensitive (tool_widget_get ("aliases_settings_ok"), FALSE);
	gtk_widget_show (w0);
	
	/* Fill aliases list */
	aliases_settings_list_fill ();
}


extern gboolean
on_aliases_settings_dialog_delete_event  (GtkWidget       *widget,
                                          GdkEvent        *event,
                                          gpointer         user_data)
{
	aliases_settings_dialog_close ();
	return TRUE;
}


extern void
on_aliases_settings_dialog_show (GtkWidget *w, gpointer user_data)
{
	if (tool_get_access ())
		gtk_widget_grab_focus (tool_widget_get ("aliases_settings_new"));
}

extern void
on_aliases_settings_ip_changed (GtkWidget *w, gpointer user_data)
{
	gtk_widget_set_sensitive (tool_widget_get ("aliases_settings_ok"), TRUE);
}


extern void
on_aliases_settings_list_select_child (GtkList *list, GtkWidget *widget, gpointer user_data)
{
	aliases_settings_entry_selected = widget;
	aliases_settings_actions_set_sensitive (TRUE);
}

extern void
on_aliases_settings_list_unselect_child (GtkList *list, GtkWidget *widget, gpointer user_data)
{
	aliases_settings_entry_selected = NULL;
	aliases_settings_actions_set_sensitive (FALSE);
}


extern void
on_aliases_settings_new_activate (GtkEditable *editable, gpointer user_data)
{
	char *new_alias;
	
	new_alias = gtk_editable_get_chars (editable, 0, -1);
	if (strlen (new_alias) > 0)
		aliases_settings_entry_add ();
	g_free (new_alias);
}


extern void
on_aliases_settings_add_clicked (GtkWidget *w, gpointer user_data)
{
	aliases_settings_entry_add ();
}


extern void
on_aliases_settings_delete_clicked (GtkWidget *w, gpointer user_data)
{
	g_return_if_fail (tool_get_access ());
	
	aliases_settings_entry_selected = list_delete_entry (GTK_LIST (tool_widget_get ("aliases_settings_list")),
																											 aliases_settings_entry_selected);
	
	gtk_widget_set_sensitive (tool_widget_get ("aliases_settings_ok"), TRUE);
}


extern void
on_aliases_settings_ok_clicked (GtkWidget *w, gpointer user_data)
{
	gboolean is_new;
	gchar *ip, *text;
	GString *aliases;
	GList *l;
	GtkWidget *clist;

	if (tool_get_access ())
	{
		clist = tool_widget_get ("statichost_list");
		is_new = (gboolean) (gtk_object_get_data 
												 (GTK_OBJECT (tool_widget_get ("aliases_settings_dialog")), "new"));
		
		if (is_new)
		{
			gchar *entry[] = {"", "", NULL};
			statichost_row_selected = gtk_clist_append (GTK_CLIST (clist), entry);
			gtk_clist_select_row (GTK_CLIST (clist), statichost_row_selected, 0);
			scrolled_window_scroll_bottom (tool_widget_get ("statichost_sw"));
		}

		aliases = g_string_new (NULL);
			
		ip = gtk_editable_get_chars (GTK_EDITABLE (tool_widget_get ("aliases_settings_ip")), 0, -1);
		gtk_clist_set_text (GTK_CLIST (clist), statichost_row_selected, 0, ip);
		
		for (l = GTK_LIST (tool_widget_get ("aliases_settings_list"))->children; l; l = l->next)
		{
			gtk_label_get (GTK_LABEL (GTK_BIN (l->data)->child), &text);
			if (l->prev)
				g_string_append_c (aliases, ' ');
			g_string_append (aliases, text);
		}
		
		gtk_clist_set_text (GTK_CLIST (tool_widget_get ("statichost_list")),
												statichost_row_selected, 1, aliases->str);
		g_string_free (aliases, TRUE);
		tool_set_modified (TRUE);
	}
	
	aliases_settings_dialog_close ();
}


extern void
on_aliases_settings_cancel_clicked (GtkWidget *w, gpointer user_data)
{
	aliases_settings_dialog_close ();
}
#endif

/* Helper functions */

static void
reply_cb (gint val, gpointer data)
{
	reply = val;
}

#if 0
static void
searchdomain_actions_set_sensitive (gboolean state)
{
	GtkWidget *w;
	
	if (tool_get_access())
	{
		gtk_widget_set_sensitive (tool_widget_get ("searchdomain_delete"), state);
		
		gtk_widget_set_sensitive (tool_widget_get ("searchdomain_move_up"), FALSE);
		gtk_widget_set_sensitive (tool_widget_get ("searchdomain_move_down"), FALSE);
		
		if (state)
		{
			if (SEARCHDOMAIN_ROW_SELECTED != 0)
				gtk_widget_set_sensitive (tool_widget_get ("searchdomain_move_up"), TRUE);
			
			w = tool_widget_get ("searchdomain_list");
			if (SEARCHDOMAIN_ROW_SELECTED != (gint) gtk_object_get_data (GTK_OBJECT (w), "max") - 1)
				gtk_widget_set_sensitive (tool_widget_get ("searchdomain_move_down"), TRUE);
		}
	}
}
#endif

static void
statichost_actions_set_sensitive (gboolean state)
{
	gtk_widget_set_sensitive (tool_widget_get ("statichost_delete"), state);
	gtk_widget_set_sensitive (tool_widget_get ("statichost_update"), state); 
}

#if 0
static void
aliases_settings_actions_set_sensitive (gboolean state)
{
	if (tool_get_access())
	{
		if (g_list_length (GTK_LIST (tool_widget_get ("aliases_settings_list"))->children) < 2)
			state = FALSE;
		
		gtk_widget_set_sensitive (tool_widget_get ("aliases_settings_delete"), state);
	}
}

static void 
listitem_children_swap (GtkWidget *src, GtkWidget *dest)
{
	GtkWidget *src_child, *dest_child;
	
	src_child = GTK_BIN (src)->child;
	dest_child = GTK_BIN (dest)->child;

	gtk_widget_ref (dest_child);
	gtk_container_remove (GTK_CONTAINER (dest), dest_child);
	gtk_widget_reparent (src_child, dest);
	gtk_container_add (GTK_CONTAINER (src), dest_child);
	gtk_widget_unref (dest_child);
	
	gtk_widget_draw_default (src_child);
	gtk_widget_draw_default (dest_child);
}

static void
aliases_settings_dialog_close (void)
{
	GtkWidget *w0;

	w0 = tool_widget_get ("aliases_settings_ip");
	gtk_entry_set_text (GTK_ENTRY (w0), "");

	w0 = tool_widget_get ("aliases_settings_new");
	gtk_entry_set_text (GTK_ENTRY (w0), "");

	w0 = tool_widget_get ("aliases_settings_list");
	gtk_list_remove_items (GTK_LIST (w0), GTK_LIST (w0)->children);
	
	w0 = tool_widget_get ("aliases_settings_dialog");
	gtk_object_remove_data (GTK_OBJECT (w0), "new");
	gtk_widget_hide (w0);
}

static void
aliases_settings_list_fill (void)
{
	gchar *aliases, **split;
	GList *l;
	GtkWidget *item;
	int i;
	
	gtk_clist_get_text (GTK_CLIST (tool_widget_get ("statichost_list")),
											statichost_row_selected, 1, &aliases);
	split = g_strsplit (aliases, " ", -1);
	
	for (l = NULL, i = 0; split[i]; i++)
	{
		item = gtk_list_item_new_with_label (split[i]);
		gtk_widget_show (item);
		l = g_list_append (l, item);
	}
	
	gtk_list_append_items (GTK_LIST (tool_widget_get ("aliases_settings_list")), l);
	
	g_strfreev (split);
}

static void
aliases_settings_entry_add (void)
{
	GtkWidget *item, *editable, *list;
	GList *list_add = NULL;
	gchar *text, *c, c2;
	
	g_return_if_fail (tool_get_access ());
	
	editable = tool_widget_get ("aliases_settings_new");
	gtk_widget_grab_focus (GTK_WIDGET (editable));
	
	text = gtk_editable_get_chars (GTK_EDITABLE (editable), 0, -1);
	g_strstrip (text);
	g_return_if_fail (strlen (text));
	
	for (c2 = '.', c = text; *c; c2 = *c, c++)
		if (! ((*c >= 'a' && *c <= 'z') ||
					 (*c >= 'A' && *c <= 'Z') ||
					 (*c >= '0' && *c <= '9') ||
					 (*c == '.' && c2 != '.')))
		{
			GnomeDialog *dialog;
		  GtkWindow *parent;
			gchar *txt;
			
			parent = GTK_WINDOW (tool_widget_get ("nameresolution_admin"));
			txt = _("The alias entry may only contain letters, numbers and dots (.),\nwhich may work as sub-domain separators.");
			dialog = GNOME_DIALOG (gnome_error_dialog_parented (txt, parent));
			gnome_dialog_run (dialog);
			
			return;
		}

	gtk_editable_delete_text (GTK_EDITABLE (editable), 0, -1);

	item = gtk_list_item_new_with_label (text);
	gtk_widget_show (item);
	list_add = g_list_append (list_add, item);
	
	list = tool_widget_get ("aliases_settings_list");
	gtk_list_append_items (GTK_LIST (list), list_add);
	gtk_list_select_child (GTK_LIST (list), item);
	
	scrolled_window_scroll_bottom (tool_widget_get ("aliases_settings_sw"));
	
	gtk_widget_set_sensitive (tool_widget_get ("aliases_settings_ok"), TRUE);
	gtk_widget_grab_focus (GTK_WIDGET (editable));
}

static void
scrolled_window_scroll_bottom (GtkWidget *sw)
{
	GtkAdjustment *adj;
	
	while (gtk_events_pending ())
		gtk_main_iteration ();
	
	adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (sw));
	gtk_adjustment_set_value (adj, adj->upper - adj->page_size);
}

static GtkWidget *
list_delete_entry (GtkList *list, GtkWidget *entry)
{
	GList *l;
	GtkWidget *item = NULL;
	
	g_return_val_if_fail (entry != NULL, NULL);
	
	for (l = list->children; l; l = l->next)
		if (l->data == entry)
	  {
			if (l->next)
				item = l->next->data;
			else if (l->prev)
				item = l->prev->data;
			else
				item = NULL;
			break;
		}
	
	gtk_widget_destroy (entry);
	
	if (item)
		gtk_list_select_child (list, item);
	
	return item;
}
#endif

/* yeah, I don't like this formatting either */
static const char *hint_entry[][3] = { {
	"hostname", "general_help", 		
	N_("This is your host name.  It is ending one day at a time.") 
},{
	"description", "general_help", 
	N_("Your computer; what not.")
},{
	"workgroup", "general_help",
	N_("dum de dum")
},{
	"wins_ip", "general_help", 
	N_("this is boring")
},{
	"domain", "dns_help",     
	N_("Enter your DNS domain (bug-buddy.org)")
},{
	"dns_list", "dns_help",
	N_("Enter a list of your DNS servers' IP addresses: 1.2.3.4, 2.3.4.5"),
},{
	"search_list", "dns_help",
	N_("Enter a list of domains where hosts will be searched for (andrew.cmu.edu, res.cmu.edu, helixcode.com)") 
},{
	"ip", "hosts_help",
	N_("Enter the IP address of the host: 1.2.4.9") 
},{
	"alias", "hosts_help",
	N_("Enter one alias per line") 
},{
	NULL
} };

static GHashTable *help_hash;

void
init_hint_entries (void)
{
	int i;
	help_hash = g_hash_table_new (g_str_hash, g_str_equal);

	for (i=0; hint_entry[i][0]; i++)
		g_hash_table_insert (help_hash, hint_entry[i][0], hint_entry[i]);
}

gint
update_hint (GtkWidget *w, GdkEventFocus *event, gpointer null)
{
	char *name;
	char **entry;
	GtkWidget *label;

	name = glade_get_widget_name (w);
	if (!name)
		return FALSE;

	entry = g_hash_table_lookup (help_hash, name);

	if (!entry)
		return FALSE;
	
	label = tool_widget_get (entry[1]);
	gtk_label_set_text (GTK_LABEL (label), _(entry[2]));

	return FALSE;
}

void
on_connection_configure_clicked (GtkWidget *w, gpointer null)
{
	g_message ("connection configure");
}

void
on_connection_delete_clicked (GtkWidget *w, gpointer null)
{
	g_message ("connection delete");
}

void
on_connection_add_clicked (GtkWidget *w, gpointer null)
{
	g_message ("connection add");
}

void
on_dns_dhcp_toggled (GtkWidget *w, gpointer null)
{
	char *ws[] = { "domain", "dns_list", "domain_label", "dns_list_label", NULL };
	int i, b;

	b = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
	for (i=0; ws[i]; i++)
		gtk_widget_set_sensitive (tool_widget_get (ws[i]), b);

}

void
on_samba_use_toggled (GtkWidget *w, gpointer null)
{
	gtk_widget_set_sensitive (tool_widget_get ("samba_frame"),
				  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));}
