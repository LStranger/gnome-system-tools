/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* transfer.c: this file is part of shares-admin, a gnome-system-tool frontend 
 * for run level services administration.
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
 * Authors: Carlos Garnacho <carlosg@gnome.org>.
 */

#include "gst.h"

void
transfer_xml_to_gui (GstTool *tool, gpointer data)
{
	   xmlNodePtr root, export;

	   root   = gst_xml_doc_get_root (tool->config);
	   export = gst_xml_element_find_first (root, "exports");

	   for (export = gst_xml_element_find_first (export, "export");
		   export;
		   export = gst_xml_element_find_next (export, "export")) {
			 table_add_node (export);
	   }
}
