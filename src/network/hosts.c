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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "hosts.h"
#include "gst.h"
#include "gst-network-tool.h"
#include "callbacks.h"

extern GstTool *tool;

GtkActionEntry hosts_popup_menu_items [] = {
  { "Add",        GTK_STOCK_ADD,        N_("_Add"),        NULL, NULL, G_CALLBACK (on_host_aliases_add_clicked) },
  { "Properties", GTK_STOCK_PROPERTIES, N_("_Properties"), NULL, NULL, G_CALLBACK (on_host_aliases_properties_clicked) },
  { "Delete",     GTK_STOCK_DELETE,     N_("_Delete"),     NULL, NULL, G_CALLBACK (on_host_aliases_delete_clicked) }
};

const gchar *hosts_ui_description =
  "<ui>"
  "  <popup name='MainMenu'>"
  "    <menuitem action='Add'/>"
  "    <separator/>"
  "    <menuitem action='Properties'/>"
  "    <menuitem action='Delete'/>"
  "  </popup>"
  "</ui>";

static GtkTreeModel*
host_aliases_model_create (void)
{
  GtkListStore *store;

  store = gtk_list_store_new (COL_HOST_LAST,
			      G_TYPE_STRING,
			      G_TYPE_STRING);
  return GTK_TREE_MODEL (store);
}

static GtkWidget*
popup_menu_create (GtkWidget *widget)
{
  GtkUIManager   *ui_manager;
  GtkActionGroup *action_group;
  GtkWidget      *popup;

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_add_actions (action_group, hosts_popup_menu_items, G_N_ELEMENTS (hosts_popup_menu_items), widget);

  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

  if (!gtk_ui_manager_add_ui_from_string (ui_manager, hosts_ui_description, -1, NULL))
    return NULL;

  g_object_set_data (G_OBJECT (widget), "ui-manager", ui_manager);
  popup = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");

  return popup;
}

