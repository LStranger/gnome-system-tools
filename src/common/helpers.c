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
 * Authors: Hans Petter Jansson <hpj@ximian.com> and Arturo Espinosa <arturo@ximian.com>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>

#include <gnome.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "helpers.h"

#include "checked.xpm"
#include "unchecked.xpm"

/* define to x for debugging */
#define d(x)

#if 0
int
ip_first_entry_is_valid (GtkEditable *ip_entry)
{
	gchar *ip;

	ip = gtk_editable_get_chars (GTK_EDITABLE (ip_entry), 0, -1);

	if (strtoul (ip, 0, 10) == 0 || strtoul (ip, 0, 10) > 254)
	{
		gtk_widget_grab_focus (GTK_WIDGET (ip_entry));
		gtk_editable_select_region (GTK_EDITABLE (ip_entry), 0, -1);
		return (FALSE);
	}

	return (TRUE);
}


int
ip_entry_is_valid (GtkEditable *ip_entry)
{
	gchar *ip;

	ip = gtk_editable_get_chars (GTK_EDITABLE (ip_entry), 0, -1);

	if (strtoul (ip, 0, 10) > 254)
	{
		gtk_widget_grab_focus (GTK_WIDGET (ip_entry));
		gtk_editable_select_region (GTK_EDITABLE (ip_entry), 0, -1);
		return (FALSE);
	}

	return (TRUE);
}


gboolean
check_ip_number(GtkEditable *editable)
{
	GtkWidget *dialog;
	gint nr_val;
	gint nr_len;

	nr_val = atoi (gtk_editable_get_chars (editable, 0, -1));
	nr_len = strlen (gtk_editable_get_chars (editable, 0, -1));

	if (nr_val > 255 || nr_len == 0)
	{
		dialog = gnome_ok_dialog_parented("IP Address numbers are in the range [0-255]\n"
			"and can't be empty.", GTK_WINDOW (tool_get_top_window()));
    
		gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

		gtk_widget_grab_focus (GTK_WIDGET (editable));
		gtk_entry_select_region (GTK_ENTRY (editable), 0, -1);
		return FALSE;
	}

	return TRUE;
}

static gchar *
ip_num_next (gchar *ip)
{
	gchar *ret;
	
	ret = strchr (ip, '.');
	
	if (ret)
		*ret++ = 0;
	
	return ret;
}

static gboolean 
ip_num_valid (gchar *strnum)
{
	guint num;
	char *end;
	
	if (!strnum)
		return FALSE;
	
	num = strtoul (strnum, &end, 10);
	if (*end != 0)
		return FALSE;
	
	return ((num >= 0) && (num <= 255));
}


gboolean
list_add_ip (GtkList *list, GtkWidget *w_ip)
{
	gchar *ip, *ip_num[4], *ip_next;
	gboolean success;
	int i;
	GtkWidget *item;
	GList *list_add = NULL;
	
	ip = gtk_editable_get_chars (GTK_EDITABLE (w_ip), 0, -1);
	
	ip_next = g_strdup (ip);
	for (i = 0; (i < 4) && ip_next; i++)
	{
		ip_num[i] = ip_next;
		ip_next = ip_num_next (ip_next);
		if (!ip_num_valid (ip_num[i]))
			break;
	}
	
	if ((i < 4) || (ip_next) ||
	    (atoi(ip_num[3]) == 0) || (atoi(ip_num[3]) == 255) ||
	    (atoi(ip_num[0]) == 0) || (atoi(ip_num[0]) == 255))
		success = FALSE;
	else
	{
		success = TRUE;
		gtk_editable_delete_text (GTK_EDITABLE (w_ip), 0, -1);
		gtk_widget_grab_focus (GTK_WIDGET (w_ip));

		item = gtk_list_item_new_with_label (ip);
		gtk_widget_show (item);
		list_add = g_list_append (list_add, item);
		gtk_list_append_items (GTK_LIST (list), list_add);
		gtk_list_select_child (GTK_LIST (list), item);
	}
	
	g_free (ip);
	g_free (ip_num[0]);
	
	return success;
}


