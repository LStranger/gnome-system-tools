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

#include "gst-network-tool.h"
#include "location.h"
#include "ifaces-list.h"
#include "gst.h"

#define GST_LOCATION_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_LOCATION, GstLocationPrivate))
#define COMPARE_TAGS(VAL1, VAL2) (((VAL1 != NULL) && (VAL2 != NULL) && (strcmp (VAL1, VAL2) != 0)) || \
				  ((VAL1 == NULL) && (VAL2 != NULL) && (strlen (VAL2) > 0)) || \
				  ((VAL1 != NULL) && (VAL2 == NULL) && (strlen (VAL1) > 0)))

extern GstTool *tool;

typedef struct _GstLocationPrivate GstLocationPrivate;

struct _GstLocationPrivate
{
  GtkTreeRowReference *anchor;
  GtkTreeRowReference *delete_anchor;
  GtkTreeRowReference *last_selected;  /* meant to be used when cancelling */
  gint                 nitems;
};

enum {
  LOCATION_COL_TEXT,
  LOCATION_COL_ACTION,
  LOCATION_COL_SENSITIVE,
  LOCATION_COL_DATA,
  LOCATION_COL_LAST
};

enum {
  LOCATION_ACTION_NONE,
  LOCATION_ACTION_CHANGE_LOCATION,
  LOCATION_ACTION_ADD,
  LOCATION_ACTION_DELETE
};

static void gst_location_class_init (GstLocationClass *class);
static void gst_location_init       (GstLocation      *location);
static void gst_location_finalize   (GObject          *object);

static xmlNodePtr copy_current_to_profile         (const gchar *name);
static void       gst_location_combo_add_location (GstLocation*, xmlNodePtr);

static void replace_row_reference (GtkTreeRowReference**, GtkTreeModel*, GtkTreeIter*);
static void get_row_iter          (GtkTreeIter*, GtkTreeModel*, GtkTreeRowReference*);
static void on_combo_changed      (GstLocation*, gpointer);

static gpointer parent_class;

const gchar *general_tags[] = {
  "hostname", "domain", "smbdesc", "smbuse", "winsserver",
  "winsuse", "workgroup", "gateway", "gatewaydev", NULL
};

const gchar *interface_tags[] = {
  "address", "auto", "bootproto", "broadcast", "debug", "dial_command",
  "external_line", "file", "netmask", "network", "noauth", "persist",
  "phone_number", "section", "serial_hwctl", "serial_port", "set_default_gw",
  "update_dns", "volume", "user", NULL
};


GType
gst_location_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo location_info = {
	sizeof (GstLocationClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gst_location_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GstLocation),
	0,		/* n_preallocs */
	(GInstanceInitFunc) gst_location_init,
      };

      type = g_type_register_static (GTK_TYPE_COMBO_BOX, "GstLocation",
				     &location_info, 0);
    }

  return type;
}

static void
gst_location_class_init (GstLocationClass *class)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (class);
  parent_class = g_type_class_peek_parent (class);

  object_class->finalize = gst_location_finalize;

  g_type_class_add_private (object_class, sizeof (GstLocationPrivate));
}

static void
location_options_fill (GtkTreeModel *model, GstLocationPrivate *priv)
{
  GtkTreeIter  iter;

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model),
		      &iter,
		      LOCATION_COL_TEXT, NULL,
		      LOCATION_COL_ACTION, NULL,
		      LOCATION_COL_SENSITIVE, NULL,
		      LOCATION_COL_DATA, NULL,
		      -1);
  replace_row_reference (&priv->anchor, model, &iter);
  
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model),
		      &iter,
		      LOCATION_COL_TEXT, _("Create location"),
		      LOCATION_COL_ACTION, LOCATION_ACTION_ADD,
		      LOCATION_COL_SENSITIVE, TRUE,
		      LOCATION_COL_DATA, NULL,
		      -1);
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model),
		      &iter,
		      LOCATION_COL_TEXT, _("Delete current location"),
		      LOCATION_COL_ACTION, LOCATION_ACTION_DELETE,
		      LOCATION_COL_SENSITIVE, TRUE,
		      LOCATION_COL_DATA, NULL,
		      -1);
  replace_row_reference (&priv->delete_anchor, model, &iter);
}

static GtkTreeModel*
location_model_create (void)
{
  GtkListStore *store;

  store = gtk_list_store_new (LOCATION_COL_LAST,
			      G_TYPE_STRING,
			      G_TYPE_INT,
			      G_TYPE_BOOLEAN,
			      G_TYPE_POINTER);
  return GTK_TREE_MODEL (store);
}

