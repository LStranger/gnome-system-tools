/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <glib.h>
#include <tree.h>    /* libxml */
#include <parser.h>  /* libxml */


xmlNodePtr xst_xml_doc_get_root(xmlDocPtr doc);

void xst_xml_doc_dump(xmlDocPtr doc);

xmlNodePtr xst_xml_element_get_parent (xmlNodePtr node);
void       xst_xml_element_add_child  (xmlNodePtr parent, xmlNodePtr child);

xmlNodePtr xst_xml_element_find_first(xmlNodePtr parent, char *name);
xmlNodePtr xst_xml_element_find_next(xmlNodePtr sibling, char *name);
xmlNodePtr xst_xml_element_find_nth (xmlNodePtr parent, char *name, int n);

xmlNodePtr xst_xml_element_add(xmlNodePtr parent, char *name);
void xst_xml_element_add_with_content(xmlNodePtr node, char *name, char *content);

char *xst_xml_element_get_content(xmlNodePtr node);
void xst_xml_element_set_content(xmlNodePtr node, char *text);

char *xst_xml_element_get_attribute(xmlNodePtr node, char *attr);
void xst_xml_element_set_attribute(xmlNodePtr node, char *attr, char *value);

gboolean xst_xml_element_get_bool_attr(xmlNodePtr node, char *attr);
void xst_xml_element_set_bool_attr(xmlNodePtr node, char *attr, gboolean state);

gboolean xst_xml_element_get_state(xmlNodePtr node, char *element);
void xst_xml_element_set_state(xmlNodePtr node, char *element, gboolean state);

void xst_xml_element_destroy(xmlNodePtr node);
void xst_xml_element_destroy_children(xmlNodePtr parent);
void xst_xml_element_destroy_children_by_name(xmlNodePtr parent, char *name);

int xst_xml_parent_childs (xmlNodePtr parent);
gchar *xst_xml_get_child_content (xmlNodePtr parent, gchar *child);
void xst_xml_set_child_content (xmlNodePtr parent, gchar *child, gchar *val);

