/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include "config.h"

#include <stdio.h>

#include <gnome.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "xst.h"

#include "transfer.h"
#include "timeserv.h"

void
on_ntp_addserver (GtkButton *button, XstDialog *dialog)
{
	GtkEditable *ntp_entry;
	GtkTreeView *ntp_list;
	GtkWidget *item;
	GtkListStore *store;
	GtkTreeIter iter;
	gchar *text;
	
	ntp_entry = GTK_EDITABLE (xst_dialog_get_widget (dialog, "ntp_entry"));
	ntp_list = GTK_TREE_VIEW (xst_dialog_get_widget (dialog, "ntp_list"));
	store = GTK_LIST_STORE (gtk_tree_view_get_model (ntp_list));
	
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
	
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, g_strdup (text), -1);

}