static void
location_add_columns (GtkComboBox *combo)
{
  GtkCellRenderer *renderer;

  gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo),
				  renderer,
				  "text", LOCATION_COL_TEXT,
				  "sensitive", LOCATION_COL_SENSITIVE,
				  NULL);
}

static gboolean
row_separator_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  gchar    *text;
  gboolean  ret;

  gtk_tree_model_get (model, iter,
		      LOCATION_COL_TEXT, &text, -1);
  ret = (text == NULL);
  g_free (text);

  return ret;
}

static void
replace_interfaces (xmlNodePtr profile)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      valid, found;
  xmlNodePtr    iface_profile;
  gchar        *dev, *profile_dev;
  GstIface     *iface;

  model = GST_NETWORK_TOOL (tool)->interfaces_model;
  valid = gtk_tree_model_get_iter_first (model, &iter);

  while (valid)
    {
      gtk_tree_model_get (model,
			  &iter,
			  COL_DEV, &dev,
			  COL_OBJECT, &iface,
			  -1);

      iface_profile = gst_xml_element_find_first (profile, "interface");
      found = FALSE;

      while (iface_profile && !found)
        {
	  profile_dev = gst_xml_get_child_content (iface_profile, "dev");
	  found = (strcmp (dev, profile_dev) == 0);
	  g_free (profile_dev);

	  if (!found)
	    iface_profile = gst_xml_element_find_next (iface_profile, "interface");
	}

      if (found)
        {
	  ifaces_model_set_interface_from_node_at_iter (iface_profile, &iter);

	  g_object_unref (iface);
	  gtk_tree_model_get (model, &iter, COL_OBJECT, &iface, -1);
	}
      else
        {
	  gst_iface_set_configured (iface, FALSE);
	  ifaces_model_set_interface_at_iter (iface, &iter);
	}

      g_free (dev);
      g_object_unref (iface);
      valid = gtk_tree_model_iter_next (model, &iter);
    }
  
}

static void
replace_row_reference (GtkTreeRowReference **reference, GtkTreeModel *model, GtkTreeIter *iter)
{
  GtkTreePath *path;

  if (*reference)
    gtk_tree_row_reference_free (*reference);

  path = gtk_tree_model_get_path (model, iter);
  *reference = gtk_tree_row_reference_new (model, path);
  gtk_tree_path_free (path);
}

static void
get_row_iter (GtkTreeIter *iter, GtkTreeModel *model, GtkTreeRowReference *reference)
{
  GtkTreePath *path;

  if (!reference)
    return;

  path = gtk_tree_row_reference_get_path (reference);
  gtk_tree_model_get_iter (model, iter, path);
  gtk_tree_path_free (path);
}

/* puts the delete button unsensitive if necessary */
static void
check_delete_button_sensitivity (GstLocation *location)
{
  GstLocationPrivate *priv;
  GtkTreeModel       *model;
  GtkTreeIter         iter;
  gint                selected;

  selected = gtk_combo_box_get_active (GTK_COMBO_BOX (location));
  model    = gtk_combo_box_get_model (GTK_COMBO_BOX (location));
  priv     = GST_LOCATION_GET_PRIVATE (location);

  get_row_iter (&iter, model, priv->delete_anchor);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      LOCATION_COL_SENSITIVE, (priv->nitems > 0 && selected != -1),
		      -1);
}

static void
add_location (GstLocation *location)
{
  GstLocationPrivate *priv;
  GtkWidget    *dialog, *entry;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gint          response;
  xmlNodePtr    profile;

  priv   = GST_LOCATION_GET_PRIVATE (location);
  dialog = gst_dialog_get_widget (tool->main_dialog, "location_dialog");
  entry  = gst_dialog_get_widget (tool->main_dialog, "location_entry");
  model  = gtk_combo_box_get_model (GTK_COMBO_BOX (location));

  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (tool->main_dialog));
  gtk_widget_grab_focus (GTK_WIDGET (entry));
  
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide (dialog);

  if (response == GTK_RESPONSE_OK)
    {
      profile = copy_current_to_profile (gtk_entry_get_text (GTK_ENTRY (entry)));

      gst_location_combo_add_location (location, profile);
      check_delete_button_sensitivity (location);
      gst_dialog_modify (tool->main_dialog);
    }
  else
    {
      if (priv->last_selected)
        {
	  get_row_iter (&iter, model, priv->last_selected);
	  g_signal_handlers_block_by_func (G_OBJECT (location), on_combo_changed, NULL);
	  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (location), &iter);
	  g_signal_handlers_unblock_by_func (G_OBJECT (location), on_combo_changed, NULL);
	}
      else
	gtk_combo_box_set_active (GTK_COMBO_BOX (location), -1);
    }

  gtk_entry_set_text (GTK_ENTRY (entry), "");
}

