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
 * Authors: Hans Petter Jansson <hpj@ximian.com> and Arturo Espinosa <arturo@ximian.com>.
 */

#include <glade/glade.h>

/* Structures for tables detailing the information types in the XML
   tree, and what widgets they correspond to. */

extern GladeXML *glade_interface;

typedef struct _TransStringEntry TransStringEntry;
typedef struct _TransStringList TransStringList;
typedef struct _TransStringCList2 TransStringCList2;
typedef struct _TransStringIPEntry TransStringIPEntry;

struct _TransStringEntry
{
  gchar *xml_path;

  gchar *editable;
  gchar *toggle;
  int unknown_verbose;      /* Whether to put <unknown> if not found in XML */
};


struct _TransStringList
{
  gchar *xml_path;           /* Path repeats for each item, forms list */

  gchar *list;
};


struct _TransStringCList2
{
  gchar *xml_path;
  gchar *xml_path_field_1;
  gchar *xml_path_field_2;

  gchar *clist;
};


struct _TransStringIPEntry
{
  gchar *xml_path;

  gchar *editable;
  gchar *toggle;
};


void transfer_xml_to_gui(GstTool *t, gpointer data);
void transfer_gui_to_xml(GstTool *t, gpointer data);
