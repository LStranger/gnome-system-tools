/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* Copyright (C) 2004 Carlos Garnacho
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
 * Authors: Carlos Garnacho Parro  <carlosg@gnome.org>
 */

#include "gst.h"
#include "gst-network-tool.h"
#include "network-iface.h"
#include "ifaces-list.h"
#include "callbacks.h"

extern GstTool *tool;

static void
enable_iface (GstIface *iface)
{
  GtkWidget *dialog;
  gboolean   success;

  success = gst_iface_enable (iface);

  if (!success)
    {
      dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
				       GTK_DIALOG_MODAL,
				       GTK_MESSAGE_ERROR,
				       GTK_BUTTONS_CLOSE,
				       _("Could not enable the interface %s"),
				       gst_iface_get_dev (iface),
				       NULL);
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						_("Check that the settings are apropriate for "
						  "this network and that the computer is correctly "
						  "connected to it."),
						NULL);
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
    }
}

void
on_table_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  GtkWidget    *properties, *activate, *deactivate;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GstIface     *iface;
  gboolean      enabled, configured;
  gboolean      state_properties, state_activate, state_deactivate;

  properties = gst_dialog_get_widget (tool->main_dialog, "properties_button");
  activate   = gst_dialog_get_widget (tool->main_dialog, "activate_button");
  deactivate = gst_dialog_get_widget (tool->main_dialog, "deactivate_button");

  state_properties = state_activate = state_deactivate = FALSE;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter,
			  COL_OBJECT, &iface,
			  COL_CONFIGURED, &configured,
			  -1);
      enabled = gst_iface_get_enabled (iface);
      g_object_unref (iface);

      state_properties = TRUE;
      state_activate = configured && !enabled;
      state_deactivate = configured && enabled;
    }

  gtk_widget_set_sensitive (properties, state_properties);
  gtk_widget_set_sensitive (activate,   state_activate);
  gtk_widget_set_sensitive (deactivate, state_deactivate);
}

void
on_iface_properties_clicked (GtkWidget *widget, gpointer data)
{
  GstConnectionDialog *dialog;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GstIface *iface;

  selection = gtk_tree_view_get_selection (GST_NETWORK_TOOL (tool)->interfaces_list);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter,
			  COL_OBJECT, &iface,
			  -1);

      dialog = GST_NETWORK_TOOL (tool)->dialog;
      connection_dialog_prepare (dialog, iface);
      g_object_unref (iface);

      gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog), GTK_WINDOW (tool->main_dialog));
      gtk_widget_show (dialog->dialog);
    }
}

void
on_iface_active_changed (GtkWidget *widget, gpointer data)
{
  GstConnectionDialog *dialog;
  gboolean active;

  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
  dialog = GST_NETWORK_TOOL (tool)->dialog;

  gtk_widget_set_sensitive (dialog->wireless_frame, active);
  gtk_widget_set_sensitive (dialog->ethernet_frame, active);
  gtk_widget_set_sensitive (dialog->plip_frame,     active);
  gtk_widget_set_sensitive (dialog->modem_frame,    active);
  gtk_widget_set_sensitive (dialog->account_page,   active);
  gtk_widget_set_sensitive (dialog->options_page,   active);
}

void
on_bootproto_changed (GtkWidget *widget, gpointer data)
{
  GstConnectionDialog *dialog;
  gboolean enabled;
  gint     pos;

  dialog = GST_NETWORK_TOOL (tool)->dialog;
  pos = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
  enabled = (pos == 0);

  gtk_widget_set_sensitive (dialog->address, enabled);
  gtk_widget_set_sensitive (dialog->netmask, enabled);
  gtk_widget_set_sensitive (dialog->gateway, enabled);
}

void
on_connection_cancel_clicked (GtkWidget *widget, gpointer data)
{
  GstConnectionDialog *dialog;

  dialog = GST_NETWORK_TOOL (tool)->dialog;
  g_object_unref (dialog->iface);
  gtk_widget_hide (dialog->dialog);

  if (dialog->standalone)
    gtk_main_quit ();
}