static void
delete_location (GstLocation *location)
{
  GstLocationPrivate *priv;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GtkWidget    *dialog;
  gint          response;

  priv  = GST_LOCATION_GET_PRIVATE (location);
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (location));
  get_row_iter (&iter, model, priv->last_selected);

  dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
				   GTK_DIALOG_MODAL,
				   GTK_MESSAGE_WARNING,
				   GTK_BUTTONS_YES_NO,
				   _("Do you want to remove this location?"),
				   NULL);
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  if (response == GTK_RESPONSE_YES)
    {
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
      gtk_combo_box_set_active (GTK_COMBO_BOX (location), -1);
      gst_dialog_modify (tool->main_dialog);
    }
  else
    {
      if (priv->last_selected)
        {
	  g_signal_handlers_block_by_func (G_OBJECT (location), on_combo_changed, NULL);
	  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (location), &iter);
	  g_signal_handlers_unblock_by_func (G_OBJECT (location), on_combo_changed, NULL);
	}
      else
	gtk_combo_box_set_active (GTK_COMBO_BOX (location), -1);
    }

  check_delete_button_sensitivity (location);
}

static void
save_profiles (GstTool *tool)
{
  xmlNodePtr  root, profiledb, root_copy, copy;
  xmlDoc     *doc;

  doc  = gst_xml_doc_create ("network");
  root_copy = gst_xml_doc_get_root (doc);

  root = gst_xml_doc_get_root (tool->config);
  profiledb = gst_xml_element_find_first (root, "profiledb");

  copy = xmlDocCopyNode (profiledb, doc, 1);
  gst_xml_element_add_child (root_copy, copy);

  gst_tool_run_set_directive (tool, doc, NULL, "save_profiles", NULL);
  gst_xml_doc_destroy (doc);
}

static void
change_location (GstLocation *location)
{
  GstLocationPrivate *priv;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  xmlNodePtr    profile;
  gchar        *name;

  priv  = GST_LOCATION_GET_PRIVATE (location);
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (location));

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (location), &iter))
    {
      replace_row_reference (&priv->last_selected, model, &iter);
      gtk_tree_model_get (model,
			  &iter,
			  LOCATION_COL_TEXT, &name,
			  LOCATION_COL_DATA, &profile,
			  -1);
      save_profiles (tool);
      gst_tool_run_set_directive (tool, NULL, _("Changing profile"),
				  "set_profile", name, NULL);

      replace_interfaces (profile);
      transfer_xml_profile_to_gui (tool, profile);

      check_delete_button_sensitivity (location);
      gst_dialog_modify (tool->main_dialog);
      g_free (name);
    }
}

static void
on_combo_changed (GstLocation *location, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gint          action;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (location));

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (location), &iter))
    return;

  gtk_tree_model_get (model, &iter,
		      LOCATION_COL_ACTION, &action,
		      -1);
  if (action == LOCATION_ACTION_ADD)
    add_location (location);
  else if (action == LOCATION_ACTION_DELETE)
    delete_location (location);
  else if (action == LOCATION_ACTION_CHANGE_LOCATION)
    change_location (location);
}

static void
gst_location_init (GstLocation *location)
{
  GstLocationPrivate *priv;
  GtkTreeModel       *model;
  GtkTreeIter         iter;

  g_return_if_fail (GST_IS_LOCATION (location));

  location_add_columns (GTK_COMBO_BOX (location));

  priv = GST_LOCATION_GET_PRIVATE (location);
  model = location_model_create ();

  priv->nitems = 0;
  priv->anchor        = NULL;
  priv->delete_anchor = NULL;
  priv->last_selected = NULL;
  location_options_fill (model, priv);

  gtk_combo_box_set_model (GTK_COMBO_BOX (location), model);
  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (location),
					row_separator_func, NULL, NULL);

  g_signal_connect (G_OBJECT (location), "changed",
		    G_CALLBACK (on_combo_changed), NULL);
}

