/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdkpixbuf.h>
#include "gst.h"
#include "callbacks.h"
#include "ifaces-list.h"
#include "network-tool.h"

extern GstTool *tool;

GtkActionEntry popup_menu_items [] = {
  { "Properties",  GTK_STOCK_PROPERTIES, N_("_Properties"), NULL, NULL, G_CALLBACK (on_iface_properties_clicked) },
  { "Activate",    GTK_STOCK_EXECUTE,    N_("_Activate"),   NULL, NULL, G_CALLBACK (on_activate_button_clicked) },
  { "Deactivate",  GTK_STOCK_STOP,       N_("_Deactivate"), NULL, NULL, G_CALLBACK (on_deactivate_button_clicked) }
};

const gchar *ui_description =
  "<ui>"
  "  <popup name='MainMenu'>"
  "    <menuitem action='Properties'/>"
  "  </popup>"
  "</ui>";

GtkTreeModel*
ifaces_model_create (void)
{
  GtkListStore *store;

  store = gtk_list_store_new (COL_LAST,
			      G_TYPE_BOOLEAN,
			      GDK_TYPE_PIXBUF,
			      G_TYPE_STRING,
			      G_TYPE_OBJECT,
			      G_TYPE_STRING,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN);
  return GTK_TREE_MODEL (store);
}

static gboolean
gateways_filter_func (GtkTreeModel *model,
		      GtkTreeIter  *iter,
		      gpointer      data)
{
  gboolean inconsistent, active, has_gateway;

  gtk_tree_model_get (model, iter,
		      COL_INCONSISTENT, &inconsistent,
		      COL_ACTIVE, &active,
		      COL_HAS_GATEWAY, &has_gateway,
		      -1);

  return !inconsistent && active && has_gateway;
}


GtkTreeModelFilter*
gateways_filter_model_create (GtkTreeModel *model)
{
  GtkTreeModelFilter *filter_model;

  filter_model = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (model, NULL));

  gtk_tree_model_filter_set_visible_func (filter_model,
					  gateways_filter_func,
					  NULL,
					  NULL);
  return filter_model;
}

static GtkWidget*
popup_menu_create (GtkWidget *widget)
{
  GtkUIManager   *ui_manager;
  GtkActionGroup *action_group;
  GtkWidget      *popup;

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, NULL);
  gtk_action_group_add_actions (action_group, popup_menu_items, G_N_ELEMENTS (popup_menu_items), widget);

  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

  if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL))
    return NULL;

  g_object_set_data (G_OBJECT (widget), "ui-manager", ui_manager);
  popup = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");

  return popup;
}

static gint
iface_to_priority (OobsIface *iface)
{
  if (OOBS_IS_IFACE_WIRELESS (iface))
    return 5;
  else if (OOBS_IS_IFACE_IRLAN (iface))
    return 2;
  else if (OOBS_IS_IFACE_ETHERNET (iface))
    return 4;
  else if (OOBS_IS_IFACE_MODEM (iface) || OOBS_IS_IFACE_ISDN (iface))
    return 3;
  else
    return 1;
}

static gint
ifaces_list_sort (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer data)
{
  OobsIface *iface_a, *iface_b;

  gtk_tree_model_get (model, a, COL_OBJECT, &iface_a, -1);
  gtk_tree_model_get (model, b, COL_OBJECT, &iface_b, -1);

  return (iface_to_priority (iface_a) - iface_to_priority (iface_b));
}

static void
add_list_columns (GtkTreeView  *table,
		  GtkTreeModel *model)
{
  GtkCellRenderer *renderer;

  renderer = gtk_cell_renderer_toggle_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table), -1,
					       "Enable",
					       renderer,
					       "active", COL_ACTIVE,
					       "inconsistent", COL_INCONSISTENT,
					       NULL);
  g_signal_connect (G_OBJECT (renderer), "toggled",
		    G_CALLBACK (on_iface_toggled), model);
  
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table), -1,
					       "Interface",
					       renderer,
					       "pixbuf", COL_IMAGE,
					       NULL);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table), -1,
					       "Interface",
					       renderer,
					       "markup", COL_DESC,
					       NULL);
}

