/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* share-export-smb.c: this file is part of shares-admin, a gnome-system-tool frontend 
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
#include "share-export-smb.h"
#include "gst.h"

struct _GstShareSMBPrivate {
	gchar *name;
	gchar *comment;

	GstShareSMBFlags flags;
};

static void gst_share_smb_class_init   (GstShareSMBClass *klass);
static void gst_share_smb_init         (GstShareSMB *share);
static void gst_share_smb_finalize     (GObject *object);

static void gst_share_smb_set_property (GObject      *object,
					guint         prop_id,
					const GValue *value,
					GParamSpec   *pspec);
static void gst_share_smb_get_property (GObject      *object,
					guint         prop_id,
					GValue       *value,
					GParamSpec   *pspec);

static void  gst_share_smb_get_xml     (GstShare     *share,
					xmlNodePtr    parent);

enum {
	PROP_0,
	PROP_NAME,
	PROP_COMMENT,
	PROP_FLAGS
};

static gpointer parent_class;

GType
gst_share_smb_flags_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GFlagsValue values[] = {
			{ GST_SHARE_SMB_ENABLED,   "GST_SHARE_SMB_ENABLED",   "smb-enabled" },
			{ GST_SHARE_SMB_BROWSABLE, "GST_SHARE_SMB_BROWSABLE", "smb-browsable" },
			{ GST_SHARE_SMB_PUBLIC,    "GST_SHARE_SMB_PUBLIC",    "smb-public" },
			{ GST_SHARE_SMB_WRITABLE,  "GST_SHARE_SMB_WRITABLE",  "smb-writable" },
			{ 0, NULL, NULL }
		};

		type = g_flags_register_static ("GstShareSMBFlags", values);
	}

	return type;
}

GType
gst_share_smb_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo share_smb_info = {
			sizeof (GstShareSMBClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gst_share_smb_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (GstShareSMB),
			0,		/* n_preallocs */
			(GInstanceInitFunc) gst_share_smb_init,
		};

		type = g_type_register_static (GST_TYPE_SHARE, "GstShareSMB",
					       &share_smb_info, 0);
	}

	return type;
}

