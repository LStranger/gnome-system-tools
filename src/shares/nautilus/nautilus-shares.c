/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* nautilus-gst-shares.c: this file is part of shares-admin, a gnome-system-tool frontend 
 * for shared folders administration.
 * 
 * Copyright (C) 2004 Carlos Garnacho
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
 * Authors: Carlos Garnacho Parro <carlosg@gnome.org>.
 */

#include <config.h>
#include <glib/gi18n-lib.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-file-info.h>
#include <libnautilus-extension/nautilus-menu-provider.h>
#include <libnautilus-extension/nautilus-info-provider.h>
#include "nautilus-shares.h"

static GType type = 0;

static gboolean
is_directory_local (NautilusFileInfo *info)
{
  GnomeVFSURI *uri;
  gchar       *str;
  gboolean     is_local;

  str = nautilus_file_info_get_uri (info);
  uri = gnome_vfs_uri_new (str);

  is_local = gnome_vfs_uri_is_local (uri);

  gnome_vfs_uri_unref (uri);
  g_free (str);

  return is_local;
}

static char *
get_path_from_file_info (NautilusFileInfo *info)
{
  GnomeVFSURI *uri     = NULL;
  gchar       *escaped = NULL;
  gchar       *path    = NULL;

  uri = gnome_vfs_uri_new (nautilus_file_info_get_uri (info));

  if (uri)
    {
      escaped = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD);

      if (escaped)
	path = gnome_vfs_unescape_string (escaped, NULL);
	
      gnome_vfs_uri_unref (uri);
      g_free (escaped);
    }

  return path;
}

static void
on_menu_item_activate (NautilusMenuItem *menu_item,
		       gpointer          data)
{
  NautilusFileInfo *info;
  GString *cmd;
  gchar *dir;

  cmd  = g_string_new ("shares-admin ");
  info = g_object_get_data (G_OBJECT (menu_item), "file");
  dir  = get_path_from_file_info (info);

  g_string_append_printf (cmd, "--add-share=\"%s\"", dir);

  g_spawn_command_line_async (cmd->str, NULL);

  g_string_free (cmd, TRUE);
  g_free (dir);
}

static GList*
get_file_items (NautilusMenuProvider *provider,
		GtkWidget            *window,
		GList                *files)
{
  gboolean one_item, is_local, is_dir;
  NautilusFileInfo *info;
  NautilusMenuItem *menu_item;
  GList            *items = NULL;

  one_item = (files && !files->next);

  if (!one_item)
    return NULL;
  
  info = files->data;
  is_dir = nautilus_file_info_is_directory (info);

  if (!is_dir)
    return NULL;

  is_local = is_directory_local (info);

  if (!is_local)
    return NULL;

  menu_item = nautilus_menu_item_new ("NautilusShares::share",
				      _("_Share folder"),
				      _("Share this folder with other computers"),
				      "gnome-fs-share");
  g_signal_connect (G_OBJECT (menu_item),
		    "activate",
		    G_CALLBACK (on_menu_item_activate), NULL);
  g_object_set_data (G_OBJECT (menu_item),
		     "file", info);

  return g_list_append (NULL, menu_item);
}

static void
menu_provider_iface_init (NautilusMenuProviderIface *iface)
{
  iface->get_file_items = get_file_items;
}

static gboolean
file_get_share_status_file (NautilusShares   *shares,
			    NautilusFileInfo *file)
{
  char *path;
  gboolean status;

  g_return_val_if_fail (path != NULL, FALSE);

  if (!nautilus_file_info_is_directory(file) || !is_directory_local (file))
    status = FALSE;
  else
    {
      path = get_path_from_file_info (file);
      g_return_val_if_fail (path != NULL, FALSE);

      status = (g_hash_table_lookup (shares->paths, path) != NULL);
      g_free (path);
    }

  return status;
}

static NautilusOperationResult
nautilus_share_update_file_info (NautilusInfoProvider *provider,
                                 NautilusFileInfo *file,
                                 GClosure *update_complete,
                                 NautilusOperationHandle **handle)
{
  NautilusShares *shares;

  shares = NAUTILUS_SHARES (provider);

  if (file_get_share_status_file (shares, file))
    nautilus_file_info_add_emblem (file, "shared");

  return NAUTILUS_OPERATION_COMPLETE;
}

static void
info_provider_iface_init (NautilusInfoProviderIface *iface)
{
  iface->update_file_info = nautilus_share_update_file_info;
}

GType
nautilus_shares_get_type (void)
{
  return type;
}

static void
add_paths (GHashTable *paths,
	   OobsList   *list)
{
  OobsListIter iter;
  gboolean valid;
  GObject *share;
  const gchar *path;

  valid = oobs_list_get_iter_first (list, &iter);

  while (valid)
    {
      share = oobs_list_get (list, &iter);
      path  = oobs_share_get_path (OOBS_SHARE (share));
      valid = oobs_list_iter_next (list, &iter);

      g_hash_table_insert (paths, g_strdup (path), GINT_TO_POINTER (TRUE));
      g_object_unref (share);
    }
}

static gboolean
return_true (gpointer key, gpointer value, gpointer data)
{
  return TRUE;
}

static void
update_shared_paths (NautilusShares *shares)
{
  /* clean up the paths */
  g_hash_table_foreach_remove (shares->paths, return_true, NULL);

  add_paths (shares->paths, oobs_smb_config_get_shares (OOBS_SMB_CONFIG (shares->smb_config)));
  add_paths (shares->paths, oobs_nfs_config_get_shares (OOBS_NFS_CONFIG (shares->nfs_config)));
}

static void
on_shares_changed (OobsObject     *object,
		   NautilusShares *shares)
{
  oobs_object_update (object);
  update_shared_paths (shares);
}

static void
nautilus_shares_init (NautilusShares *shares)
{
  shares->paths = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  shares->session = oobs_session_get ();

  shares->smb_config = oobs_smb_config_get (shares->session);
  g_signal_connect (G_OBJECT (shares->smb_config), "changed",
		    G_CALLBACK (on_shares_changed), shares);

  shares->nfs_config = oobs_nfs_config_get (shares->session);
  g_signal_connect (G_OBJECT (shares->nfs_config), "changed",
		    G_CALLBACK (on_shares_changed), shares);

  /* fill the hash table */
  update_shared_paths (shares);
}

void
nautilus_shares_register_type (GTypeModule *module)
{
  if (!type)
    {
      static const GTypeInfo info =
	{
	  sizeof (NautilusSharesClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) NULL,
	  NULL,
	  NULL,
	  sizeof (NautilusShares),
	  0,
	  (GInstanceInitFunc) nautilus_shares_init,
	};

      static const GInterfaceInfo menu_provider_iface_info =
	{
	  (GInterfaceInitFunc) menu_provider_iface_init,
	  NULL,
	  NULL
	};

      static const GInterfaceInfo info_provider_iface_info = 
	{
	  (GInterfaceInitFunc) info_provider_iface_init,
	  NULL,
	  NULL
	};

      type = g_type_module_register_type (module,
					  G_TYPE_OBJECT,
					  "NautilusShares",
					  &info, 0);
      g_type_module_add_interface (module,
				   type,
				   NAUTILUS_TYPE_MENU_PROVIDER,
				   &menu_provider_iface_info);

      g_type_module_add_interface (module,
				   type,
				   NAUTILUS_TYPE_INFO_PROVIDER,
				   &info_provider_iface_info);
    }
}