static void
gst_location_finalize (GObject *object)
{
  GstLocation        *location;
  GstLocationPrivate *priv;

  location = GST_LOCATION (object);
  priv = GST_LOCATION_GET_PRIVATE (location);

  gtk_tree_row_reference_free (priv->anchor);
  gtk_tree_row_reference_free (priv->delete_anchor);
  gtk_tree_row_reference_free (priv->last_selected);
  
  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

GstLocation*
gst_location_combo_new (void)
{
  GstLocation *location;

  location = g_object_new (GST_TYPE_LOCATION, NULL);
  return location;
}

static void
gst_location_combo_add_location (GstLocation *location, xmlNodePtr profile)
{
  GstLocationPrivate *priv;
  gchar              *name;
  GtkTreeIter         iter, anchor;
  GtkTreeModel       *model;

  g_signal_handlers_block_by_func (G_OBJECT (location), on_combo_changed, NULL);
  priv = GST_LOCATION_GET_PRIVATE (location);
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (location));
  name = gst_xml_get_child_content (profile, "name");

  get_row_iter (&anchor, model, priv->anchor);
  gtk_list_store_insert_before (GTK_LIST_STORE (model),
				&iter, &anchor);
  gtk_list_store_set (GTK_LIST_STORE (model),
		      &iter,
		      LOCATION_COL_TEXT, name,
		      LOCATION_COL_ACTION, LOCATION_ACTION_CHANGE_LOCATION,
		      LOCATION_COL_SENSITIVE, TRUE,
		      LOCATION_COL_DATA, profile,
		      -1);
  priv->nitems++;
  g_free (name);

  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (location), &iter);
  replace_row_reference (&priv->last_selected, model, &iter);
  g_signal_handlers_unblock_by_func (G_OBJECT (location), on_combo_changed, NULL);
}

/* bunch of functions for comparing profiles */
static gboolean
compare_tag_list (xmlNodePtr current, xmlNodePtr profile, const gchar *list[])
{
  gchar    **string = (gchar **) list;
  gchar     *value1, *value2;
  gboolean   value = TRUE;
       
  while (*string && value)
    {
      value1 = gst_xml_get_child_content (current, *string);
      value2 = gst_xml_get_child_content (profile, *string);

      if (COMPARE_TAGS (value1, value2))
	value = FALSE;

      g_free (value1);
      g_free (value2);
      string++;
    } 

  return value;
}

static gboolean
compare_list (xmlNodePtr current, xmlNodePtr profile, gchar *tag)
{
  xmlNodePtr  node1, node2;
  gchar      *value1, *value2;
  gboolean    value = TRUE;

  node1 = gst_xml_element_find_first (current, tag);
  node2 = gst_xml_element_find_first (profile, tag);

  while (node1 && node2 && value)
    {
      value1 = gst_xml_element_get_content (node1);
      value2 = gst_xml_element_get_content (node2);

      if (COMPARE_TAGS (value1, value2))
	value = FALSE;

      g_free (value1);
      g_free (value2);

      node1 = gst_xml_element_find_next (node1, tag);
      node2 = gst_xml_element_find_next (node2, tag);
    } 

  return value;
}

static gboolean
compare_statichosts (xmlNodePtr current, xmlNodePtr profile)
{
  xmlNodePtr  node1, node2, alias_node1, alias_node2;
  gchar      *ip1, *ip2;
  gboolean    value = TRUE;

  node1 = gst_xml_element_find_first (current, "statichost");
  node2 = gst_xml_element_find_first (profile, "statichost");

  while (node1 && node2 && value)
    {
      ip1 = gst_xml_get_child_content (node1, "ip");
      ip2 = gst_xml_get_child_content (node2, "ip");

      if (COMPARE_TAGS (ip1, ip2))
	value = FALSE;
      else
	value = compare_list (node1, node2, "alias");

      g_free (ip1);
      g_free (ip2);

      node1 = gst_xml_element_find_next (node1, "statichost");
      node2 = gst_xml_element_find_next (node2, "statichost");
    }

  return value;
}

static gboolean
compare_general_data (xmlNodePtr current, xmlNodePtr profile)
{
  compare_tag_list (current, profile, general_tags);
}

static gboolean
compare_interfaces (xmlNodePtr current, xmlNodePtr profile)
{
  xmlNodePtr  iface_current, iface_profile;
  gchar      *dev_name, *profile_dev_name;
  gboolean    found, value;

  value = TRUE;

  for (iface_current = gst_xml_element_find_first (current, "interface");
       iface_current && value; iface_current = gst_xml_element_find_next (iface_current, "interface"))
    {
      dev_name      = gst_xml_get_child_content (iface_current, "dev");
      iface_profile = gst_xml_element_find_first (profile, "interface");
      found = FALSE;

      while (iface_profile && !found)
        {
	  profile_dev_name = gst_xml_get_child_content (iface_profile, "dev");
	  found = (strcmp (dev_name, profile_dev_name) == 0);
	  g_free (profile_dev_name);

	  if (!found)
	    iface_profile = gst_xml_element_find_next (iface_profile, "interface");
	}

      if (found)
	value = compare_tag_list (gst_xml_element_find_first (iface_current, "configuration"),
				  gst_xml_element_find_first (iface_profile, "configuration"),
				  interface_tags);
      else
	value = FALSE;
      
      g_free (dev_name);
    }

  return value;
}

