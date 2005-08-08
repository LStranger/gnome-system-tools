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

#ifndef __GST_DISKS_TOOL_H__
#define __GST_DISKS_TOOL_H__

#include <glib-object.h>
#include "gst-tool.h"
#include "disks-storage.h"

#define GST_TYPE_DISKS_TOOL         (gst_disks_tool_get_type ())
#define GST_DISKS_TOOL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GST_TYPE_DISKS_TOOL, GstDisksTool))
#define GST_DISKS_TOOL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GST_TYPE_DISKS_TOOL, GstDisksToolClass))
#define GST_IS_DISKS_TOOL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_DISKS_TOOL))
#define GST_IS_DISKS_TOOL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GST_TYPE_DISKS_TOOL))
#define GST_DISKS_TOOL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GST_TYPE_DISKS_TOOL, GstDisksToolClass))

typedef struct _GstDisksTool      GstDisksTool;
typedef struct _GstDisksToolClass GstDisksToolClass;

struct _GstDisksTool {
	   GstTool      parent;

	   GList          *storages;
	   GtkIconTheme *icon_theme;
};

struct _GstDisksToolClass {
	   GstToolClass parent_class;
};

GType            gst_disks_tool_get_type (void);
GstTool         *gst_disks_tool_new      (void);

void             gst_disks_tool_add_storage (GstDisksTool *tool, GstDisksStorage *storage);

#endif /* __GST_DISKS_TOOL_H__ */