void
list_add_word (GtkList *list, GtkWidget *editable)
{
	GtkWidget *item;
	GList *list_add = NULL;
	gchar *text;

	text = gtk_editable_get_chars (GTK_EDITABLE (editable), 0, -1);
	g_strstrip (text);

	if (strchr (text, ' '))
	{
		gtk_widget_grab_focus (GTK_WIDGET (editable));
		gtk_editable_select_region (GTK_EDITABLE (editable), 0, -1);
		return;
	}

	if (!strlen (text)) return;

	gtk_editable_delete_text (GTK_EDITABLE (editable), 0, -1);
	gtk_widget_grab_focus (GTK_WIDGET (editable));

	item = gtk_list_item_new_with_label (text);
	gtk_widget_show (item);
	list_add = g_list_append (list_add, item);
	gtk_list_append_items (GTK_LIST (list), list_add);
	g_list_remove (list_add, item);
}


void
clist_add_word (GtkCList *clist, GtkWidget *editable)
{
	gchar *text;
	gchar *row_data[3];

	text = gtk_editable_get_chars (GTK_EDITABLE (editable), 0, -1);
	g_strstrip (text);

	if (strchr (text, ' '))
	{
		gtk_widget_grab_focus (GTK_WIDGET (editable));
		gtk_editable_select_region (GTK_EDITABLE (editable), 0, -1);
		g_free(text);
		return;
	}

	if (!strlen (text)) return;

	gtk_editable_delete_text (GTK_EDITABLE (editable), 0, -1);
	gtk_widget_grab_focus (GTK_WIDGET (editable));

	row_data[0] = text;
	row_data[1] = "";
	row_data[2] = NULL;
	gtk_clist_select_row (clist, gtk_clist_append (clist, row_data), -1);
	
	g_free(text);
}


void
clist_add_ip (GtkCList *clist, GtkWidget *w_ip_1, GtkWidget *w_ip_2, GtkWidget *w_ip_3, GtkWidget *w_ip_4)
{
	gchar *ip, *ip1, *ip2, *ip3, *ip4;
	gchar *row_data[3];

	if (!ip_first_entry_is_valid (GTK_EDITABLE (w_ip_1)) ||
	    !ip_entry_is_valid (GTK_EDITABLE (w_ip_2)) ||
	    !ip_entry_is_valid (GTK_EDITABLE (w_ip_3)) ||
	    !ip_entry_is_valid (GTK_EDITABLE (w_ip_4)))
		return;

	ip1 = gtk_editable_get_chars (GTK_EDITABLE (w_ip_1), 0, -1);
	ip2 = gtk_editable_get_chars (GTK_EDITABLE (w_ip_2), 0, -1);
	ip3 = gtk_editable_get_chars (GTK_EDITABLE (w_ip_3), 0, -1);
	ip4 = gtk_editable_get_chars (GTK_EDITABLE (w_ip_4), 0, -1);

	gtk_editable_delete_text (GTK_EDITABLE (w_ip_1), 0, -1);
	gtk_editable_delete_text (GTK_EDITABLE (w_ip_2), 0, -1);
	gtk_editable_delete_text (GTK_EDITABLE (w_ip_3), 0, -1);
	gtk_editable_delete_text (GTK_EDITABLE (w_ip_4), 0, -1);
	gtk_widget_grab_focus (GTK_WIDGET (w_ip_1));

	ip = g_strjoin (".", ip1, ip2, ip3, ip4, NULL);

	row_data[0] = ip;
	row_data[1] = "";
	row_data[2] = NULL;
	gtk_clist_select_row (clist, gtk_clist_append (clist, row_data), -1);

	g_free (ip);
}
#endif

