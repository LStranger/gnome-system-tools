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
 * Authors: Hans Petter Jansson <hpj@ximian.com>
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>

#include "xst-platform.h"
#include "xst-report-line.h"

XstPlatform *
xst_platform_new (const gchar *name)
{
	XstPlatform *xp;

	g_return_val_if_fail (name != NULL, NULL);

	xp = g_new0 (XstPlatform, 1);
	xp->name = g_strdup (name);

	return xp;
}

XstPlatform *
xst_platform_new_from_report_line (XstReportLine *rline)
{
	const gchar *key;
	
	g_return_val_if_fail (rline != NULL, NULL);

	key = xst_report_line_get_key (rline);
		
	g_return_val_if_fail (!strcmp (key, "platform_list") ||
			      !strcmp (key, "platform_success"),
			      NULL);

	return xst_platform_new (xst_report_line_get_argv (rline)[0]);
}

XstPlatform *
xst_platform_dup (XstPlatform *platform)
{
	XstPlatform *new_platform;

	g_return_val_if_fail (platform != NULL, NULL);

	new_platform = g_new0 (XstPlatform, 1);
	new_platform->name = g_strdup (platform->name);

	return new_platform;
}

void
xst_platform_free (XstPlatform *platform)
{
	g_free (platform->name);
	g_free (platform);
}

const gchar *
xst_platform_get_name (XstPlatform *platform)
{
	g_return_val_if_fail (platform != NULL, NULL);
	
	return (platform->name);
}
