/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* transfer.c: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
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
 * Authors: Tambet Ingo <tambet@ximian.com> and Arturo Espinosa <arturo@ximian.com>.
 */

/* Functions for transferring information between XML tree and UI */


#include <gnome.h>

#include "transfer.h"
#include "e-table.h"
#include "profile.h"

void
transfer_xml_to_gui (XstTool *tool, gpointer data)
{
	xmlNodePtr root;

	root = xst_xml_doc_get_root (tool->config);

	/* Profiles */
	profile_table_init ();
	profile_table_from_xml (root);

	/* Popuplate tables */
	populate_all_tables ();
}

void
transfer_gui_to_xml (XstTool *tool, gpointer data)
{
}
