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
 *          Carlos Garnacho Parro <garparr@teleline.es> (added support for the logos of the distros)
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>

#include "gst-platform.h"
#include "gst-report-line.h"
#include "gst-xml.h"

/* pixmaps used for distros/OS's */
extern GdkPixbuf *redhat;
extern GdkPixbuf *debian;
extern GdkPixbuf *mandrake;
extern GdkPixbuf *turbolinux;
extern GdkPixbuf *slackware;
extern GdkPixbuf *suse;
extern GdkPixbuf *freebsd;
extern GdkPixbuf *gentoo;
extern GdkPixbuf *pld;
extern GdkPixbuf *openna;
extern GdkPixbuf *fedora;
extern GdkPixbuf *conectiva;

GstPlatform *
gst_platform_new (const gchar *key, const gchar *name)
{
	GstPlatform *xp;

	g_return_val_if_fail (name != NULL, NULL);

	xp = g_new0 (GstPlatform, 1);
	xp->key  = g_strdup (key);
	xp->name = g_strdup (name);

	return xp;
}

GstPlatform *
gst_platform_new_from_node (xmlNodePtr node)
{
	gchar *key;
	gchar *name;
	
	g_return_val_if_fail (node != NULL, NULL);

	name = gst_xml_get_child_content (node, "name");
	key = gst_xml_get_child_content (node, "key");

	if (key == NULL) {
		g_free (name);
		g_warning ("Invalid platform XML node: key != NULL failed.");
		return NULL;
	}

	if (name == NULL || *name == 0) {
		g_free (name);
		name = strdup (key);
	}

	return gst_platform_new (key, name);
}

GstPlatform *
gst_platform_new_from_report_line (GstReportLine *rline)
{
	const gchar *key;
	const gchar **argv;
	
	g_return_val_if_fail (rline != NULL, NULL);

	key = gst_report_line_get_key (rline);
		
	g_return_val_if_fail (!strcmp (key, "platform_list") ||
			      !strcmp (key, "platform_success"),
			      NULL);

	argv = gst_report_line_get_argv (rline);
	return gst_platform_new (argv[0], argv[1]);
}

GstPlatform *
gst_platform_dup (GstPlatform *platform)
{
	GstPlatform *new_platform;

	g_return_val_if_fail (platform != NULL, NULL);

	new_platform = gst_platform_new (platform->key, platform->name);

	return new_platform;
}

gint
gst_platform_cmp (GstPlatform *a, GstPlatform *b)
{
	return strcmp (a->key, b->key);
}

void
gst_platform_free (GstPlatform *platform)
{
	g_free (platform->key);
	g_free (platform->name);
	g_free (platform);
}

const gchar *
gst_platform_get_key (GstPlatform *platform)
{
	g_return_val_if_fail (platform != NULL, NULL);
	
	return (platform->key);
}

const GdkPixbuf*
gst_platform_get_pixmap (GstPlatform *platform)
{
	if (g_ascii_strncasecmp (platform->name, "Debian", 6) == 0)
		return debian;
	else if (g_ascii_strncasecmp (platform->name, "Red Hat", 7) == 0)
		return redhat;
	else if (g_ascii_strncasecmp (platform->name, "Linux Mandrake", 14) == 0)
		return mandrake;
	else if (g_ascii_strncasecmp (platform->name, "SuSE", 4) == 0)
		return suse;
	else if (g_ascii_strncasecmp (platform->name, "Turbolinux", 10) == 0)
		return turbolinux;
	else if (g_ascii_strncasecmp (platform->name, "Slackware", 9) == 0)
		return slackware;
	else if (g_ascii_strncasecmp (platform->name, "FreeBSD", 7) == 0)
		return freebsd;
	else if (g_ascii_strncasecmp (platform->name, "Gentoo", 6) == 0)
		return gentoo;
	else if (g_ascii_strncasecmp (platform->name, "PLD", 3) == 0)
		return pld;
	else if (g_ascii_strncasecmp (platform->name, "OpenNA", 6) == 0)
		return openna;
	else if (g_ascii_strncasecmp (platform->name, "Fedora", 6) == 0)
		return fedora;
	else if (g_ascii_strncasecmp (platform->name, "Conectiva", 9) == 0)
		return conectiva;
	else return NULL;
}

const gchar *
gst_platform_get_name (GstPlatform *platform)
{
	g_return_val_if_fail (platform != NULL, NULL);
	
	return (platform->name);
}