GtkTreeView*
ifaces_list_create (GstTool *tool)
{
  GtkWidget        *table = gst_dialog_get_widget (tool->main_dialog, "interfaces_list");
  GstTablePopup    *table_popup;
  GtkTreeSelection *selection;
  GtkTreeModel     *model;

  model = GST_NETWORK_TOOL (tool)->interfaces_model;
  gtk_tree_view_set_model (GTK_TREE_VIEW (table), model);

  gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (GTK_LIST_STORE (model)),
					   ifaces_list_sort,
					   NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (GTK_LIST_STORE (model)),
					GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					GTK_SORT_DESCENDING);

  add_list_columns (GTK_TREE_VIEW (table), model);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (on_table_selection_changed), NULL);

  table_popup = g_new0 (GstTablePopup, 1);
  table_popup->properties = on_iface_properties_clicked;
  table_popup->popup = popup_menu_create (table);

  g_signal_connect (G_OBJECT (table), "button-press-event",
		    G_CALLBACK (on_table_button_press), (gpointer) table_popup);
  g_signal_connect (G_OBJECT (table), "popup_menu",
		    G_CALLBACK (on_table_popup_menu), (gpointer) table_popup);

  return GTK_TREE_VIEW (table);
}

const gchar*
iface_to_type (OobsIface *iface)
{
  if (OOBS_IS_IFACE_WIRELESS (iface))
    return "wireless";
  else if (OOBS_IS_IFACE_IRLAN (iface))
    return "irlan";
  else if (OOBS_IS_IFACE_ETHERNET (iface))
    return "ethernet";
  else if (OOBS_IS_IFACE_PLIP (iface))
    return "plip";
  else if (OOBS_IS_IFACE_MODEM (iface))
    return "modem";
  else if (OOBS_IS_IFACE_ISDN (iface))
    return "isdn";

  return "unknown";
}

OobsIface*
ifaces_model_search_iface (IfaceSearchTerm search_term, const gchar *term)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      valid;
  gchar        *dev, *item;
  OobsIface    *iface = NULL;

  g_return_val_if_fail (term != NULL, NULL);

  model = GST_NETWORK_TOOL (tool)->interfaces_model;
  valid = gtk_tree_model_get_iter_first (model, &iter);

  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  COL_OBJECT, &iface,
			  COL_DEV,    &dev,
			  -1);

      if (search_term == SEARCH_DEV)
	item = dev;
      else
	item = (gchar *) iface_to_type (iface);

      if (strcmp (term, item) == 0)
	valid = FALSE;
      else
        {
          valid = gtk_tree_model_iter_next (model, &iter);
	  g_object_unref (iface);
	  iface = NULL;
	}

      g_free (dev);
    }

  return iface;
}

static void
update_gateways_combo (void)
{
  GtkTreeModel *model;
  gint          count;

  /* refilter the gateways model */
  /* FIXME
  gtk_tree_model_filter_refilter (GST_NETWORK_TOOL (tool)->gateways_model);

  model = GTK_TREE_MODEL (GST_NETWORK_TOOL (tool)->gateways_model);
  count = gtk_tree_model_iter_n_children (model, NULL);

  gtk_widget_set_sensitive (GTK_WIDGET (GST_NETWORK_TOOL (tool)->gateways_list), (count > 0));
  gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "gateways_combo_label"), (count > 0));
  */
}

static GdkPixbuf*
get_iface_pixbuf (OobsIface *iface)
{
  GdkPixbuf *pixbuf = NULL;

  if (OOBS_IS_IFACE_WIRELESS (iface))
    pixbuf = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/wavelan-48.png", NULL);
  else if (OOBS_IS_IFACE_IRLAN (iface))
    pixbuf = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/irda-48.png", NULL);
  else if (OOBS_IS_IFACE_ETHERNET (iface))
    pixbuf = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/connection-ethernet.png", NULL);
  else if (OOBS_IS_IFACE_PLIP (iface))
    pixbuf = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/plip-48.png", NULL);
  else if (OOBS_IS_IFACE_ISDN (iface))
    pixbuf = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/ppp.png", NULL);

  return pixbuf;
}

