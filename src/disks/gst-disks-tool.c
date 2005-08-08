/*
 * Copyright (C) 2001 Ximian, Inc.
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
 * Authors: Carlos Garcia Campos <elkalmail@yahoo.es>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gst-tool.h"
#include "gst-disks-tool.h"

#define PARENT_TYPE GST_TYPE_TOOL

static void disks_tool_init                    (GstDisksTool      *tool);
static void disks_tool_class_init              (GstDisksToolClass *klass);
static void disks_tool_finalize                (GObject           *object);

static GObjectClass *parent_class = NULL;

GType
gst_disks_tool_get_type (void)
{
	   static GType type = 0;

	   if (!type) {
			 static const GTypeInfo info = {
				    sizeof (GstDisksToolClass),
				    (GBaseInitFunc) NULL,
				    (GBaseFinalizeFunc) NULL,
				    (GClassInitFunc) disks_tool_class_init,
				    NULL,
				    NULL,
				    sizeof (GstDisksTool),
				    0,
				    (GInstanceInitFunc) disks_tool_init
			 };
			 type = g_type_register_static (PARENT_TYPE, "GstDisksTool",
									  &info, 0);
	   }
	   return type;
}

static void
disks_tool_init (GstDisksTool *tool)
{
	   g_return_if_fail (GST_IS_DISKS_TOOL (tool));

	   tool->storages = NULL;
	   tool->icon_theme = gtk_icon_theme_get_default ();
}

static void
disks_tool_class_init (GstDisksToolClass *klass)
{
	   GObjectClass         *object_class  = G_OBJECT_CLASS (klass);

	   parent_class = g_type_class_peek_parent (klass);

	   object_class->finalize = disks_tool_finalize;
}

static void
delete_object (gpointer object, gpointer gdata)
{
	   g_object_unref (G_OBJECT (object));
}

static void
disks_tool_finalize (GObject *object)
{
	   GstDisksTool *tool = GST_DISKS_TOOL (object);
	   g_return_if_fail (GST_IS_DISKS_TOOL (tool));

	   if (tool->storages) {
			 g_list_foreach (tool->storages, delete_object, NULL);
			 g_list_free (tool->storages);
			 tool->storages = NULL;
	   }
	   if (tool->icon_theme) {
			 g_object_unref (G_OBJECT (tool->icon_theme));
			 tool->icon_theme = NULL;
	   }
	   
	   if (G_OBJECT_CLASS (parent_class)->finalize)
			 (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

GstTool *
gst_disks_tool_new (void)
{
	   GstDisksTool *tool;

	   tool = g_object_new (GST_TYPE_DISKS_TOOL, NULL);

	   return GST_TOOL (tool);
}

void
gst_disks_tool_add_storage (GstDisksTool *tool, GstDisksStorage *storage)
{
	   g_return_if_fail (GST_IS_DISKS_TOOL (tool));

	   tool->storages = g_list_append (tool->storages, (gpointer) storage);
}
