/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <glib.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

xmlNodePtr     gst_xml_doc_get_root             (xmlDocPtr doc);
void           gst_xml_doc_dump                 (xmlDocPtr doc);
xmlDocPtr      gst_xml_doc_create               (const gchar *root_name);
void           gst_xml_doc_destroy              (xmlDocPtr doc);

xmlNodePtr     gst_xml_element_get_parent       (xmlNodePtr node);
void           gst_xml_element_add_child        (xmlNodePtr parent, xmlNodePtr child);
void           gst_xml_element_reparent         (xmlNodePtr node, xmlNodePtr new_parent);

xmlNodePtr     gst_xml_element_find_first       (xmlNodePtr parent,  const gchar *name);
xmlNodePtr     gst_xml_element_find_next        (xmlNodePtr sibling, const gchar *name);
xmlNodePtr     gst_xml_element_find_nth         (xmlNodePtr parent,  const gchar *name, int n);

xmlNodePtr     gst_xml_element_add              (xmlNodePtr node, const gchar *name);
void           gst_xml_element_add_with_content (xmlNodePtr node, const gchar *name, const gchar *content);

const xmlChar *gst_xml_element_peek_content     (xmlNodePtr node);
char          *gst_xml_element_get_content      (xmlNodePtr node);
void           gst_xml_element_set_content      (xmlNodePtr node, const gchar *text);

char          *gst_xml_element_get_attribute    (xmlNodePtr node, const gchar *attr);
void           gst_xml_element_set_attribute    (xmlNodePtr node, const gchar *attr, const gchar *value);

/* For tags looking like <element state="{true|false}"/> */
gboolean       gst_xml_element_get_bool_attr    (xmlNodePtr node, const gchar *attr);
void           gst_xml_element_set_bool_attr    (xmlNodePtr node, const gchar *attr, gboolean state);

gboolean       gst_xml_element_get_state        (xmlNodePtr node, const gchar *element);
void           gst_xml_element_set_state        (xmlNodePtr node, const gchar *element, gboolean state);

void           gst_xml_element_destroy                  (xmlNodePtr node);
void           gst_xml_element_destroy_children         (xmlNodePtr parent);
void           gst_xml_element_destroy_children_by_name (xmlNodePtr parent, const gchar *name);

int            gst_xml_parent_childs                    (xmlNodePtr parent);

gchar         *gst_xml_get_child_content        (xmlNodePtr parent, const gchar *child);
void           gst_xml_set_child_content        (xmlNodePtr parent, const gchar *child, const gchar *val);

/* For tags looking like <element>{1|0}</element> */
gboolean       gst_xml_element_get_boolean      (xmlNodePtr parent, const gchar *name);
void           gst_xml_element_set_boolean      (xmlNodePtr parent, const gchar *child, const gboolean val);
