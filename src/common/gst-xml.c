/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Hans Petter Jansson <hpj@ximian.com>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>

#include "gst-xml.h"

xmlNodePtr
gst_xml_doc_get_root (xmlDocPtr doc)
{
	return (xmlDocGetRootElement (doc));
}

void
gst_xml_doc_dump (xmlDocPtr doc)
{
	xmlDocDump (stdout, doc);
}

xmlDocPtr
gst_xml_doc_create (const gchar *root_name)
{
	xmlDocPtr  doc;
	xmlNodePtr root;

	doc = xmlNewDoc ("1.0");
	root = xmlNewNode (NULL, root_name);
	root = xmlDocSetRootElement (doc, root);

	return doc;
}
	
void
gst_xml_doc_destroy (xmlDocPtr doc)
{
	xmlFreeDoc (doc);
}

xmlNodePtr
gst_xml_element_get_parent (xmlNodePtr node)
{
	return node->parent;
}

void
gst_xml_element_reparent (xmlNodePtr node, xmlNodePtr new_parent)
{
	xmlUnlinkNode (node);
	gst_xml_element_add_child (new_parent, node);
}

xmlNodePtr
gst_xml_element_find_first (xmlNodePtr parent, const gchar *name)
{
	xmlNodePtr node;
	
	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	for (node = parent->children; node;)
	{
		if (node->name && !strcmp (name, node->name)) break;
		node = node->next;
	}

	return (node);
}


