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

#ifndef GST_REPORT_HOOK_H
#define GST_REPORT_HOOK_H

#include "gst-types.h"
#include "gst-report-line.h"
#include "gst-tool.h"

/* The return value defines if more hooks to the same entry type should be called. */
typedef gboolean (GstReportHookFunc) (GstTool *tool, GstReportLine *rline, gpointer data);

/* Internal storage */

struct _GstReportHook {
	gchar             *key;
	GstReportHookFunc *func;
	GstReportHookType  type;
	gboolean           allow_repeat;
	gboolean           invoked;
	gpointer           data;
};

/* Arranged in arrays, allows for easy loading of report-hook entries */

struct _GstReportHookEntry {
	gchar             *key;
	GstReportHookFunc *func;
	GstReportHookType  type;
	gboolean           allow_repeat;
	gpointer           data;
};

GstReportHook     *gst_report_hook_new             (gchar             *key,
						    GstReportHookFunc  func,
                                                    GstReportHookType  type,
                                                    gboolean           allow_repeat,
						    gpointer           data);
GstReportHook     *gst_report_hook_new_from_entry  (GstReportHookEntry *entry);
void               gst_report_hook_destroy         (GstReportHook *xrh);

#endif /* GST_REPORT_HOOK_H */
