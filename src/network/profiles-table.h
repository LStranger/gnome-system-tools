/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
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
 * Authors: Carlos Garnacho Parro  <carlosg@gnome.org>
 */

#ifndef __PROFILES_TABLE_H__
#define __PROFILES_TABLE_H__

#include "gst.h"

enum {
	   PROFILES_TABLE_COL_NAME,
	   PROFILES_TABLE_COL_DESCRIPTION,
	   PROFILES_TABLE_COL_POINTER,
	   PROFILES_TABLE_COL_LAST
};

void profiles_table_create (GstTool*);
void profiles_table_populate (GstTool*, xmlNodePtr);


#endif /* __PROFILES_TABLE_H__ */
