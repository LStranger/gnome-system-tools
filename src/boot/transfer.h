/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* transfer.h: this file is part of boot-admin, a ximian-setup-tool frontend 
 * for boot administration.
 * 
 * Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Tambet Ingo <tambet@ximian.com>.
 */

#ifndef __TRANSFER_H
#define __TRANSFER_H

#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>

typedef struct _TransStringCList TransStringCList;

struct _TransStringCList
{
  gchar *xml_path;
  gchar *xml_path_field_1;
  gchar *xml_path_field_2;

  gchar *clist;
};

void transfer_xml_to_gui(XstTool *tool, gpointer data);
void transfer_gui_to_xml(XstTool *tool, gpointer data);

#endif /* TRANSFER_H */
