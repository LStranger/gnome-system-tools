/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* share-export-nfs.h: this file is part of shares-admin, a gnome-system-tool frontend 
 * for run level services administration.
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

#ifndef __SHARE_NFS_EXPORT_H__
#define __SHARE_NFS_EXPORT_H__

#include "share-export.h"
#include "gst.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GST_TYPE_SHARE_NFS            (gst_share_nfs_get_type())
#define GST_SHARE_NFS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_SHARE_NFS, GstShareNFS))
#define GST_SHARE_NFS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GST_TYPE_SHARE_NFS, GstShareNFSClass))
#define GST_IS_SHARE_NFS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_SHARE_NFS))
#define GST_IS_SHARE_NFS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GST_TYPE_SHARE_NFS))
#define GST_SHARE_NFS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_SHARE_NFS, GstShareNFSClass))

typedef struct _GstShareNFS        GstShareNFS;
typedef struct _GstShareNFSClass   GstShareNFSClass;
typedef struct _GstShareNFSPrivate GstShareNFSPrivate;

struct _GstShareNFS {
	GstShare parent;

	GstShareNFSPrivate *_priv;
};

struct _GstShareNFSClass {
	GstShareClass parent_class;
};

GType      gst_share_nfs_get_type (void);

/*GstShareSMB*     gst_share_nfs_new (gchar*, gchar*, gchar*);
GstShareSMB*     gst_share_smb_new_from_xml (xmlNodePtr);
*/
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SHARE_NFS_EXPORT_H__ */
