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
#include <tree.h>  /* libxml */
#include <parser.h>  /* libxml */
#include <xmlmemory.h>  /* libxml */

#include "xml.h"

typedef struct {
	char *font_name;
	gint is_basic_14;
} ps_internal_font;

xmlNodePtr
xst_xml_doc_get_root (xmlDocPtr doc)
{
	return (xmlDocGetRootElement (doc));
}

void
xst_xml_doc_dump (xmlDocPtr doc)
{
	xmlDocDump (stdout, doc);
}


xmlNodePtr
xst_xml_element_find_first (xmlNodePtr parent, char *name)
{
	xmlNodePtr node;
	
	g_return_val_if_fail (parent != NULL, NULL);

	for (node = parent->childs; node;)
	{
		if (!strcmp (name, node->name)) break;
		node = node->next;
	}

	return (node);
}


xmlNodePtr
xst_xml_element_find_next (xmlNodePtr sibling, char *name)
{
	xmlNodePtr node;
	
	g_return_val_if_fail (sibling != NULL, NULL);

	for (node = sibling->next; node; node = node->next)
		if (!strcmp (name, node->name))
			break;

	return (node);
}


xmlNodePtr
xst_xml_element_find_nth (xmlNodePtr parent, char *name, int n)
{
	xmlNodePtr node;
	gint i = 0;

	g_return_val_if_fail (parent != NULL, NULL);

	for (node = parent->childs; node;)
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
xst_xml_element_add (xmlNodePtr parent, char *name)
{
	return (xmlNewChild (parent, NULL, name, NULL));
}


gchar *
xst_xml_element_get_content (xmlNodePtr node)
{
	gchar *text = NULL, *r;
	xmlNodePtr n0;

	g_return_val_if_fail (node != NULL, NULL);
	
	for (n0 = node->childs; n0; n0 = n0->next)
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


void
xst_xml_element_set_content (xmlNodePtr node, char *text)
{
	g_return_if_fail (node != NULL);
	
	xmlNodeSetContent (node, text);
}


void
xst_xml_element_add_with_content (xmlNodePtr node, char *name, char *content)
{
	xmlNodePtr n0;

	g_return_if_fail (node != NULL);
	
	n0 = xst_xml_element_find_first (node, name);
	if (!n0) n0 = xst_xml_element_add (node, name);

	xst_xml_element_set_content (n0, content);
}


gchar *
xst_xml_element_get_attribute (xmlNodePtr node, char *attr)
{
	xmlAttrPtr a0;
	gchar *text = NULL, *r = NULL;

	g_return_val_if_fail (node != NULL, NULL);
	
	for (a0 = node->properties; a0; a0 = a0->next)
	{
		if (a0->name && !strcmp (a0->name, attr))
		{
			text = xmlNodeGetContent (a0->val);
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
xst_xml_element_set_attribute (xmlNodePtr node, char *attr, char *value)
{
	g_return_if_fail (node != NULL);
	
	xmlSetProp (node, attr, value);
}


gboolean
xst_xml_element_get_bool_attr (xmlNodePtr node, char *attr)
{
	char *s;
	int r = FALSE;

	g_return_val_if_fail (node != NULL, FALSE);
	
	s = xst_xml_element_get_attribute (node, attr);
	if (s)
	{
		if (strchr ("yYtT", s[0])) r = TRUE;  /* Yes, true */
		g_free (s);
	}

	return (r);
}


void
xst_xml_element_set_bool_attr (xmlNodePtr node, char *attr, gboolean state)
{
	g_return_if_fail (node != NULL);
	
	xst_xml_element_set_attribute (node, attr, state ? "true" : "false");
}


gboolean
xst_xml_element_get_state (xmlNodePtr node, char *element)
{
	xmlNodePtr elem;
	char *s;
	int r = FALSE;

	g_return_val_if_fail (node != NULL, FALSE);
	
	elem = xst_xml_element_find_first (node, element);
	if (elem)
	{
		s = xst_xml_element_get_attribute (elem, "state");
		if (s)
		{
			if (strchr ("yYtT", s[0])) r = TRUE;  /* Yes, true */
			g_free (s);
		}
	}

	return (r);
}


void
xst_xml_element_set_state (xmlNodePtr node, char *element, gboolean state)
{
	xmlNodePtr elem;

	g_return_if_fail (node != NULL);
	
	elem = xst_xml_element_find_first (node, element);
	if (!elem) elem = xst_xml_element_add (node, element);
	xst_xml_element_set_attribute (elem, "state", state ? "true" : "false");
}


void
xst_xml_element_destroy (xmlNodePtr node)
{
	g_return_if_fail (node != NULL);
	
	xmlUnlinkNode (node);
	xmlFreeNode (node);
}


void
xst_xml_element_destroy_children (xmlNodePtr parent)
{
	xmlNodePtr node, node_next;

	g_return_if_fail (parent != NULL);
	
	for (node = parent->childs; node;)
	{
		node_next = node->next;
		xmlUnlinkNode (node);
		xmlFreeNode (node);
		node = node_next;
	}
}


void
xst_xml_element_destroy_children_by_name (xmlNodePtr parent, char *name)
{
	xmlNodePtr node, node_next;

	g_return_if_fail (parent != NULL);
	
	for (node = xst_xml_element_find_first (parent, name); node;)
	{
		node_next = xst_xml_element_find_next (node, name);
		xmlUnlinkNode (node);
		xmlFreeNode (node);
		node = node_next;
	}
}

int
xst_xml_parent_childs (xmlNodePtr parent)
{
	xmlNodePtr node;
	gint ret = 0;

	g_return_val_if_fail (parent != NULL, 0);

	for (node = parent->childs; node; ret++)
		node = node->next;

	return ret;
}

gchar *
xst_xml_get_child_content (xmlNodePtr parent, gchar *child)
{
	xmlNodePtr node;

	node = xst_xml_element_find_first (parent, child);
	if (!node)
		return NULL;

	return xst_xml_element_get_content (node);
}

void
xst_xml_set_child_content (xmlNodePtr parent, gchar *child, gchar *val)
{
	xmlNodePtr node;

	g_return_if_fail (parent != NULL);
	g_return_if_fail (child != NULL);
	g_return_if_fail (val != NULL);

	node = xst_xml_element_find_first (parent, child);
	if (!node)
		node = xst_xml_element_add (parent, child);

	xst_xml_element_set_content (node, val);
}

