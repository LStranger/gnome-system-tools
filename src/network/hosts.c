/* 
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
 * Authors: Chema Celorio <chema@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>

#include <gnome.h>

#include "global.h"

#include "callbacks.h"
#include "transfer.h"
#include "hosts.h"

/* Yes, I don't like globals & externs we should really have
   an XstHostsPageInfo struct with all the stuff but it does
   not work with our signals connecting system */
static int hosts_row_selected = -1;
static gboolean updating = FALSE;
static gboolean hack = FALSE;

extern XstTool *tool;

static char *
fixup_text_list (GtkWidget *text)
{
	char *s2, *s;

	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (GTK_IS_EDITABLE (text), NULL);

	s = gtk_editable_get_chars (GTK_EDITABLE (text), 0, -1);
	
	for (s2 = strchr (s, '\n'); s2; s2 = strchr (s2, '\n'))
		*s2 = ' ';

	return s;
}

static char *
fixdown_text_list (char *s)
{
	char *s2;

	g_return_val_if_fail (s != NULL, NULL);

	for (s2 = strchr (s, ' '); s2; s2 = strchr (s2, ' '))
		*s2 = '\n';

	return s;
}

/**
 * xst_hosts_ip_is_in_list:
 * @ip_str: 
 * 
 * Determines is @ip_str is already in the clist. We shuold keep a GList if
 * the ip's in the clists really, not have to query the view to get the data
 * 
 * Return Value: the row in which it lives, -1 if it is not on list
 **/
static gint 
xst_hosts_ip_is_in_list (const gchar *ip_str)
{
	GtkCList *clist;
	gchar *ip;
	gint rows;
	gint row;

	if (!ip_str) 
		return FALSE;
	
	clist = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "statichost_list"));
	rows = GTK_CLIST(clist)->rows;

	for (row = 0; row < rows; row++)
	{
		gtk_clist_get_text (clist, row, 0, &ip);
		if (strcmp (ip, ip_str) == 0)
			return row;
	}
	
	return -1;
}

/**
 * xst_hosts_unselect_all:
 * @void: 
 * 
 * Clears the selection of the GtkCList 
 **/
static void
xst_hosts_unselect_all (void)
{
	GtkCList *clist;
	GtkWidget *alias;

	g_print ("Unselect all\n");
	
	clist = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "statichost_list"));
	alias = xst_dialog_get_widget (tool->main_dialog, "alias");

	updating = TRUE;

	gtk_clist_unselect_all (clist);
	g_print ("b1\n");
	gtk_editable_delete_text (GTK_EDITABLE (alias), 0, -1);

	updating = FALSE;
}

static void
my_gtk_clist_select_row (GtkCList *clist, gint row, gint unused)
{
	GtkScrolledWindow *s_win;
	GtkAdjustment * vadjustment;
	gdouble move_to;
	gint rows;
	g_return_if_fail (GTK_IS_CLIST (clist));

	s_win = GTK_SCROLLED_WINDOW (xst_dialog_get_widget (tool->main_dialog, "statichost_list_sw"));
	vadjustment = gtk_scrolled_window_get_vadjustment (s_win);
	rows = clist->rows;

	move_to = ((((gdouble) row) - 1) / ((gdouble) rows)) * (vadjustment->upper - vadjustment->page_size) * 2;
	if (move_to > (vadjustment->upper - vadjustment->page_size))
	    move_to = vadjustment->upper - vadjustment->page_size;

	if (!hack)
		gtk_adjustment_set_value (vadjustment, move_to);

	updating = TRUE;
	gtk_clist_select_row (clist, row, -1);
	updating = FALSE;
}

/**
 * xst_hosts_select_row:
 * @row: 
 * 
 * Select row
 **/
static void
xst_hosts_select_row (gint row)
{
	GtkCList *clist;

	clist = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "statichost_list"));

	updating = TRUE;

	if (row == -1)
		gtk_clist_unselect_all (clist);
	else
		my_gtk_clist_select_row (clist, row, -1);

	updating = FALSE;
}

void
xst_hosts_update_sensitivity (void)
{
	GtkWidget *delete_button;
	GtkWidget *change_button;
	GtkWidget *add_button;
	GtkWidget *ip;
	GtkWidget *alias;
	gboolean change;
	gboolean delete;
	gboolean add;
	gboolean ip_is_in_list;
	gchar *ip_str;
	gchar *alias_str;

	g_print ("In\n");
	
	if (updating) {
		g_print ("Updating .. return\n");
		return;
	}
	
	/* Get the widgets */
	delete_button = xst_dialog_get_widget (tool->main_dialog, "statichost_delete");
	change_button = xst_dialog_get_widget (tool->main_dialog, "statichost_update");
	add_button    = xst_dialog_get_widget (tool->main_dialog, "statichost_add");

	ip    = xst_dialog_get_widget (tool->main_dialog, "ip");
	alias = xst_dialog_get_widget (tool->main_dialog, "alias");

	/* Get the texts */
	ip_str    = g_strdup (gtk_editable_get_chars (GTK_EDITABLE (ip),    0, -1));
	alias_str = g_strdup (gtk_editable_get_chars (GTK_EDITABLE (alias), 0, -1));
	ip_is_in_list = xst_hosts_ip_is_in_list (ip_str) != -1;
		
	/* DELETE : You can delete if the row is selected and the ip is in the list of ip's
	 * and also that the ip is not the loopback ip address. FIXME
	 * ADD: You can add when the ip is not in the clist already,
	 * CHANGE : You can change when the ip is in the list 
	 */
	delete = (ip_is_in_list) && strcmp (ip_str, "127.0.0.1");
	add = (strlen(ip_str) > 0) && (!ip_is_in_list);
	change = ip_is_in_list;

	/* Set the states */
	gtk_widget_set_sensitive (delete_button, delete);
	gtk_widget_set_sensitive (change_button, change);
	gtk_widget_set_sensitive (add_button,    add);

	g_free (ip_str);
	g_free (alias_str);

	g_print ("Out\n");
}