static gchar*
get_iface_secondary_text (OobsIface *iface)
{
  GString *str;
  gchar *text;

  str = g_string_new ("");

  if (!oobs_iface_get_configured (iface))
    str = g_string_append (str, _("This network interface is not configured"));
  else if (OOBS_IS_IFACE_ETHERNET (iface))
    {
      if (OOBS_IS_IFACE_WIRELESS (iface))
	g_string_append_printf (str, _("<b>Essid:</b> %s "),
				oobs_iface_wireless_get_essid (iface));

      if (oobs_iface_ethernet_get_configuration_method (iface) != OOBS_METHOD_DHCP)
	g_string_append_printf (str, _("<b>Address:</b> %s <b>Subnet mask:</b> %s"),
				oobs_iface_ethernet_get_ip_address (iface),
				oobs_iface_ethernet_get_network_mask (iface));
      else
	g_string_append_printf (str, _("<b>Address:</b> DHCP"));
    }
  else if (OOBS_IS_IFACE_PLIP (iface))
    {
      g_string_append_printf (str, _("<b>Address:</b> %s <b>Remote address:</b> %s"),
			      oobs_iface_plip_get_address (iface),
			      oobs_iface_plip_get_remote_address (iface));
    }
  else if (OOBS_IS_IFACE_ISDN (iface))
    {
      g_string_append_printf (str, _("<b>Phone number:</b> %s <b>Login:</b> %s"),
			      oobs_iface_isdn_get_phone_number (iface),
			      oobs_iface_isdn_get_login (iface));
    }

  text = str->str;
  g_string_free (str, FALSE);

  return text;
}

static gchar*
get_iface_desc (OobsIface *iface, gboolean show_name)
{
  gchar *primary_text, *secondary_text, *full_text;

  primary_text = secondary_text = full_text = NULL;

  if (OOBS_IS_IFACE_WIRELESS (iface))
    primary_text = N_("Wireless connection");
  else if (OOBS_IS_IFACE_IRLAN (iface))
    primary_text = N_("Infrared connection");
  else if (OOBS_IS_IFACE_ETHERNET (iface))
    primary_text = N_("Wired connection");
  else if (OOBS_IS_IFACE_PLIP (iface))
    primary_text = N_("Parallel port connection");
  else if (OOBS_IS_IFACE_MODEM (iface))
    primary_text = N_("Modem connection");
  else if (OOBS_IS_IFACE_ISDN (iface))
    primary_text = N_("ISDN connection");

  secondary_text = get_iface_secondary_text (iface);

  if (primary_text)
    {
      if (show_name)
	full_text = g_strdup_printf ("<span size=\"larger\" weight=\"bold\">%s (%s)</span>\n%s",
				     _(primary_text), oobs_iface_get_device_name (iface), secondary_text);
      else
	full_text = g_strdup_printf ("<span size=\"larger\" weight=\"bold\">%s</span>\n%s", _(primary_text), secondary_text);
    }
  else
    full_text = g_strdup_printf ("Unknown interface type");

  g_free (secondary_text);
  return full_text;
}

