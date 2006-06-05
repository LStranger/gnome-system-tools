/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* main.c: this file is part of time-admin, a ximian-setup-tool frontend 
 * for system time configuration.
 * 
 * Copyright (C) 2000-2001 Ximian, Inc.
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
 *          Jacob Berkman <jacob@ximian.com>
 *          Chema Celorio <chema@ximian.com>
 *          Carlos Garnacho Parro <garparr@teleline.es>
 */
 
#include <config.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gst.h"
#include "time-tool.h"

#include "tz.h"
#include "timeserv.h"
#include "e-map/e-map.h"
#include "tz-map.h"

ETzMap *tzmap;

static void timezone_button_clicked (GtkWidget *w, gpointer data);
static void update_tz (GstTimeTool *time_tool);
static void server_button_clicked (GtkWidget *w, gpointer data);
static void gst_time_calendar_change_cb (GtkCalendar *, gpointer);
static void on_server_list_element_toggled (GtkCellRendererToggle*, gchar*, gpointer);

static GstDialogSignal signals[] = {
	{ "timezone_button",     "clicked",  G_CALLBACK (timezone_button_clicked) },
	{ "timeserver_button",   "clicked",  G_CALLBACK (server_button_clicked) },
	{ "ntp_add_server",      "clicked",  G_CALLBACK (on_ntp_addserver) },
	{ NULL, NULL }
};

#define is_leap_year(yyy) ((((yyy % 4) == 0) && ((yyy % 100) != 0)) || ((yyy % 400) == 0));

static void
gst_time_update_date (GstTimeTool *tool, gint add)
{
	static const gint month_length[2][13] =
	{
		{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
		{ 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
	};
	GtkWidget *calendar;
	guint day, month, year;
	gint days_in_month;
	gboolean leap_year;

	calendar = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog,
					  "calendar");
	gtk_calendar_get_date (GTK_CALENDAR (calendar),
			       &year, &month, &day);

	/* Taken from gtk_calendar which was taken from lib_date */
	leap_year = is_leap_year (year);
	days_in_month = month_length [leap_year][month+1];
	
	if (add != 0) {
		day += add;
		if (day < 1) {
			day = (month_length [leap_year][month]) + day;
			month--;
		} else if (day > days_in_month) {
			day -= days_in_month;
			month++;
		}

		if (month < 0) {
			year--;
			leap_year = is_leap_year (year);
			month = 11;
			day = month_length [leap_year][month+1];
		} else if (month > 11) {
			year++;
			leap_year = is_leap_year (year);
			month = 0;
			day = 1;
		}
	}
	
	gtk_calendar_select_month (GTK_CALENDAR (calendar),
				   month, year);
	gtk_calendar_select_day (GTK_CALENDAR (calendar),
				 day);
}
#undef is_leap_year

static void
on_server_list_element_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
	/* FIXME
	GtkListStore *store = (GtkListStore *)data;
	GtkTreeModel *model = GTK_TREE_MODEL (store);
	GtkTreeIter iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	GstTool *tool = g_object_get_data (G_OBJECT (cell), "tool");
	gboolean toggle;
	gchar *server, *p;
	xmlNodePtr root, node;

	root = gst_xml_doc_get_root (tool->config);
	node = gst_xml_element_find_first (root, "sync");
	if (!node) 
		node = gst_xml_element_add (root, "sync");

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &toggle, 1, &server, -1);

	p = (char *) strchr (server, ' ');
	if (p) 
		*p = '\0';  /* Kill comments */
/*	
	if (toggle == TRUE) {
		/* Toggle is set to true, we have to delete entry in XML and set toggle to FALSE */
/*		toggle = FALSE;
		for (node = gst_xml_element_find_first (node, "server"); node != NULL; node = gst_xml_element_find_next (node, "server")) {
			if (strcmp (server, gst_xml_element_get_content (node)) == 0) 
				gst_xml_element_destroy (node);
		}
	} else {
		/* Toggle is set to false, we have to add a server entry in the XML and set toggle to TRUE */
/*		toggle = TRUE;
		node = gst_xml_element_add (node, "server");
				gst_xml_element_set_content (node, server);
	}

	gtk_list_store_set (store, &iter, 0, toggle, -1);
	gtk_tree_path_free (path);
	gst_dialog_modify (tool->main_dialog);
*/
}

