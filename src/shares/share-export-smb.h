/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* share-export-smb.h: this file is part of shares-admin, a gnome-system-tool frontend 
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

#ifndef __SHARE_SMB_EXPORT_H__
#define __SHARE_SMB_EXPORT_H__

#include "share-export.h"
#include "gst.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
	GST_SHARE_SMB_ENABLED   = 1 << 0,
	GST_SHARE_SMB_BROWSABLE = 1 << 1,
	GST_SHARE_SMB_PUBLIC    = 1 << 2,
	GST_SHARE_SMB_WRITABLE  = 1 << 3,
} GstShareSMBFlags;

#define GST_TYPE_SHARE_SMB            (gst_share_smb_get_type())
#define GST_TYPE_SHARE_SMB_FLAGS      (gst_share_smb_flags_get_type())
#define GST_SHARE_SMB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_SHARE_SMB, GstShareSMB))
#define GST_SHARE_SMB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GST_TYPE_SHARE_SMB, GstShareSMBClass))
#define GST_IS_SHARE_SMB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_SHARE_SMB))
#define GST_IS_SHARE_SMB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GST_TYPE_SHARE_SMB))
#define GST_SHARE_SMB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_SHARE_SMB, GstShareSMBClass))

typedef struct _GstShareSMB        GstShareSMB;
typedef struct _GstShareSMBClass   GstShareSMBClass;
typedef struct _GstShareSMBPrivate GstShareSMBPrivate;

struct _GstShareSMB {
	GstShare parent;

	GstShareSMBPrivate *_priv;
};

struct _GstShareSMBClass {
	GstShareClass parent_class;
};

GType            gst_share_smb_get_type (void);
GType            gst_share_smb_flags_get_type (void);

const gchar*     gst_share_smb_get_name    (GstShareSMB*);
void             gst_share_smb_set_name    (GstShareSMB*, const gchar*);

const gchar*     gst_share_smb_get_comment (GstShareSMB*);
void             gst_share_smb_set_comment (GstShareSMB*, const gchar*);

GstShareSMBFlags gst_share_smb_get_flags (GstShareSMB*);
void             gst_share_smb_set_flags (GstShareSMB*, GstShareSMBFlags);

GstShareSMB*     gst_share_smb_new (const gchar*, const gchar*, const gchar*, GstShareSMBFlags);
GstShareSMB*     gst_share_smb_new_from_xml (xmlNodePtr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SHARE_SMB_EXPORT_H__ */
