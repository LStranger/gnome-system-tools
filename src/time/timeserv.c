#include "config.h"

#include <stdio.h>

#include <gnome.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gnome-canvas-pixbuf.h>

#include "global.h"

#include "transfer.h"
#include "timeserv.h"

extern XstTool *tool;

void
on_ntp_addserver (GtkButton *button, gpointer data)
{
	GtkEditable *ntp_entry;
	GtkList *ntp_list;
	GtkWidget *item;
	GList *list_add = NULL;
	gchar *text;
	
	ntp_entry = GTK_EDITABLE (xst_dialog_get_widget (tool->main_dialog, "ntp_entry"));
	ntp_list = GTK_LIST (xst_dialog_get_widget (tool->main_dialog, "ntp_list"));
	
	text = gtk_editable_get_chars (ntp_entry, 0, -1);
	g_strstrip (text);
	
	if (strchr (text, ' ')) {
		gtk_widget_grab_focus (GTK_WIDGET (ntp_entry));
		gtk_editable_select_region (ntp_entry, 0, -1);
		return;
	}
	
	if (!strlen (text)) return;
	
	gtk_editable_delete_text (ntp_entry, 0, -1);
	gtk_widget_grab_focus (GTK_WIDGET (ntp_entry));
	
	item = gtk_list_item_new_with_label (text);
	gtk_widget_show (item);
	gtk_list_item_select (GTK_LIST_ITEM (item));
	list_add = g_list_append (list_add, item);
	gtk_list_append_items (ntp_list, list_add);
}
