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

#include "gst-report-hook.h"


GstReportHook *
gst_report_hook_new (gchar *key, GstReportHookFunc func, GstReportHookType type,
                     gboolean allow_repeat, gpointer data)
{
	GstReportHook *xrh;

	xrh = g_new0 (GstReportHook, 1);
	xrh->key  = key;
	xrh->func = func;
	xrh->type = type;
	xrh->allow_repeat = allow_repeat;
	xrh->invoked = FALSE;
	xrh->data = data;

	return xrh;
}


GstReportHook *
gst_report_hook_new_from_entry (GstReportHookEntry *entry)
{
	return gst_report_hook_new (entry->key, entry->func, entry->type,
				    entry->allow_repeat, entry->data);
}


void
gst_report_hook_destroy (GstReportHook *xrh)
{
	g_free (xrh);
}
