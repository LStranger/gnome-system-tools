/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Structures for tables detailing the information types in the XML
   tree, and what widgets they correspond to. */


typedef struct _TransStringEntry TransStringEntry;
typedef struct _TransStringSpin TransStringSpin;
typedef struct _TransStringList TransStringList;
typedef struct _TransStringCList2 TransStringCList2;
typedef struct _TransStringIPEntry TransStringIPEntry;
typedef struct _TransStringCList TransStringCList;
typedef struct _TransStringCalendar TransStringCalendar;

typedef struct _TransTree TransTree;

/*
  We refer to widgets by their Glade names.
*/

struct _TransStringEntry
{
  char *xml_path;

  char *editable;
  char *toggle;
  int unknown_verbose;      /* Whether to put <unknown> if not found in XML */
};


struct _TransStringSpin
{
  char *xml_path;
  char *spin;
};


struct _TransStringList
{
  char *xml_path;           /* Last path element repeats to form list */

  char *list;
};


struct _TransStringCList2
{
  char *xml_path;
  char *xml_path_field_1;
  char *xml_path_field_2;

  char *clist;
};

struct _TransStringCList
{
  char *xml_path;
  int num_fields;
  char **xml_path_fields;    /* Array of path elements, one for each column in the CList. */

  char *clist;
};

struct _TransStringIPEntry
{
  char *xml_path;

  char *editable_1;
  char *editable_2;
  char *editable_3;
  char *editable_4;
  char *toggle;
};

struct _TransStringCalendar
{
  char *xml_year_path, *xml_month_path, *xml_day_path;
  
  char *calendar;
};


struct _TransTree
{
  TransStringEntry   *transfer_string_entry_table;
  TransStringList    *transfer_string_list_table;
  TransStringCList   *transfer_string_clist_table;
  TransStringIPEntry *transfer_string_ip_table;
  TransStringCalendar *transfer_string_calendar_table;
  TransStringSpin *transfer_string_spin_table;
};

/*
  Some utility functions that can be used in callbacks.
*/

void xml_clist_insert (TransStringCList *trans_clist, int row, xmlNodePtr node);
void xml_clist_append (TransStringCList *trans_clist, xmlNodePtr node);
void xml_from_clist_row (TransStringCList *trans_clist, int row, xmlNodePtr node);


/*
  The routines that do all the work.
 */

void transfer_xml_to_gui (XstTool *tool, gpointer data);
void transfer_gui_to_xml (XstTool *tool, gpointer data);
