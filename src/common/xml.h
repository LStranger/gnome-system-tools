#include <glib.h>
#include <tree.h>    /* libxml */
#include <parser.h>  /* libxml */


xmlNodePtr xml_doc_get_root(xmlDocPtr doc);

xmlDocPtr xml_doc_read_from_backend(char *backend_name);
void xml_doc_write_to_backend(xmlDocPtr doc, char *backend_name);
void xml_doc_dump(xmlDocPtr doc);

xmlNodePtr xml_element_find_first(xmlNodePtr parent, char *name);
xmlNodePtr xml_element_find_next(xmlNodePtr sibling, char *name);
xmlNodePtr xml_element_find_nth (xmlNodePtr parent, char *name, int n);

xmlNodePtr xml_element_add(xmlNodePtr parent, char *name);
void xml_element_add_with_content(xmlNodePtr node, char *name, char *content);

char *xml_element_get_content(xmlNodePtr node);
void xml_element_set_content(xmlNodePtr node, char *text);

char *xml_element_get_attribute(xmlNodePtr node, char *attr);
void xml_element_set_attribute(xmlNodePtr node, char *attr, char *value);

gboolean xml_element_get_bool_attr(xmlNodePtr node, char *attr);
void xml_element_set_bool_attr(xmlNodePtr node, char *attr, gboolean state);

gboolean xml_element_get_state(xmlNodePtr node, char *element);
void xml_element_set_state(xmlNodePtr node, char *element, gboolean state);

void xml_element_destroy(xmlNodePtr node);
void xml_element_destroy_children(xmlNodePtr parent);
void xml_element_destroy_children_by_name(xmlNodePtr parent, char *name);

int xml_parent_childs (xmlNodePtr parent);