static void
add_list_columns (GtkTreeView *list)
{
  GtkCellRenderer *renderer;

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (list, -1,
					       _("IP Address"),
					       renderer,
					       "text", COL_HOST_IP,
					       NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (list, -1,
					       _("Aliases"),
					       renderer,
					       "text", COL_HOST_ALIASES,
					       NULL);
}

GtkTreeView*
host_aliases_list_create (void)
{
  GtkWidget     *list;
  GstTablePopup *table_popup;
  GtkTreeModel  *model;

  list = gst_dialog_get_widget (tool->main_dialog, "host_aliases_list");

  model = host_aliases_model_create ();
  gtk_tree_view_set_model (GTK_TREE_VIEW (list), model);
  g_object_unref (model);

  add_list_columns (GTK_TREE_VIEW (list));

  table_popup = g_new0 (GstTablePopup, 1);
  table_popup->setup = NULL;
  table_popup->properties = on_host_aliases_properties_clicked;
  table_popup->popup = popup_menu_create (list);

  g_signal_connect (G_OBJECT (list), "button-press-event",
		    G_CALLBACK (on_table_button_press), (gpointer) table_popup);
  g_signal_connect (G_OBJECT (list), "popup_menu",
		    G_CALLBACK (on_table_popup_menu), (gpointer) table_popup);

  return GTK_TREE_VIEW (list);
}

static void
host_aliases_modify_at_iter (GtkTreeIter *iter,
			     const gchar *address,
			     const gchar *aliases)
{
  GtkTreeView  *list;
  GtkTreeModel *model;

  list = GST_NETWORK_TOOL (tool)->host_aliases_list;
  model = gtk_tree_view_get_model (list);

  gtk_list_store_set (GTK_LIST_STORE (model), iter,
		      COL_HOST_IP, address,
		      COL_HOST_ALIASES, aliases,
		      -1);
}

static void
host_aliases_add (const gchar *address, const gchar *aliases)
{
  GtkTreeView  *list;
  GtkTreeModel *model;
  GtkTreeIter   iter;

  list = GST_NETWORK_TOOL (tool)->host_aliases_list;
  model = gtk_tree_view_get_model (list);

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  host_aliases_modify_at_iter (&iter, address, aliases);
}

void
host_aliases_add_from_xml (xmlNodePtr node)
{
  xmlNodePtr  alias;
  gchar      *address, *str;
  GString    *aliases = NULL;
  
  address = gst_xml_get_child_content (node, "ip");

  for (alias = gst_xml_element_find_first (node, "alias");
       alias; alias = gst_xml_element_find_next (alias, "alias"))
    {
      str = gst_xml_element_get_content (alias);

      if (!aliases)
        aliases = g_string_new (str);
      else
        g_string_append_printf (aliases, " %s", str);

      g_free (str);
    }

  host_aliases_add (address, aliases->str);

  g_free (address);
  g_string_free (aliases, TRUE);
}

void
host_aliases_extract_to_xml (GtkTreeIter *iter, xmlNodePtr root)
{
  GtkTreeView  *list;
  GtkTreeModel *model;
  xmlNodePtr    host, alias;
  gchar        *address, *aliases, **arr, **str;

  g_return_if_fail (iter != NULL);
  g_return_if_fail (root != NULL);

  list = GST_NETWORK_TOOL (tool)->host_aliases_list;
  model = gtk_tree_view_get_model (list);
  
  gtk_tree_model_get (model, iter,
                      COL_HOST_IP, &address,
                      COL_HOST_ALIASES, &aliases,
                      -1);

  arr = g_strsplit (aliases, " ", -1);

  host = gst_xml_element_add (root, "statichost");
  gst_xml_set_child_content (host, "ip", address);

  for (str = arr; *str; str++)
    {
      alias = gst_xml_element_add (host, "alias");
      gst_xml_element_set_content (alias, *str);
    }

  g_free (address);
  g_free (aliases);
  g_strfreev (arr);
}

void
host_aliases_clear (void)
{
  GtkTreeView  *list;
  GtkTreeModel *model;

  list = GST_NETWORK_TOOL (tool)->host_aliases_list;
  model = gtk_tree_view_get_model (list);

  gtk_list_store_clear (GTK_LIST_STORE (model));
}

static void
host_aliases_dialog_prepare (GtkTreeIter *iter)
{
  GtkTreeView   *list;
  GtkTreeModel  *model;
  GtkWidget     *address, *aliases;
  GtkTextBuffer *buffer;
  gchar         *addr, *al, *str;

  address = gst_dialog_get_widget (tool->main_dialog, "host_alias_address");
  aliases = gst_dialog_get_widget (tool->main_dialog, "host_alias_list");
  buffer  = gtk_text_view_get_buffer (GTK_TEXT_VIEW (aliases));

  if (iter)
    {
      list = GST_NETWORK_TOOL (tool)->host_aliases_list;
      model = gtk_tree_view_get_model (list);

      gtk_tree_model_get (model, iter,
			  COL_HOST_IP, &addr,
			  COL_HOST_ALIASES, &al,
			  -1);

      for (str = (gchar *) strchr (al, ' '); str; str = (gchar *) strchr (str, ' '))
	*str = '\n';

      gtk_entry_set_text (GTK_ENTRY (address), addr);
      gtk_text_buffer_set_text (buffer, al, -1);

      g_free (addr);
      g_free (al);
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (address), "");
      gtk_text_buffer_set_text (buffer, "", -1);
    }
}

void
host_aliases_run_dialog (GtkTreeIter *iter)
{
  GtkWidget     *dialog, *address, *aliases;
  GtkTextBuffer *buffer;
  GtkTextIter    start, end;
  gchar *addr, *str, *list;
  gint   response;

  dialog = gst_dialog_get_widget (tool->main_dialog, "host_aliases_edit_dialog");
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (tool->main_dialog));

  host_aliases_dialog_prepare (iter);
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide (dialog);

  if (response == GTK_RESPONSE_OK)
    {
      address = gst_dialog_get_widget (tool->main_dialog, "host_alias_address");
      aliases = gst_dialog_get_widget (tool->main_dialog, "host_alias_list");
      buffer  = gtk_text_view_get_buffer (GTK_TEXT_VIEW (aliases));
      gtk_text_buffer_get_bounds (buffer, &start, &end);

      addr = (gchar *) gtk_entry_get_text (GTK_ENTRY (address));
      list = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);

      for (str = (gchar *) strchr (list, '\n'); str; str = (gchar *) strchr (str, '\n'))
	*str = ' ';

      if (!iter)
	host_aliases_add (addr, list);
      else
	host_aliases_modify_at_iter (iter, addr, list);

      gst_dialog_modify (tool->main_dialog);
      gtk_widget_hide (dialog);
      g_free (list);
    }
}
