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

#include "xst-report-line.h"

XstReportLine *
xst_report_line_new (guint id, gchar *message)
{
	XstReportLine *xrl;

	g_return_val_if_fail (message != NULL, NULL);

	xrl = g_new0 (XstReportLine, 1);
	xrl->id = id;
	xrl->message = g_strdup (message);
	xrl->handled = FALSE;

	return xrl;
}

XstReportLine *
xst_report_line_new_from_string (gchar *string)
{
	XstReportLine *xrl;
	gchar **parts;

	g_return_val_if_fail (strlen (string) > 2, NULL);
	g_return_val_if_fail (strchr (string, ' ') != NULL, NULL);

	parts = g_strsplit (string, " ", 1);
	xrl = xst_report_line_new (atoi (parts [0]), parts [1]);
	g_strfreev (parts);

	return xrl;
}

void
xst_report_line_free (XstReportLine *line)
{
	g_free (line->message);
	g_free (line);
}

guint
xst_report_line_get_id (XstReportLine *line)
{
	g_return_val_if_fail (line != NULL, 0);
	return (line->id);
}

const gchar *
xst_report_line_get_message (XstReportLine *line)
{
	g_return_val_if_fail (line != NULL, NULL);
	return (line->message);
}

gboolean
xst_report_line_get_handled (XstReportLine *line)
{
	g_return_val_if_fail (line != NULL, FALSE);
	return (line->handled);
}

void
xst_report_line_set_handled (XstReportLine *line, gboolean handled)
{
	g_return_if_fail (line != NULL);

	line->handled = handled;
}
