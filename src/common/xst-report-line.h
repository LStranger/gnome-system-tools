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

#ifndef XST_REPORT_LINE_H
#define XST_REPORT_LINE_H

#include "xst-types.h"
#include "xst-tool.h"

struct _XstReportLine {
	guint id;
	gchar *message;
	guint handled : 1;
};

XstReportLine     *xst_report_line_new              (guint id, gchar *message);
XstReportLine     *xst_report_line_new_from_string  (gchar *string);
void               xst_report_line_free             (XstReportLine *line);

guint              xst_report_line_get_id           (XstReportLine *line);
const gchar       *xst_report_line_get_message      (XstReportLine *line);
gboolean           xst_report_line_get_handled      (XstReportLine *line);

void               xst_report_line_set_handled      (XstReportLine *line, gboolean handled);

#endif /* XST_REPORT_LINE_H */
