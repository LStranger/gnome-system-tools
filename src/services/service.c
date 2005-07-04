/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* table.h: this file is part of services-admin, a gnome-system-tool frontend 
 * for run level services administration.
 * 
 * Copyright (C) 2002 Ximian, Inc.
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

#include "service.h"
#include <gtk/gtk.h>
#include "gst.h"

/* this has to be sorted */
static ServiceDescription descriptions[] = {
	{ "COMMAND_SCHEDULER",    FALSE, "stock_alarm",       N_("Actions scheduler"),             N_("Executes scheduled actions") },
	{ "DISPLAY_MANAGER",      TRUE,  "gdm",               N_("Graphical login manager"),       N_("Requests user and password in a graphical fashion") },
	{ "FILE_SERVER",          FALSE, "gnome-fs-share",    N_("Folder sharing service"),        N_("Shares your selected folders through your network") },
	{ "MTA",                  FALSE, "stock_mail",        N_("Mail agent"),                    N_("Delivers your sent mails through internet") },
	{ "NTP_SERVER",           FALSE, "clock",             N_("Clock synchronization service"), N_("Synchronizes your computer clock with internet time servers") },
	{ "PRINTER_SERVICE",      FALSE, "gnome-dev-printer", N_("Printer service"),               N_("Allows applications to use printers") },
	{ "SECURE_SHELL_SERVER",  FALSE, "gnome-terminal",    N_("Remote shell server"),           N_("Secure shell server") },
	{ "SYSTEM_LOGGER",        FALSE, "logviewer",         N_("Computer activity logger"),      N_("Keeps a log of your computer activity") },
	{ "WEB_SERVER",           FALSE, "apacheconf",        N_("Web server"),                    N_("Shares your web pages through internet") },
};

static int
compare_descriptions (const void *p1, const void *p2)
{
	ServiceDescription *desc1 = (ServiceDescription *) p1;
	ServiceDescription *desc2 = (ServiceDescription *) p2;

	return strcmp (desc1->role, desc2->role);
}

const ServiceDescription*
service_search (const gchar *role)
{
	ServiceDescription s;

	s.role = (gchar *) role;

	return bsearch (&role, descriptions, G_N_ELEMENTS (descriptions),
			sizeof (ServiceDescription), compare_descriptions);
}
