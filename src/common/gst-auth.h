/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2001 Ximian, Inc.
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
 * Authors: Carlos Garnacho Parro  <garnacho@tuxerver.net>
 */

#ifndef GST_AUTH_H
#define GST_AUTH_H

#include "gst-types.h"

enum {
	GST_AUTH_PASSWORDLESS,
	GST_AUTH_PASSWORD
} AuthRequired;

void gst_auth_do_ssh_authentication (GstTool*, gchar*);
void gst_auth_do_su_authentication (GstTool*);
#endif /* GST_AUTH_H */
