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
 * Authors: Hans Petter Jansson <hpj@ximian.com>.
 */

#include <glib.h>
#include <string.h>
#include "xst-util.h"

/**
 * xst_util_nice_hostname: Removes typical superfluous syntax from a network hostname in-place.
 * @hostname: Unpretty hostname.
 *
 * Typical formats are "\\hostname\", "//hostname/" and "hostname/". The resulting format
 * is "hostname". The transformation is done in-place.
 **/
void
xst_util_nice_hostname (gchar *hostname)
{
	gchar *p0;

	g_return_if_fail (hostname != NULL);

	/* Remove leading and trailing blanks */

	g_strstrip (hostname);
	
	/* Remove leading slashes (both kinds) */

	for (p0 = hostname; *p0 && (*p0 == '\\' || *p0 == '/'); p0++)
		;
	
	if (p0 != hostname)
		memmove (hostname, p0, strlen (p0) + 1);
	
	/* Remove trailing slashes (both kinds) */
	
	for (p0 = hostname + strlen (hostname) - 1; p0 >= hostname; p0--)
	{
		if (*p0 != '\\' && *p0 != '/')
			break;

		*p0 = '\0';
	}
}

/**
 * xst_util_nice_hostname_dup: Removes typical superfluous syntax from a network hostname.
 * @hostname: Unpretty hostname.
 *
 * Typical formats are "\\hostname\", "//hostname/" and "hostname/". The resulting format
 * is "hostname".
 *
 * Return value: The prettified hostname.
 **/
gchar *
xst_util_nice_hostname_dup (gchar *hostname)
{
	gchar *new_hostname;

	g_return_val_if_fail (hostname != NULL, NULL);

	new_hostname = g_strdup (hostname);
	xst_util_nice_hostname (new_hostname);
	return new_hostname;
}

/**
 * xst_util_nice_path_dup: Removes typical superfluous syntax from a network path.
 * @hostname: Unpretty path.
 *
 * Typical formats are "\path", "path/" and "\path\". The resulting format
 * is "/path" (emulates an absolute path).
 *
 * Return value: The prettified path.
 **/
gchar *
xst_util_nice_path_dup (gchar *path)
{
	gchar *path1, *path2, *p0;

	g_return_val_if_fail (path != NULL, NULL);

	path1 = g_strdup (path);

	/* Remove leading and trailing blanks */

	g_strstrip (path1);

	if (!*path1)
		return path1;  /* User meant it to be empty */

	/* Remove leading slashes (both kinds) */

	for (p0 = path1; *p0 && (*p0 == '\\' || *p0 == '/'); p0++)
		;

	if (p0 != path1)
		memmove (path1, p0, strlen (p0) + 1);

	/* Remove trailing slashes (both kinds) */

	for (p0 = path1 + strlen (path1) - 1; p0 >= path1; p0--)
	{
		if (*p0 != '\\' && *p0 != '/')
			break;

		*p0 = '\0';
	}

	/* Ensure a leading slash */

	if (path1 [0] != '/')
	{
		path2 = g_strconcat ("/", path1, NULL);
		g_free (path1);
		path1 = path2;
	}

	return (path1);
}
