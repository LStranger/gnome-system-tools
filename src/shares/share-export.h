/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* share-export.h: this file is part of shares-admin, a gnome-system-tool frontend 
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

#ifndef __SHARE_EXPORT_H__
#define __SHARE_EXPORT_H__

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GST_TYPE_SHARE            (gst_share_get_type())
#define GST_SHARE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_SHARE, GstShare))
#define GST_SHARE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GST_TYPE_SHARE, GstShareClass))
#define GST_IS_SHARE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_SHARE))
#define GST_IS_SHARE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GST_TYPE_SHARE))
#define GST_SHARE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_SHARE, GstShareClass))

typedef struct _GstShare        GstShare;
typedef struct _GstShareClass   GstShareClass;
typedef struct _GstSharePrivate GstSharePrivate;

struct _GstShare {
	GObject parent;

	GstSharePrivate *_priv;
};

struct _GstShareClass {
	GObjectClass parent_class;
};

GType      gst_share_get_type    (void);

gchar*     gst_share_get_name    (GstShare*);
void       gst_share_set_name    (GstShare*, gchar*);

gchar*     gst_share_get_comment (GstShare*);
void       gst_share_set_comment (GstShare*, gchar*);

gchar*     gst_share_get_path    (GstShare*);
void       gst_share_set_path    (GstShare*, gchar*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SHARE_EXPORT_H__ */