gboolean
check_ip_string (const char *ip, gboolean allow_mask)
{
	char **nums, *num;
	int i, j, x, min = 0, max = 0;
	gboolean retval = FALSE;

	d(g_print ("checking: %s\n", ip));

	if (!(ip && ip[0] && strlen (ip) > 6))
		return retval;

	nums = g_strsplit (ip, ".", 3);

	for (i=0; (num = nums[i]); i++) {
		d(g_print ("    checking: %s\n", num));
		for (j=0; num[j]; j++)
			if (!isdigit (num[j]))
				goto real_check_ip_cleanup;
		x = atoi (num);
		switch (i) {
		case 0:
			min = 1;
			max = allow_mask ? 255 : 254;
			break;
		case 1: case 2:
			min = 0;
			max = 255;
			break;
		case 3:
			min = allow_mask ? 0 : 1;
			max = allow_mask ? 255 : 254;
			break;
		default:
			g_assert_not_reached ();
		}

		if (x < min || x > max)
			goto real_check_ip_cleanup;
	}
	retval = (i == 4);
 real_check_ip_cleanup:
	g_strfreev (nums);
	d(g_print ("returning: %d\n", retval));
	return retval;
}

gboolean
check_ip_entry (GtkEntry *entry, gboolean allow_mask)
{
	g_return_val_if_fail (entry != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_ENTRY (entry), FALSE);

	return check_ip_string (gtk_entry_get_text (entry), allow_mask);
}

/* --- CTree checkmarks --- */


GdkPixmap *checked_pixmap = NULL, *unchecked_pixmap = NULL;
GdkBitmap *checked_mask = NULL, *unchecked_mask = NULL;


void
set_ctree_checkmark (GtkCTree *ctree, GtkCTreeNode *node, gint column, gboolean state)
{
	GdkPixbuf *pixbuf;
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	if (state)
	{
		if (!checked_pixmap)
		{
			pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) checked_xpm);
			gdk_pixbuf_render_pixmap_and_mask (pixbuf, &checked_pixmap, &checked_mask, 1);
		}

		pixmap = checked_pixmap;
		mask = checked_mask;
	}
	else
	{
		if (!unchecked_pixmap)
		{
			pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) unchecked_xpm);
			gdk_pixbuf_render_pixmap_and_mask (pixbuf, &unchecked_pixmap, &unchecked_mask, 1);
		}

		pixmap = unchecked_pixmap;
		mask = unchecked_mask;
	}

	gtk_ctree_node_set_pixmap (ctree, node, column, pixmap, mask);
}


gboolean
get_ctree_checkmark (GtkCTree *ctree, GtkCTreeNode *node, gint column)
{
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	gtk_ctree_node_get_pixmap (ctree, node, column, &pixmap, &mask);

	if (pixmap == checked_pixmap) return (TRUE);
	return (FALSE);
}


/* --- CList checkmarks --- */


void
set_clist_checkmark (GtkCList *clist, gint row, gint column, gboolean state)
{
	GdkPixbuf *pixbuf;
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	if (state)
	{
		if (!checked_pixmap)
		{
			pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) checked_xpm);
			gdk_pixbuf_render_pixmap_and_mask (pixbuf, &checked_pixmap, &checked_mask, 1);
		}

		pixmap = checked_pixmap;
		mask = checked_mask;
	}
	else
	{
		if (!unchecked_pixmap)
		{
			pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) unchecked_xpm);
			gdk_pixbuf_render_pixmap_and_mask (pixbuf, &unchecked_pixmap, &unchecked_mask, 1);
		}

		pixmap = unchecked_pixmap;
		mask = unchecked_mask;
	}

	gtk_clist_set_pixmap (clist, row, column, pixmap, mask);
}


gboolean
get_clist_checkmark (GtkCList *clist, gint row, gint column)
{
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	gtk_clist_get_pixmap (clist, row, column, &pixmap, &mask);

	if (pixmap == checked_pixmap) return (TRUE);
	return (FALSE);
}
