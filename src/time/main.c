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
static void ntp_use_toggled (GtkWidget *w, GstDialog *dialog);
static void gst_time_calendar_change_cb (GtkCalendar *, gpointer);
static void on_server_list_element_toggled (GtkCellRendererToggle*, gchar*, gpointer);

struct NtpServer {
	const gchar *url;
	const gchar *location;
} ntp_servers [] = {
	{ "time.nrc.ca", "Canada" },
	{ "ntp1.cmc.ec.gc.ca", "Eastern Canada" },
	{ "ntp2.cmc.ec.gc.ca", "Eastern Canada" },
	{ "clock.tricity.wsu.edu", "Washington, USA" },
	{ "wuarchive.wustl.edu", "Missouri, USA" },
	{ "clock.psu.edu", "Pennsylvania, USA" },
	{ "constellation.ecn.uoknor.edu", "Oklahoma, USA" },
	{ "gilbreth.ecn.purdue.edu", "Indiana, USA" },
	{ "molecule.ecn.purdue.edu", "Indiana, USA" },
	{ "libra.rice.edu", "Texas, USA" },
	{ "ntp.cox.smu.edu", "Texas, USA" },
	{ "ntp.tmc.edu", "Texas, USA" },
	{ "louie.udel.edu", "Delaware, USA" },
	{ "ntp.cmr.gov", "Virginia, USA" },
	{ "ntp0.cornell.edu", "New York, USA" },
	{ "ntp-0.cso.uiuc.edu", "Illinois, USA" },
	{ "ntp1.cs.wisc.edu", "Wisconsin, USA" },
	{ "tick.cs.unlv.edu", "Las Vegas, USA" },
	{ "ntp2a.mcc.ac.uk", "England, Europe" },
	{ "ntp2b.mcc.ac.uk", "England, Europe" },
	{ "salmon.maths.tcd.ie", "Ireland, Europe" },
	{ "ntp.cs.strath.ac.uk", "Scotland, Europe" },
	{ "bernina.ethz.ch", "Switzerland, Europe" },
	{ "ntp.univ-lyon1.fr", "France, Europe" },
	{ "tick.keso.fi", "Finland, Europe" },
	{ "fartein.ifi.uio.no", "Norway, Europe" },
	{ "ntp1.arnes.si", "Slovenia, Europe" },
	{ "ntp2.arnes.si", "Slovenia, Europe" },
	{ "ntp.landau.ac.ru", "Moscow, Russia" },
	{ "time.esec.com.au", "Australia" },
	{ "ntp.adelaide.edu.au", "South Australia" },
	{ "ntp.shim.org", "Singapore, Asia" },
	{ "time.nuri.net", "Korea, Asia" },
	{ "ntp.cs.mu.oz.au", "Melbourne, Australia" },
	{ "ntp.mel.nml.csiro.au", "Melbourne, Australia" },
	{ "ntp.nml.csiro.au", "Sydney, Australia" },
	{ "ntp.per.nml.csiro.au", "Perth, Australia" },
	{ "swisstime.ethz.ch", "Zurich, Switzerland" },
	{ "ntp.cesnet.cz", "Prague, Czech Republic" },
	{ "ntpa2.kph.uni-mainz.de", "Mainz, Germany" },
	{ "ntps1-0.cs.tu-berlin.de", "Berlin, Germany" },
	{ "ntps1-1.cs.tu-berlin.de", "Berlin, Germany" },
	{ "ntps1-2.uni-erlangen.de", "Erlangen, Germany" },
	{ "canon.inria.fr", "Rocquencourt, France" },
	{ "chronos.cru.fr", "Britany, France" },
	{ "stdtime.gov.hk", "Hong Kong, China" },
	{ "clock.cuhk.edu.hk", "Hong Kong, China" },
	{ "time.ien.it", "Torino, Italy" },
	{ "clock.tl.fukuoka-u.ac.jp", "Fukuoka, Japan" },
	{ "cronos.cenam.mx", "Queretaro, Mexico" },
	{ "ntp0.nl.net", "Amsterdam, The Netherlands" },
	{ "ntp1.nl.net", "Amsterdam, The Netherlands" },
	{ "ntp2.nl.net", "Amsterdam, The Netherlands" },
	{ "time.service.uit.no", "Norway" },
	{ "ntp.certum.pl", "Poland" },
	{ "vega.cbk.poznan.pl", "Borowiec, Poland" },
	{ "time1.stupi.se", "Stockholm, Sweden" },
	{ "goodtime.ijs.si", "Ljubljana, Slovenia" },
	{ "ntp2.ja.net", "United Kingdom" },
	{ NULL }
};

static GstDialogSignal signals[] = {
	{ "timezone_button",     "clicked",  G_CALLBACK (timezone_button_clicked) },
	{ "ntp_use",             "toggled",  G_CALLBACK (ntp_use_toggled) },
	{ "timeserver_button",   "clicked",  G_CALLBACK (server_button_clicked) },
	{ "ntp_add_server",      "clicked",  G_CALLBACK (on_ntp_addserver) },
	{ NULL, NULL }
};

static void
populate_ntp_list (GstTimeTool *time_tool)
{
	GstTool *tool = GST_TOOL (time_tool);
	GtkWidget *ntp_list, *item;
	GList *list_add = 0;
	GtkListStore *store;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GtkTreeIter iter;
	gchar *str;
	gint i;

	ntp_list = gst_dialog_get_widget (tool->main_dialog, "ntp_list");

	/* set the model */
	store = gtk_list_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (ntp_list),
				 GTK_TREE_MODEL (store));

	column = gtk_tree_view_column_new ();

	cell = gtk_cell_renderer_toggle_new ();
	gtk_tree_view_column_pack_start (column, cell, FALSE);
	gtk_tree_view_column_add_attribute (column, cell, "active", 0);
	g_object_set_data (G_OBJECT (cell), "tool", tool);
	g_signal_connect (G_OBJECT (cell), "toggled", G_CALLBACK (on_server_list_element_toggled), store);
	
	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_end (column, cell, TRUE);
	gtk_tree_view_column_add_attribute (column, cell, "text", 1);

	gtk_tree_view_append_column (GTK_TREE_VIEW (ntp_list), column);
	
	for (i = 0; ntp_servers[i].url; i++) {
		str = g_strdup_printf ("%s (%s)", ntp_servers[i].url, ntp_servers[i].location);

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    0, FALSE,
				    1, str,
				    2, ntp_servers[i].url,
				    -1);
		g_free (str);
	}
}

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
ntp_use_toggled (GtkWidget *w, GstDialog *dialog)
{
	GstTool *tool;
	gboolean active;

	tool = gst_dialog_get_tool (dialog);
	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

	if (!GST_TIME_TOOL (tool)->ntp_service && active) {
		GtkWidget *message;

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), FALSE);

		message = gtk_message_dialog_new (GTK_WINDOW (dialog),
						  GTK_DIALOG_MODAL,
						  GTK_MESSAGE_INFO,
						  GTK_BUTTONS_CLOSE,
						  _("NTP support is not installed"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
							  _("Please install and activate NTP support in the system to enable "
							    "synchronization of your local time server with "
							    "internet time servers."));
		gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);
		return;
	}

	/* FIXME: enable NTP for the current runlevel */

	gtk_widget_set_sensitive (gst_dialog_get_widget (dialog, "timeserver_button"), active);
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
	populate_ntp_list (GST_TIME_TOOL (tool));

	gtk_widget_show (GTK_WIDGET (tool->main_dialog));
	
	gtk_main ();

	return 0;
}