static gboolean
compare_current_with_profile (xmlNodePtr current, xmlNodePtr profile)
{
  return (compare_general_data (current, profile) &&
	  compare_list (current, profile, "nameserver") &&
	  compare_list (current, profile, "searchdomain") &&
	  compare_list (current, profile, "order") &&
	  compare_statichosts (current, profile) &&
	  compare_interfaces (current, profile));
}

/* bunch of functions for profile saving */
static void
copy_tag_list (xmlNodePtr source, xmlNodePtr dest, const gchar *list[])
{
  gchar **string = (gchar **) list;;
  gchar  *value;

  while (*string)
    {
      value = gst_xml_get_child_content (source, *string);

      if (value && *value)
	gst_xml_element_add_with_content (dest, *string, value);

      g_free (value);
      string++;
    }
}

static void
copy_list (xmlNodePtr source, xmlNodePtr dest, const gchar *tag)
{
  xmlNodePtr  node, element;
  gchar      *value;

  for (node = gst_xml_element_find_first (source, tag);
       node; node = gst_xml_element_find_next (node, tag))
    {
      value   = gst_xml_element_get_content (node);
      element = gst_xml_element_add (dest, tag);

      gst_xml_element_set_content (element, value);
      g_free (value);
    }
}

static void
copy_statichosts (xmlNodePtr source, xmlNodePtr dest)
{
  xmlNodePtr  node, new_statichost, alias, new_alias;
  gchar      *value;

  for (node = gst_xml_element_find_first (source, "statichost");
       node; node = gst_xml_element_find_next (node, "statichost"))
    {
      new_statichost = gst_xml_element_add (dest, "statichost");
      value = gst_xml_get_child_content (node, "ip");

      gst_xml_element_add_with_content (new_statichost, "ip", value);
      copy_list (node, new_statichost, "alias");
      g_free (value);
    }
}

static void
copy_general_data (xmlNodePtr source, xmlNodePtr dest)
{
  copy_tag_list (source, dest, general_tags);
}

static void
copy_interfaces (xmlNodePtr source, xmlNodePtr dest)
{
  xmlNodePtr node, new_interface, source_config, dest_config;
  const gchar *list[] = { "dev", "enabled", NULL };
  gchar *type;
  
  for (node = gst_xml_element_find_first (source, "interface");
       node; node = gst_xml_element_find_next (node, "interface"))
    {
      new_interface = gst_xml_element_add (dest, "interface");
      copy_tag_list (node, new_interface, list);

      type = gst_xml_element_get_attribute (node, "type");
      if (type)
        {
	  gst_xml_element_set_attribute (new_interface, "type", type);
	  g_free (type);
	}

      source_config = gst_xml_element_find_first (node, "configuration");

      if (source_config)
        {
	  dest_config   = gst_xml_element_add (new_interface, "configuration");
	  copy_tag_list (source_config, dest_config, interface_tags);
	}
    }
}

static xmlNodePtr
copy_current_to_profile (const gchar *name)
{
  xmlNodePtr root = gst_xml_doc_get_root (tool->config);
  xmlNodePtr profiledb = gst_xml_element_find_first (root, "profiledb");
  xmlNodePtr new_profile = gst_xml_element_add (profiledb, "profile");

  /* first of all, we sync the content of the dialogs with the XML */
  transfer_gui_to_xml (tool, NULL);

  /* save name */
  gst_xml_element_add_with_content (new_profile, "name", name);

  /* store the current configuration */
  copy_general_data (root, new_profile);
  copy_list (root, new_profile, "nameserver");
  copy_list (root, new_profile, "searchdomain");
  copy_statichosts (root, new_profile);
  copy_interfaces (root, new_profile);

  return new_profile;
}

void
gst_location_combo_setup (GstLocation *location, xmlNodePtr root)
{
  xmlNodePtr profiles, node;
  gint       index, count;

  index = -1;
  count = 0;
  profiles = gst_xml_element_find_first (root, "profiledb");

  for (node = gst_xml_element_find_first (profiles, "profile");
       node; node = gst_xml_element_find_next (node, "profile"))
    {
      gst_location_combo_add_location (location, node);

      if (compare_current_with_profile (root, node))
	index = count;

      count++;
    }

  gtk_combo_box_set_active (GTK_COMBO_BOX (location), index);
  check_delete_button_sensitivity (location);
}