void
on_connection_ok_clicked (GtkWidget *widget, gpointer data)
{
  GstConnectionDialog *dialog;
  GtkTreeSelection    *selection;
  GtkTreeModel        *model;
  GtkTreeIter          iter;
  GstIface            *iface;

  dialog = GST_NETWORK_TOOL (tool)->dialog;
  gtk_widget_hide (dialog->dialog);

  if (dialog->changed)
    {
      connection_save (dialog);
      selection = gtk_tree_view_get_selection (GST_NETWORK_TOOL (tool)->interfaces_list);

      if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
	  gtk_tree_model_get (model, &iter, COL_OBJECT, &iface, -1);

	  /* modify the config realtime */
	  if (gst_iface_get_enabled (iface))
	    enable_iface (iface);

	  ifaces_model_modify_interface_at_iter (&iter);
	  g_object_unref (iface);

	  g_signal_emit_by_name (G_OBJECT (selection), "changed");
	  gst_dialog_modify (tool->main_dialog);
	}
    }

  g_object_unref (dialog->iface);

  if (dialog->standalone)
    {
      gtk_signal_emit_by_name (GTK_OBJECT (tool->main_dialog), "apply", tool);
      gtk_main_quit ();
    }
}

void
on_detect_modem_clicked (GtkWidget *widget, gpointer data)
{
  GstNetworkTool      *network_tool;
  GstConnectionDialog *dialog;
  GtkWidget           *d;
  GdkCursor           *cursor;
  gchar               *dev = NULL;

  network_tool = GST_NETWORK_TOOL (tool);
  dialog = network_tool->dialog;

  /* give some feedback to let know the user that the tool is busy */
  gtk_entry_set_text (GTK_ENTRY (GTK_BIN (GTK_COMBO_BOX (dialog->serial_port))->child), "");
  gtk_widget_set_sensitive (dialog->serial_port, FALSE);
  gtk_widget_set_sensitive (dialog->detect_modem, FALSE);

  cursor = gdk_cursor_new (GDK_WATCH);
  gdk_window_set_cursor (GTK_WIDGET (dialog->dialog)->window, cursor);
  gdk_cursor_unref (cursor);

  dev = connection_detect_modem ();

  /* remove the user feedback */
  gtk_widget_set_sensitive (dialog->detect_modem, TRUE);
  gtk_widget_set_sensitive (dialog->serial_port, TRUE);
  gdk_window_set_cursor (GTK_WIDGET (dialog->dialog)->window, NULL);

  if (!dev || !*dev)
    {
      d = gtk_message_dialog_new (GTK_WINDOW (dialog->dialog),
				  GTK_DIALOG_MODAL,
				  GTK_MESSAGE_INFO,
				  GTK_BUTTONS_CLOSE,
				  _("Could not autodetect modem device"),
				  NULL);
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (d),
						_("Check that the device is not busy and "
						  "that is correctly attached to the computer"),
						NULL);
      gtk_dialog_run (GTK_DIALOG (d));
      gtk_widget_destroy (d);
    }
  else
    gtk_entry_set_text (GTK_ENTRY (GTK_BIN (GTK_COMBO_BOX (dialog->serial_port))->child), dev);

  g_free (dev);
}

static void
do_popup_menu (GtkWidget *table, GstTablePopup *table_popup, GdkEventButton *event)
{
  gint button, event_time;

  if (!table_popup)
    return;

  if (event)
    {
      button     = event->button;
      event_time = event->time;
    }
  else
    {
      button     = 0;
      event_time = gtk_get_current_event_time ();
    }

  if (table_popup->setup)
    (table_popup->setup) (table);

  gtk_menu_popup (GTK_MENU (table_popup->popup), NULL, NULL, NULL, NULL,
		  button, event_time);
}

gboolean
on_table_button_press (GtkWidget *table, GdkEventButton *event, gpointer data)
{
  GtkTreePath      *path;
  GstTablePopup    *table_popup;
  GtkTreeSelection *selection;

  table_popup = (GstTablePopup *) data;
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));

  if (event->button == 3)
    {
      gtk_widget_grab_focus (table);

      if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (table),
					 event->x, event->y,
					 &path, NULL, NULL, NULL))
        {
	  gtk_tree_selection_unselect_all (selection);
	  gtk_tree_selection_select_path (selection, path);

	  do_popup_menu (table, table_popup, event);
	}

      return TRUE;
    }

  if (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS)
    {
      if (table_popup->properties)
	(table_popup->properties) (NULL, NULL);

      return TRUE;
    }

  return FALSE;
}

