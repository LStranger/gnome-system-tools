/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* transfer.h: this file is part of disks-admin, a gnome-system-tool frontend 
 * for disks administration.
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
 * Authors: Alvaro Pe�a Gonzalez <apg@esware.com>
 *          Carlos Garc�a Campos <elkalmail@yahoo.es>
 */

#ifndef __TRANSFER_H
#define __TRANSFER_H

typedef struct _TransStringCList TransStringCList;

struct _TransStringCList
{
  gchar *xml_path;
  gchar *xml_path_field_1;
  gchar *xml_path_field_2;

  gchar *clist;
};

void transfer_xml_to_gui(GstTool *tool, gpointer data);
void transfer_gui_to_xml(GstTool *tool, gpointer data);

#endif /* TRANSFER_H */
