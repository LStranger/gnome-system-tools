/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
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
get_path_from_url (const char *url)
{
  GnomeVFSURI *uri     = NULL;
  gchar       *escaped = NULL;
  gchar       *path    = NULL;

  uri = gnome_vfs_uri_new (url);

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
  GnomeVFSURI      *uri;
  GString          *cmd;
  gchar            *str, *dir;

  cmd  = g_string_new ("shares-admin ");
  info = g_object_get_data (G_OBJECT (menu_item), "file");

  str = nautilus_file_info_get_uri (info);
  uri = gnome_vfs_uri_new (str);
  dir = get_path_from_url (gnome_vfs_uri_get_path (uri));

  g_string_append_printf (cmd, "--add-share=\"%s\"", dir);

  g_spawn_command_line_async (cmd->str, NULL);

  gnome_vfs_uri_unref (uri);
  g_string_free (cmd, TRUE);
  g_free (str);
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
				      NULL);
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

GType
nautilus_shares_get_type (void)
{
  return type;
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
	  (GInstanceInitFunc) NULL,
	};

      static const GInterfaceInfo iface_info =
	{
	  (GInterfaceInitFunc) menu_provider_iface_init,
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
				   &iface_info);
    }
}