gboolean
on_table_popup_menu (GtkWidget *widget, gpointer data)
{
  GstTablePopup *table_popup = (GstTablePopup *) data;

  do_popup_menu (widget, table_popup, NULL);
  return TRUE;
}

static void
enable_disable_iface (GstNetworkTool *network_tool, gboolean enable)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GstIface     *iface;

  selection = gtk_tree_view_get_selection (network_tool->interfaces_list);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter,
			  COL_OBJECT, &iface,
			  -1);
      if (enable)
	enable_iface (iface);
      else
	gst_iface_disable (iface);

      ifaces_model_modify_interface_at_iter (&iter);
      g_object_unref (iface);

      /* we need this to update the buttons state */
      g_signal_emit_by_name (G_OBJECT (selection), "changed");
    }
}

void
on_activate_button_clicked (GtkWidget *widget, gpointer data)
{
  enable_disable_iface (GST_NETWORK_TOOL (tool), TRUE);
}

void
on_deactivate_button_clicked (GtkWidget *widget, gpointer data)
{
  enable_disable_iface (GST_NETWORK_TOOL (tool), FALSE);
}

void
on_host_aliases_add_clicked (GtkWidget *widget, gpointer data)
{
  host_aliases_run_dialog (NULL);
}

void
on_host_aliases_properties_clicked (GtkWidget *widget, gpointer data)
{
  GtkTreeSelection *selection;
  GtkTreeView      *list;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  list  = GST_NETWORK_TOOL (tool)->host_aliases_list;
  selection = gtk_tree_view_get_selection (list);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    host_aliases_run_dialog (&iter);
}

void
on_host_aliases_delete_clicked (GtkWidget *widget, gpointer data)
{
  GtkTreeSelection *selection;
  GtkTreeView      *list;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  list  = GST_NETWORK_TOOL (tool)->host_aliases_list;
  selection = gtk_tree_view_get_selection (list);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
      gst_dialog_modify (tool->main_dialog);
    }
}

void
on_dialog_changed (GtkWidget *widget, gpointer data)
{
  GstNetworkTool *network_tool = GST_NETWORK_TOOL (tool);

  network_tool->dialog->changed = TRUE;
  connection_check_fields (network_tool->dialog);
}

gboolean
callbacks_check_hostname_hook (GstDialog *dialog, gpointer data)
{
  GstNetworkTool *network_tool;
  gchar          *hostname_old, *hostname_new;
  xmlNodePtr      root, node;
  GtkWidget      *d;
  gint            res;

  network_tool = GST_NETWORK_TOOL (dialog->tool);
  root = gst_xml_doc_get_root (dialog->tool->config);
  node = gst_xml_element_find_first (root, "hostname");

  hostname_old = gst_xml_element_get_content (node);
  hostname_new = (gchar *) gtk_entry_get_text (network_tool->hostname);

  if (hostname_old && hostname_new &&
      (strcmp (hostname_new, hostname_old) != 0))
    {
      d = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
				  GTK_DIALOG_MODAL,
				  GTK_MESSAGE_WARNING,
				  GTK_BUTTONS_NONE,
				  _("The host name has changed"),
				  NULL);
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (d),
						_("This will prevent you "
						  "from launching new applications, and so you will "
						  "have to log in again. Continue anyway?"),
						NULL);
      gtk_dialog_add_buttons (GTK_DIALOG (d),
			      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			      _("Change _Host name"), GTK_RESPONSE_ACCEPT,
			      NULL);

      res = gtk_dialog_run (GTK_DIALOG (d));
      gtk_widget_destroy (d);

      switch (res)
        {
	case GTK_RESPONSE_ACCEPT:
	  g_free (hostname_old);
	  return TRUE;
	case GTK_RESPONSE_CANCEL:
	default:
	  gtk_entry_set_text (network_tool->hostname, hostname_old);
	  return FALSE;
	}
    }

  g_free (hostname_old);
  return TRUE;
}

gboolean
on_ip_address_focus_out (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
  GstConnectionDialog *dialog = GST_NETWORK_TOOL (tool)->dialog;

  connection_check_netmask (dialog->address, dialog->netmask);
  return FALSE;
}