static void
gst_share_smb_class_init (GstShareSMBClass *class)
{
	GObjectClass  *object_class = G_OBJECT_CLASS (class);
	GstShareClass *share_class  = GST_SHARE_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	object_class->set_property = gst_share_smb_set_property;
	object_class->get_property = gst_share_smb_get_property;
	object_class->finalize     = gst_share_smb_finalize;

	share_class->get_xml = gst_share_smb_get_xml;

	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("share_name",
							      NULL,
							      NULL,
							      NULL,
							      G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_COMMENT,
					 g_param_spec_string ("share_comment",
							      "Share comment",
							      "Comment for the share",
							      NULL,
							      G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_FLAGS,
					 g_param_spec_flags ("share_flags",
							     "Flags",
							     "Property flags for the share",
							     GST_TYPE_SHARE_SMB_FLAGS,
							     0,
							     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}

static void
gst_share_smb_init (GstShareSMB *share)
{
	g_return_if_fail (GST_IS_SHARE_SMB (share));

	share->_priv = g_new0 (GstShareSMBPrivate, 1);
	share->_priv->name    = NULL;
	share->_priv->comment = NULL;
	share->_priv->flags   = 0;
}

static void
gst_share_smb_finalize (GObject *object)
{
	GstShareSMB *share = GST_SHARE_SMB (object);

	g_return_if_fail (GST_IS_SHARE_SMB (share));

	if (share->_priv) {
		g_free (share->_priv->name);
		share->_priv->name = NULL;
		
		g_free (share->_priv->comment);
		share->_priv->comment = NULL;
		
		g_free (share->_priv);
		share->_priv = NULL;
	}

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gst_share_smb_set_property (GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GstShareSMB *share = GST_SHARE_SMB (object);

	g_return_if_fail (GST_IS_SHARE_SMB (share));

	switch (prop_id) {
	case PROP_NAME:
		share->_priv->name = (gchar*) g_value_dup_string (value);
		break;
	case PROP_COMMENT:
		share->_priv->comment = (gchar*) g_value_dup_string (value);
		break;
	case PROP_FLAGS:
		share->_priv->flags = g_value_get_flags (value);
		break;
	}
}

static void
gst_share_smb_get_property (GObject      *object,
			    guint         prop_id,
			    GValue       *value,
			    GParamSpec   *pspec)
{
	GstShareSMB *share = GST_SHARE_SMB (object);

	g_return_if_fail (GST_IS_SHARE_SMB (share));

	switch (prop_id) {
	case PROP_NAME:
		g_value_set_string (value, share->_priv->name);
		break;
	case PROP_COMMENT:
		g_value_set_string (value, share->_priv->comment);
		break;
	case PROP_FLAGS:
		g_value_set_flags (value, share->_priv->flags);
		break;
	}
}

static void
gst_share_smb_get_xml (GstShare *share, xmlNodePtr parent)
{
	xmlNodePtr node;
	gint       flags;

	node = gst_xml_element_add (parent, "export");
	gst_xml_element_set_attribute (node, "type", "smb");

	gst_xml_set_child_content (node, "path",    gst_share_get_path (share));
	gst_xml_set_child_content (node, "name",    GST_SHARE_SMB (share)->_priv->name);
	gst_xml_set_child_content (node, "comment", GST_SHARE_SMB (share)->_priv->comment);

	flags = GST_SHARE_SMB (share)->_priv->flags;

	if (flags & GST_SHARE_SMB_ENABLED)
		gst_xml_element_set_state (node, "enabled", TRUE);
	if (flags & GST_SHARE_SMB_BROWSABLE)
		gst_xml_element_set_state (node, "browse", TRUE);
	if (flags & GST_SHARE_SMB_PUBLIC)
		gst_xml_element_set_state (node, "public", TRUE);
	if (flags & GST_SHARE_SMB_WRITABLE)
		gst_xml_element_set_state (node, "write", TRUE);
}

const gchar*
gst_share_smb_get_name (GstShareSMB *share)
{
	g_return_val_if_fail (GST_IS_SHARE_SMB (share), NULL);

	return share->_priv->name;
}

void
gst_share_smb_set_name (GstShareSMB *share, const gchar *name)
{
	g_return_if_fail (GST_IS_SHARE_SMB (share));

	share->_priv->name = g_strdup (name);
}

const gchar*
gst_share_smb_get_comment (GstShareSMB *share)
{
	g_return_val_if_fail (GST_IS_SHARE_SMB (share), NULL);

	return share->_priv->comment;
}

void
gst_share_smb_set_comment (GstShareSMB *share, const gchar *comment)
{
	g_return_if_fail (GST_IS_SHARE_SMB (share));

	share->_priv->comment = g_strdup (comment);
}

GstShareSMBFlags
gst_share_smb_get_flags (GstShareSMB *share)
{
	g_return_val_if_fail (GST_IS_SHARE_SMB (share), 0);

	return share->_priv->flags;
}

void
gst_share_smb_set_flags (GstShareSMB *share, GstShareSMBFlags flags)
{
	g_return_if_fail (GST_IS_SHARE_SMB (share));

	share->_priv->flags = flags;
}

GstShareSMB*
gst_share_smb_new (const gchar      *name,
		   const gchar      *comment,
		   const gchar      *path,
		   GstShareSMBFlags  flags)
{
	GstShareSMB *share;

	share = g_object_new (GST_TYPE_SHARE_SMB,
			      "share-name",    name,
			      "share-comment", comment,
			      "share-flags",   flags,
			      NULL);

	gst_share_set_path    (GST_SHARE (share), path);

	return share;
}

static gint
gst_share_smb_get_flags_from_xml (xmlNodePtr node)
{
	gint flags = 0;

	if (gst_xml_element_get_state (node, "enabled"))
	    flags |= GST_SHARE_SMB_ENABLED;

	if (gst_xml_element_get_state (node, "browse"))
	    flags |= GST_SHARE_SMB_BROWSABLE;

	if (gst_xml_element_get_state (node, "public"))
	    flags |= GST_SHARE_SMB_PUBLIC;

	if (gst_xml_element_get_state (node, "write"))
	    flags |= GST_SHARE_SMB_WRITABLE;

	return flags;
}

GstShareSMB*
gst_share_smb_new_from_xml (xmlNodePtr node)
{
	gchar       *type, *name, *comment, *path;
	GstShareSMB *share;
	gint         flags;

	type = gst_xml_element_get_attribute (node, "type");

	g_return_val_if_fail (type != NULL, NULL);
	g_return_val_if_fail (strcmp (type, "smb") == 0, NULL);
	g_free (type);

	name    = gst_xml_get_child_content (node, "name");
	comment = gst_xml_get_child_content (node, "comment");
	path    = gst_xml_get_child_content (node, "path");
	flags   = gst_share_smb_get_flags_from_xml (node);

	share = gst_share_smb_new (name, comment, path, flags);

	g_free (name);
	g_free (comment);
	g_free (path);

	return share;
}