xmlNodePtr
gst_xml_element_find_next (xmlNodePtr sibling, const gchar *name)
{
	xmlNodePtr node;
	
	g_return_val_if_fail (sibling != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	for (node = sibling->next; node; node = node->next)
		if (node->name && !strcmp (name, node->name))
			break;

	return (node);
}


xmlNodePtr
gst_xml_element_find_nth (xmlNodePtr parent, const gchar *name, int n)
{
	xmlNodePtr node;
	gint i = 0;

	g_return_val_if_fail (parent != NULL, NULL);

	for (node = parent->children; node;)
	{
		if (!strcmp (name, node->name))
		{
			if (i == n)
				break;
			i++;
		}
		node = node->next;
	}

	return (node);
}


xmlNodePtr
gst_xml_element_add (xmlNodePtr parent, const gchar *name)
{
	g_return_val_if_fail (parent != NULL, NULL);

	return (xmlNewChild (parent, NULL, name, NULL));
}


void
gst_xml_element_add_child (xmlNodePtr parent, xmlNodePtr child)
{
	xmlAddChild (parent, child);
}


gchar *
gst_xml_element_get_content (xmlNodePtr node)
{
	gchar *text = NULL;
	gchar *r;
	xmlNodePtr n0;

	g_return_val_if_fail (node != NULL, NULL);
	
	for (n0 = node->children; n0; n0 = n0->next)
	{
		if (n0->type == XML_TEXT_NODE)
		{
			text = xmlNodeGetContent (n0);
			break;
		}
	}

	if (text)
	{
		r = g_strdup (text);
		xmlFree (text);
	}
	else
		r = g_strdup ("");

	return r;
}


const xmlChar *
gst_xml_element_peek_content (xmlNodePtr node)
{
	xmlChar *text = NULL;
	xmlNodePtr n0;

	g_return_val_if_fail (node != NULL, NULL);
	
	for (n0 = node->children; n0; n0 = n0->next)
	{
		if (n0->type == XML_TEXT_NODE)
		{
			/* NOTE: This could be a problem if XML_USE_BUFFER_CONTENT */

			text = n0->content;
			break;
		}
	}

	return text;
}


void
gst_xml_element_set_content (xmlNodePtr node, const gchar *text)
{
	g_return_if_fail (node != NULL);
	
	xmlNodeSetContent (node, text);
}


void
gst_xml_element_add_with_content (xmlNodePtr node, const gchar *name, const gchar *content)
{
	xmlNodePtr n0;

	g_return_if_fail (node != NULL);
	
	n0 = gst_xml_element_find_first (node, name);
	if (!n0) n0 = gst_xml_element_add (node, name);

	gst_xml_element_set_content (n0, content);
}


gchar *
gst_xml_element_get_attribute (xmlNodePtr node, const gchar *attr)
{
	xmlAttrPtr a0;
	gchar *text = NULL, *r = NULL;

	g_return_val_if_fail (node != NULL, NULL);
	
	for (a0 = node->properties; a0; a0 = a0->next)
	{
		if (a0->name && !strcmp (a0->name, attr))
		{
			text = xmlNodeGetContent (a0->children);
			break;
		}
	}

	if (text)
	{
		r = g_strdup (text);
		xmlFree (text);
	}

	return (r);
}


void
gst_xml_element_set_attribute (xmlNodePtr node, const gchar *attr, const gchar *value)
{
	g_return_if_fail (node != NULL);
	
	xmlSetProp (node, attr, value);
}


gboolean
gst_xml_element_get_bool_attr (xmlNodePtr node, const gchar *attr)
{
	char *s;
	int r = FALSE;

	g_return_val_if_fail (node != NULL, FALSE);
	
	s = gst_xml_element_get_attribute (node, attr);
	if (s)
	{
		if (strchr ("yYtT", s[0])) r = TRUE;  /* Yes, true */
		g_free (s);
	}

	return (r);
}


void
gst_xml_element_set_bool_attr (xmlNodePtr node, const gchar *attr, gboolean state)
{
	g_return_if_fail (node != NULL);
	
	gst_xml_element_set_attribute (node, attr, state ? "true" : "false");
}


gboolean
gst_xml_element_get_state (xmlNodePtr node, const gchar *element)
{
	xmlNodePtr elem;
	char *s;
	int r = FALSE;

	g_return_val_if_fail (node != NULL, FALSE);
	
	elem = gst_xml_element_find_first (node, element);
	if (elem)
	{
		s = gst_xml_element_get_attribute (elem, "state");
		if (s)
		{
			if (strchr ("yYtT", s[0])) r = TRUE;  /* Yes, true */
			g_free (s);
		}
	}

	return (r);
}


void
gst_xml_element_set_state (xmlNodePtr node, const gchar *element, gboolean state)
{
	xmlNodePtr elem;

	g_return_if_fail (node != NULL);
	
	elem = gst_xml_element_find_first (node, element);
	if (!elem) elem = gst_xml_element_add (node, element);
	gst_xml_element_set_attribute (elem, "state", state ? "true" : "false");
}


void
gst_xml_element_destroy (xmlNodePtr node)
{
	g_return_if_fail (node != NULL);
	
	xmlUnlinkNode (node);
	xmlFreeNode (node);
}


void
gst_xml_element_destroy_children (xmlNodePtr parent)
{
	xmlNodePtr node, node_next;

	g_return_if_fail (parent != NULL);
	
	for (node = parent->children; node;)
	{
		node_next = node->next;
		xmlUnlinkNode (node);
		xmlFreeNode (node);
		node = node_next;
	}
}


void
gst_xml_element_destroy_children_by_name (xmlNodePtr parent, const gchar *name)
{
	xmlNodePtr node, node_next;

	g_return_if_fail (parent != NULL);
	
	for (node = gst_xml_element_find_first (parent, name); node;)
	{
		node_next = gst_xml_element_find_next (node, name);
		xmlUnlinkNode (node);
		xmlFreeNode (node);
		node = node_next;
	}
}

int
gst_xml_parent_childs (xmlNodePtr parent)
{
	xmlNodePtr node;
	gint ret = 0;

	g_return_val_if_fail (parent != NULL, 0);

	for (node = parent->children; node; ret++)
		node = node->next;

	return ret;
}

gchar *
gst_xml_get_child_content (xmlNodePtr parent, const gchar *child)
{
	xmlNodePtr node;

	node = gst_xml_element_find_first (parent, child);
	if (!node)
		return NULL;

	return gst_xml_element_get_content (node);
}

void
gst_xml_set_child_content (xmlNodePtr parent, const gchar *child, const gchar *val)
{
	xmlNodePtr node;

	g_return_if_fail (parent != NULL);
	g_return_if_fail (child != NULL);

	if (!val)
		return;

	node = gst_xml_element_find_first (parent, child);
	if (!node)
		node = gst_xml_element_add (parent, child);

	gst_xml_element_set_content (node, val);
}

gboolean
gst_xml_element_get_boolean (xmlNodePtr parent, const gchar *name)
{
	xmlNodePtr node;
	gboolean res;
	gchar *str;

	res = FALSE;
	node = gst_xml_element_find_first (parent, name);

	if (node) {
		str = gst_xml_element_get_content (node);
		res = (*str == '1' || *str == 'T' ) ? TRUE: FALSE;
		g_free (str);
	}

	return res;
}

void
gst_xml_element_set_boolean (xmlNodePtr parent, const gchar *child, const gboolean val)
{
	gst_xml_set_child_content (parent, child, val? "1": "0");
}