static void
timezone_button_clicked (GtkWidget *w, gpointer data)
{
	GstTimeTool *time_tool;
	GstDialog *dialog;

	dialog = GST_DIALOG (data);
	time_tool = GST_TIME_TOOL (gst_dialog_get_tool (dialog));
	gst_time_tool_run_timezone_dialog (time_tool);
}

static void
server_button_clicked (GtkWidget *w, gpointer data)
{
	GtkWidget *d;
	GstDialog *dialog;

	dialog = GST_DIALOG (data);

	d = gst_dialog_get_widget (dialog, "time_server_window");

	gtk_window_set_transient_for (GTK_WINDOW (d), GTK_WINDOW (dialog));

	while (gtk_dialog_run (GTK_DIALOG (d)) == GTK_RESPONSE_HELP);
	gtk_widget_hide (d);
}

static void
gst_time_calendar_change_cb (GtkCalendar *calendar, gpointer data)
{
	/* FIXME
	GstTimeTool *tool = (GstTimeTool *)data;

	gst_time_clock_stop (tool);
	gst_dialog_modify (GST_TOOL (tool)->main_dialog);
	*/
}

static void
gst_time_change (GtkSpinButton *widget, gpointer data)
{
	/*
	GstTimeTool *tool = data;
	gint value = gtk_spin_button_get_value (widget);
	gchar *val;

	g_return_if_fail (GTK_IS_SPIN_BUTTON (widget));
	g_return_if_fail (GST_IS_TIME_TOOL (tool));

	if (widget == GTK_SPIN_BUTTON (tool->seconds)) {
		if (value > 59) {
			gtk_spin_button_set_value (widget, value - 60);
			gtk_spin_button_spin (GTK_SPIN_BUTTON (tool->minutes), GTK_SPIN_STEP_FORWARD, 1);
			tool->min++;
		} else if (value < 0) {
			gtk_spin_button_set_value (widget, value + 60);
			gtk_spin_button_spin (GTK_SPIN_BUTTON (tool->minutes), GTK_SPIN_STEP_BACKWARD, 1);
			tool->min--;
		}
	} else if (widget == GTK_SPIN_BUTTON (tool->minutes)) {
		if (value > 59) {
			gtk_spin_button_set_value (widget, value - 60);
			gtk_spin_button_spin (GTK_SPIN_BUTTON (tool->hours), GTK_SPIN_STEP_FORWARD, 1);
			tool->hrs++;
		} else if (value < 0) {
			gtk_spin_button_set_value (widget, value + 60);
			gtk_spin_button_spin (GTK_SPIN_BUTTON (tool->hours), GTK_SPIN_STEP_BACKWARD, 1);
			tool->hrs--;
		}
	} else if (widget == GTK_SPIN_BUTTON (tool->hours)) {
		if (value > 23) {
			gtk_spin_button_set_value (widget, value - 24);
			gst_time_update_date (tool, +1);
		} else if (value < 0) {
			gst_time_update_date (tool, -1);
			gtk_spin_button_set_value (widget, value + 24);
		}
	}

	gst_dialog_modify (GST_TOOL (tool)->main_dialog);
	gst_time_clock_stop (tool);
	
	/* We have to set it to 01 instead of 1, it's more pretty */
/*	val = g_strdup_printf ("%02d", value);
	gtk_entry_set_text (GTK_ENTRY (widget), val);
	g_free (val);
*/
}

/*
void
gst_time_set_from_localtime (GstTimeTool *time_tool, gint correction)
{
	struct tm *tm;
	time_t tt;

	tt = time (NULL);
	tt += correction; 
	tm = localtime (&tt);

	gst_time_set_full (time_tool, tm);
}
*/

int
main (int argc, char *argv[])
{
	GstTool *tool;

	gst_init_tool ("time-admin", argc, argv, NULL);
	tool = GST_TOOL (gst_time_tool_new ());

	gst_dialog_connect_signals (tool->main_dialog, signals);
	gtk_widget_show (GTK_WIDGET (tool->main_dialog));
	gtk_main ();

	return 0;
}