static void
xst_hosts_clear_entries (void)
{
	GtkWidget *ip;
	GtkWidget *alias;

	ip    = xst_dialog_get_widget (tool->main_dialog, "ip");
	alias = xst_dialog_get_widget (tool->main_dialog, "alias");

	g_print ("Clear entries 1\n");
	updating = TRUE;
	g_print ("b2\n");
	gtk_editable_delete_text (GTK_EDITABLE (ip), 0, -1);
	gtk_editable_delete_text (GTK_EDITABLE (alias), 0, -1);
	updating = FALSE;
	g_print ("Clear entries 3\n");
}


void
on_hosts_ip_changed (GtkEditable *ip, gpointer not_used)
{
	const gchar *ip_str;
	gint row;

	if (updating)
		return;
	
	xst_hosts_update_sensitivity ();
	
	/* Get the texts */
	ip_str = gtk_editable_get_chars (ip,  0, -1);
	row = xst_hosts_ip_is_in_list (ip_str);

	g_print ("row %i row selected %i\n", row, hosts_row_selected);
	if (row != hosts_row_selected)
		xst_hosts_select_row (row);

}

void
on_hosts_alias_changed (GtkEditable *w, gpointer not_used)
{
	if (updating)
		return;
	
	g_print ("Alias changed\n");
}


void
on_hosts_list_unselect_row (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer user_data)
{
	GtkWidget *w;

	hosts_row_selected = -1;

	w = xst_dialog_get_widget (tool->main_dialog, "alias");
	g_print ("b4\n");
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);

	if (updating)
		return;
	
	updating = TRUE;
	
	/* Load aliases into entry widget */
	w = xst_dialog_get_widget (tool->main_dialog, "ip");
	g_print ("b5\n");
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);

	updating = FALSE;

	xst_hosts_update_sensitivity ();

}

void
on_hosts_list_select_row (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer user_data)
{
	GtkWidget *w;
	gchar *row_data;
	gint pos = 0;

	hosts_row_selected = row;

	if (!updating) {
		/* Load aliases into entry widget */
		pos = 0;
		gtk_clist_get_text (GTK_CLIST (clist), row, 0, &row_data);
		w = xst_dialog_get_widget (tool->main_dialog, "ip");

		hack = TRUE;
		gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
		gtk_editable_insert_text (GTK_EDITABLE (w), row_data, strlen (row_data), &pos);
		hack = FALSE;
	}

	updating = TRUE;
	
	pos = 0;
	gtk_clist_get_text (GTK_CLIST (clist), row, 1, &row_data);
	row_data = fixdown_text_list (g_strdup (row_data));
	
	w = xst_dialog_get_widget (tool->main_dialog, "alias");
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
	gtk_editable_insert_text (GTK_EDITABLE (w), row_data, strlen (row_data), &pos);
	g_free (row_data);

	updating = FALSE;

	xst_hosts_update_sensitivity ();
}



void
on_hosts_add_clicked (GtkWidget * button, gpointer user_data)
{
	GtkWidget *clist;
	char *entry[3];
	int row;

	g_return_if_fail (xst_tool_get_access (tool));

	entry[2] = NULL;
	
	entry[0] = gtk_editable_get_chars (
		GTK_EDITABLE (xst_dialog_get_widget (tool->main_dialog, "ip")), 0, -1);

	entry[1] = fixup_text_list (xst_dialog_get_widget (tool->main_dialog, "alias"));

	clist = xst_dialog_get_widget (tool->main_dialog, "statichost_list");

	row = gtk_clist_append (GTK_CLIST (clist), entry);

	xst_hosts_clear_entries ();
	xst_hosts_unselect_all ();
	my_gtk_clist_select_row (GTK_CLIST (clist), row, 0);
}

void
on_hosts_delete_clicked (GtkWidget * button, gpointer user_data)
{
	gchar *txt, *name;
	GtkWidget *parent, *dialog;
	gint res;

	g_return_if_fail (xst_tool_get_access (tool));
	g_return_if_fail (hosts_row_selected != -1);

	parent = GTK_WIDGET (tool->main_dialog);
	gtk_clist_get_text (GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "statichost_list")),
			    hosts_row_selected, 1, &name);

	txt = g_strdup_printf (_("Are you sure you want to delete the aliases for %s?"), name);
	dialog = gnome_question_dialog_parented (txt, NULL, NULL, GTK_WINDOW (parent));
	res = gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
	g_free (txt);

	if (res) return;
		
	gtk_clist_remove (GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "statichost_list")), hosts_row_selected);
	xst_dialog_modify (tool->main_dialog);

	xst_hosts_clear_entries ();
	xst_hosts_unselect_all ();
}

void
on_hosts_update_clicked (GtkWidget *b, gpointer null)
{
	GtkWidget *clist, *w;
	char *s;

	g_return_if_fail (xst_tool_get_access (tool));

	clist = xst_dialog_get_widget (tool->main_dialog, "statichost_list");

	gtk_clist_set_text (GTK_CLIST (clist), hosts_row_selected, 0,
			    gtk_editable_get_chars (
				    GTK_EDITABLE (xst_dialog_get_widget (tool->main_dialog, "ip")), 0, -1));
	
	w = xst_dialog_get_widget (tool->main_dialog, "alias");
	s = fixup_text_list (w);
	gtk_clist_set_text (GTK_CLIST (clist), hosts_row_selected, 1, s);
	g_free (s);

	xst_hosts_clear_entries ();
	xst_hosts_unselect_all ();
}

