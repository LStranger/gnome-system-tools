/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* share-export.c: this file is part of shares-admin, a gnome-system-tool frontend 
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
#include <glib.h>
#include "share-export.h"

struct _GstSharePrivate {
	gchar *path;
};

static void gst_share_class_init   (GstShareClass *klass);
static void gst_share_init         (GstShare *share);
static void gst_share_finalize     (GObject *object);

static void gst_share_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec);
static void gst_share_get_property (GObject      *object,
				    guint         prop_id,
				    GValue       *value,
				    GParamSpec   *pspec);

enum {
	PROP_0,
	PROP_PATH
};

static gpointer parent_class;

GType
gst_share_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo share_info = {
			sizeof (GstShareClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gst_share_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (GstShare),
			0,		/* n_preallocs */
			(GInstanceInitFunc) gst_share_init,
		};

		type = g_type_register_static (G_TYPE_OBJECT, "GstShare",
					       &share_info, 0);
	}

	return type;
}

static void
gst_share_class_init (GstShareClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	object_class->set_property = gst_share_set_property;
	object_class->get_property = gst_share_get_property;
	object_class->finalize     = gst_share_finalize;

	g_object_class_install_property (object_class,
					 PROP_PATH,
					 g_param_spec_string ("share_path",
							      "Share path",
							      "Path for the share",
							      NULL,
							      G_PARAM_READWRITE));
}

static void
gst_share_init (GstShare *share)
{
	g_return_if_fail (GST_IS_SHARE (share));

	share->_priv = g_new0 (GstSharePrivate, 1);

	share->_priv->path = NULL;
}

static void
gst_share_finalize (GObject *object)
{
	GstShare *share = GST_SHARE (object);

	g_return_if_fail (GST_IS_SHARE (share));

	if (share->_priv) {
		g_free (share->_priv->path);
		share->_priv->path = NULL;

		g_free (share->_priv);
		share->_priv = NULL;
	}

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gst_share_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
	GstShare *share = GST_SHARE (object);

	g_return_if_fail (GST_IS_SHARE (share));

	switch (prop_id) {
	case PROP_PATH:
		share->_priv->path    = (gchar*) g_value_dup_string (value);
		break;
	}
}

static void
gst_share_get_property (GObject      *object,
			guint         prop_id,
			GValue       *value,
			GParamSpec   *pspec)
{
	GstShare *share = GST_SHARE (object);

	g_return_if_fail (GST_IS_SHARE (share));

	switch (prop_id) {
	case PROP_PATH:
		g_value_set_string (value, share->_priv->path);
		break;
	}
}

const gchar*
gst_share_get_path (GstShare *share)
{
	g_return_val_if_fail (GST_IS_SHARE (share), NULL);

	return share->_priv->path;
}

void
gst_share_set_path (GstShare *share, const gchar *path)
{
	g_return_if_fail (GST_IS_SHARE (share));

	share->_priv->path = g_strdup (path);
}

void
gst_share_get_xml (GstShare *share, xmlNodePtr parent)
{
	g_return_if_fail (GST_IS_SHARE (share));

	if (GST_SHARE_GET_CLASS (share)->get_xml == NULL)
		return;

	return GST_SHARE_GET_CLASS (share)->get_xml (share, parent);
}
