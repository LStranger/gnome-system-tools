/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* main.c: this file is part of services-admin, a gnome-system-tool frontend 
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
 * Authors: Carlos Garnacho Parro <carlosg@gnome.org>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gst.h"
#include "table.h"
#include "transfer.h"
#include "share-export-smb.h"

GstTool *tool;

int
main (int argc, char *argv[])
{
	GstShareSMB *share;
	
	gst_init ("shares-admin", argc, argv, NULL);

	tool = gst_tool_new ();
	gst_tool_construct (tool, "shares", _("Shared folders settings"));
	gst_tool_set_xml_funcs (tool, transfer_xml_to_gui, NULL, NULL);

	table_create ();

	gst_tool_main (tool, FALSE);
}
