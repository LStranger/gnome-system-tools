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
#include <gnome.h>

#include "gst-report-line.h"

/* printf, perl style, with a 1024 char limit. Gets the job done. */
static gchar *
gst_report_sprintf (gchar *fmt, gchar **argv)
{
	char *orig_fmt;
	char str[1024], ret[1024];
	char *c, *fmt_p, *fmt_p2;
	int i;

	orig_fmt = g_strdup (fmt);
 	if (!argv[0])
		return orig_fmt;
	
	ret[0] = 0;
	fmt_p = orig_fmt;
	c = strchr (orig_fmt, '%');
	if (!c)
		return orig_fmt;
	
	for (i = 0; argv[i]; i++) {
		if (c)
			*c = '%';
		else {
			g_warning ("Excess arguments given for [%s].", fmt);
			break;
		}
		
		c = strchr (c + 1, '%');
		if (c)
			*c = 0;
		fmt_p2 = c;
		
		snprintf (str, 1023, fmt_p, argv[i]);
		fmt_p = fmt_p2;
		strncpy (ret + strlen (ret), str, 1023 - strlen (ret));
	}

	g_free (orig_fmt);
	return g_strdup (ret);
}

GstReportLine *
gst_report_line_new (GstReportMajor major, gchar *key, gchar *fmt, gchar **argv)
{
	GstReportLine *xrl;
	gchar *str;

	xrl = g_new0 (GstReportLine, 1);
	xrl->major = major;
	xrl->key = g_strdup (key);
	xrl->fmt = g_strdup (fmt);

	if (major == GST_MAJOR_DEBUG)
		g_print ("debug\n");

	/* This code duplicates the argv */
	str = g_strjoinv ("::", argv);
	xrl->argv = g_strsplit (str, "::", 0);
	g_free (str);
	
	xrl->message = gst_report_sprintf (fmt, argv);
	xrl->handled = FALSE;

	return xrl;
}

static GstReportMajor
gst_report_line_str_to_major (gchar *string)
{
	struct {
		GstReportMajor  major;
		gchar          *str;
	} table[] = {
		{ GST_MAJOR_SYS,     "sys" },
		{ GST_MAJOR_ERROR,   "error" },
		{ GST_MAJOR_WARN,    "warn" },
		{ GST_MAJOR_INFO,    "info" },
		{ GST_MAJOR_DEBUG,   "debug" },
		{ GST_MAJOR_INVALID, NULL }
	};

	gint i;

	for (i = 0; table[i].major != GST_MAJOR_INVALID; i++)
		if (!strcmp (string, table[i].str))
			return table[i].major;
	return GST_MAJOR_INVALID;
}

static gchar **
gst_report_line_parse_string (gchar *string)
{
	gchar *s;
	GString *str;
	GList *list, *l;
	gchar **parts;
	gint i;

	g_return_val_if_fail (string != NULL, NULL);

	str = g_string_new ("");
	list = g_list_append (NULL, str);
	for (s = string; *s; s++) {
		switch (*s) {
		case '\\':
			if (*(s + 1) == '\\') {
				g_string_append_c (str, '\\');
				s++;
				break;
			}

			if ((*(s + 1) == ':') &&
			    (*(s + 2) == ':')) {
				g_string_append (str, "::");
				s+=2;
				break;
			}

			g_string_append_c (str, '\\');
			break;
		case ':':
			if (*(s + 1) == ':') {
				str = g_string_new ("");
				g_list_append (list, str);
				s++;
				break;
			}
			
			g_string_append_c (str, ':');
			break;
		default:
			g_string_append_c (str, *s);
		}
	}

	parts = g_new0 (gchar *, g_list_length (list) + 1);

	for (i = 0, l = list; l; i++, l = l->next) {
		str = (GString *) l->data;
		parts[i] = str->str;
		g_string_free (str, FALSE);
	}

	g_list_free (list);
	
	return parts;
}

GstReportLine *
gst_report_line_new_from_string (gchar *string)
{
	GstReportLine *xrl;
	GstReportMajor major;
	gchar **parts;

	if (strlen (string) <= 1)
		return NULL;

	parts = gst_report_line_parse_string (string);

	/* must have at least major, minor and format */
	if (!parts[2]) {
		g_strfreev (parts);
		return NULL;
	}

	major = gst_report_line_str_to_major (parts[0]);

	g_return_val_if_fail (major != GST_MAJOR_INVALID, NULL);
	
	xrl = gst_report_line_new (major, parts[1], parts[2], &parts[3]);
	g_strfreev (parts);

	return xrl;
}

void
gst_report_line_free (GstReportLine *line)
{
	g_free (line->message);
	g_free (line->key);
	g_free (line->fmt);
	if (line->argv)
		g_strfreev (line->argv);
	g_free (line);
}

const gchar *
gst_report_line_get_key (GstReportLine *line)
{
	g_return_val_if_fail (line != NULL, 0);
	return (line->key);
}

const gchar **
gst_report_line_get_argv (GstReportLine *line)
{
	g_return_val_if_fail (line != NULL, 0);
	return (const gchar **) (line->argv);
}

const gchar *
gst_report_line_get_message (GstReportLine *line)
{
	g_return_val_if_fail (line != NULL, NULL);
	return (line->message);
}

gboolean
gst_report_line_get_handled (GstReportLine *line)
{
	g_return_val_if_fail (line != NULL, FALSE);
	return (line->handled);
}

void
gst_report_line_set_handled (GstReportLine *line, gboolean handled)
{
	g_return_if_fail (line != NULL);

	line->handled = handled;
}
