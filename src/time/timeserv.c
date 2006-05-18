/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include "config.h"

#include <string.h>
#include <stdio.h>
#include "gst.h"
#include "timeserv.h"

void
on_ntp_addserver (GtkButton *button, GstDialog *dialog)
{
	GtkEditable *ntp_entry;
	GtkTreeView *ntp_list;
	GtkWidget *item;
	GtkListStore *store;
	GtkTreeIter iter;
	gchar *text;
	
	ntp_entry = GTK_EDITABLE (gst_dialog_get_widget (dialog, "ntp_entry"));
	ntp_list = GTK_TREE_VIEW (gst_dialog_get_widget (dialog, "ntp_list"));
        store = GTK_LIST_STORE (gtk_tree_view_get_model (ntp_list));
	
	text = gtk_editable_get_chars (ntp_entry, 0, -1);
	g_strstrip (text);
	
	if (strchr (text, ' ')) {
		gtk_widget_grab_focus (GTK_WIDGET (ntp_entry));
		gtk_editable_select_region (ntp_entry, 0, -1);

		g_free (text);
		return;
	}

	if (text && *text) {
		gtk_editable_delete_text (ntp_entry, 0, -1);
		gtk_widget_grab_focus (GTK_WIDGET (ntp_entry));
	
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, TRUE, 1, text, -1);

		g_free (text);
	}
}