void
ifaces_model_modify_interface_at_iter (GtkTreeIter *iter)
{
  GtkTreeModel *model;
  OobsIface *iface;
  gchar *desc;
  gboolean show_name;

  model = GST_NETWORK_TOOL (tool)->interfaces_model;

  gtk_tree_model_get (model, iter,
		      COL_OBJECT, &iface,
		      COL_SHOW_IFACE_NAME, &show_name,
		      -1);
  desc = get_iface_desc (OOBS_IFACE (iface), show_name);

  gtk_list_store_set (GTK_LIST_STORE (model), iter,
		      COL_ACTIVE, oobs_iface_get_active (OOBS_IFACE (iface)),
		      COL_IMAGE, get_iface_pixbuf (OOBS_IFACE (iface)),
		      COL_DESC, desc,
		      COL_DEV, oobs_iface_get_device_name (OOBS_IFACE (iface)),
		      COL_INCONSISTENT, !oobs_iface_get_configured (OOBS_IFACE (iface)),
		      /* FIXME
		      COL_HAS_GATEWAY, gst_iface_has_gateway (GST_IFACE (iface)),
		      */
		      -1);
  g_object_unref (iface);
  g_free (desc);

  update_gateways_combo ();
}

static void
ifaces_model_set_interface_at_iter (OobsIface *iface, GtkTreeIter *iter, gboolean show_name)
{
  GtkTreeModel *model;

  g_return_if_fail (iface != NULL);

  model = GST_NETWORK_TOOL (tool)->interfaces_model;

  gtk_list_store_set (GTK_LIST_STORE (model), iter,
		      COL_OBJECT, iface,
		      COL_SHOW_IFACE_NAME, show_name,
		      -1);
  ifaces_model_modify_interface_at_iter (iter);
}

void
ifaces_model_add_interface (OobsIface *iface, gboolean show_name)
{
  GtkTreeModel *model;
  GtkTreeIter   it;

  model = GST_NETWORK_TOOL (tool)->interfaces_model;

  gtk_list_store_append (GTK_LIST_STORE (model), &it);
  ifaces_model_set_interface_at_iter (iface, &it, show_name);
}

void
ifaces_model_clear (void)
{
  GtkTreeModel *model;

  model = GST_NETWORK_TOOL (tool)->interfaces_model;
  gtk_list_store_clear (GTK_LIST_STORE (model));
}

static void
add_combo_layout (GtkComboBox *combo)
{
  GtkCellRenderer *renderer;

  gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo),
			      renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo),
				  renderer,
				  "markup", COL_DEV,
				  "visible", COL_HAS_GATEWAY,
				  NULL);
  g_object_unref (renderer);
}

GtkComboBox*
gateways_combo_create (void)
{
  GtkWidget    *combo = gst_dialog_get_widget (tool->main_dialog, "gateways_combo");
  GtkTreeModel *model;

  model = GTK_TREE_MODEL (GST_NETWORK_TOOL (tool)->gateways_model);
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo), model);
  g_object_unref (model);

  add_combo_layout (GTK_COMBO_BOX (combo));

  return GTK_COMBO_BOX (combo);
}

void
gateways_combo_select (gchar *dev)
{
  GtkComboBox  *combo = GST_NETWORK_TOOL (tool)->gateways_list;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      valid;
  gchar        *iter_dev;

  g_return_if_fail (dev != NULL);

  /* block/unblock the combobox changed signal */
  g_signal_handlers_block_by_func (G_OBJECT (combo),
				   G_CALLBACK (on_gateway_combo_changed), tool->main_dialog);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
  valid = gtk_tree_model_get_iter_first (model, &iter);

  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  COL_DEV, &iter_dev,
			  -1);

      if (iter_dev && (strcmp (dev, iter_dev) == 0))
        {
	  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
	  valid = FALSE; /* just exit */
	}
      else
	valid = gtk_tree_model_iter_next (model, &iter);

      g_free (iter_dev);
    }

  g_signal_handlers_unblock_by_func (G_OBJECT (combo),
				     G_CALLBACK (on_gateway_combo_changed), tool->main_dialog);
}

gchar*
gateways_combo_get_selected (void)
{
  GtkComboBox  *combo;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar        *gatewaydev = NULL;

  combo = GST_NETWORK_TOOL (tool)->gateways_list;
  model = gtk_combo_box_get_model (combo);

  if (gtk_combo_box_get_active_iter (combo, &iter))
    gtk_tree_model_get (model, &iter, COL_DEV, &gatewaydev, -1);

  return gatewaydev;
}
