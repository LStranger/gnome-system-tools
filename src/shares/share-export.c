/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* share-export.c: this file is part of shares-admin, a gnome-system-tool frontend 
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

#include <config.h>
#include <glib.h>
#include "share-export.h"

struct _GstSharePrivate {
	   gchar *name;
	   gchar *comment;
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
	PROP_NAME,
	PROP_COMMENT,
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
					 PROP_NAME,
					 g_param_spec_string ("share_name",
							      "Share name",
							      "Name for the share",
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_COMMENT,
					 g_param_spec_string ("share_comment",
							      "Share comment",
							      "Comment for the share",
							      NULL,
							      G_PARAM_READWRITE));
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

	share->_priv->name    = NULL;
	share->_priv->comment = NULL;
	share->_priv->path    = NULL;
}

static void
gst_share_finalize (GObject *object)
{
	GstShare *share = GST_SHARE (object);

	g_return_if_fail (GST_IS_SHARE (share));

	if (share->_priv) {
		g_free (share->_priv->name);
		share->_priv->name = NULL;
		
		g_free (share->_priv->comment);
		share->_priv->comment = NULL;
		
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
	case PROP_NAME:
		share->_priv->name    = (gchar*) g_value_get_string (value);
		break;
	case PROP_COMMENT:
		share->_priv->comment = (gchar*) g_value_get_string (value);
		break;
	case PROP_PATH:
		share->_priv->path    = (gchar*) g_value_get_string (value);
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
	case PROP_NAME:
		g_value_set_string (value, share->_priv->name);
		break;
	case PROP_COMMENT:
		g_value_set_string (value, share->_priv->comment);
		break;
	case PROP_PATH:
		g_value_set_string (value, share->_priv->path);
		break;
	}
}

gchar*
gst_share_get_name (GstShare *share)
{
	g_return_val_if_fail (GST_IS_SHARE (share), NULL);

	return share->_priv->name;
}

void
gst_share_set_name (GstShare *share, gchar *name)
{
	g_return_if_fail (GST_IS_SHARE (share));

	share->_priv->name = name;
}

gchar*
gst_share_get_comment (GstShare *share)
{
	g_return_val_if_fail (GST_IS_SHARE (share), NULL);

	return share->_priv->comment;
}

void
gst_share_set_comment (GstShare *share, gchar *comment)
{
	g_return_if_fail (GST_IS_SHARE (share));

	share->_priv->comment = comment;
}

gchar*
gst_share_get_path (GstShare *share)
{
	g_return_val_if_fail (GST_IS_SHARE (share), NULL);

	return share->_priv->path;
}

void
gst_share_set_path (GstShare *share, gchar *path)
{
	g_return_if_fail (GST_IS_SHARE (share));

	share->_priv->path = path;
}

