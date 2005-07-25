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

#include <gnome.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gst.h"
#include "time-tool.h"

#include "tz.h"
#include "timeserv.h"
#include "transfer.h"
#include "e-map/e-map.h"
#include "tz-map.h"

ETzMap *tzmap;

static void timezone_button_clicked (GtkWidget *w, gpointer data);
static void update_tz (GstTimeTool *time_tool);
static void server_button_clicked (GtkWidget *w, gpointer data);
static void ntp_use_toggled (GtkWidget *w, GstDialog *dialog);
static void gst_time_calendar_change_cb (GtkCalendar *, gpointer);
static void on_server_list_element_toggled (GtkCellRendererToggle*, gchar*, gpointer);
static void on_timezone_help_button_clicked (GtkWidget*, gpointer);
static void on_time_server_help_button_clicked (GtkWidget*, gpointer);

static char *ntp_servers[] =
{
	"time.nrc.ca (Canada)",
	"ntp1.cmc.ec.gc.ca (Eastern Canada)",
	"ntp2.cmc.ec.gc.ca (Eastern Canada)",
	"clock.tricity.wsu.edu (Washington, USA)",
	"wuarchive.wustl.edu (Missouri, USA)",
	"clock.psu.edu (Pennsylvania, USA)",
	"constellation.ecn.uoknor.edu (Oklahoma, USA)",
	"gilbreth.ecn.purdue.edu (Indiana, USA)",
	"molecule.ecn.purdue.edu (Indiana, USA)",
	"libra.rice.edu (Texas, USA)",
	"ntp.cox.smu.edu (Texas, USA)",
	"ntp.tmc.edu (Texas, USA)",
	"louie.udel.edu (Delaware, USA)",
	"ntp.cmr.gov (Virginia, USA)",
	"ntp0.cornell.edu (New York, USA)",
	"ntp-0.cso.uiuc.edu (Illinois, USA)",
	"ntp1.cs.wisc.edu (Wisconsin, USA)",
	"tick.cs.unlv.edu (Las Vegas, USA)",
	"ntp2a.mcc.ac.uk (England, Europe)",
	"ntp2b.mcc.ac.uk (England, Europe)",
	"salmon.maths.tcd.ie (Ireland, Europe)",
	"ntp.cs.strath.ac.uk (Scotland, Europe)",
	"bernina.ethz.ch (Switzerland, Europe)",
	"ntp.univ-lyon1.fr (France, Europe)",
	"tick.keso.fi (Finland, Europe)",
	"fartein.ifi.uio.no (Norway, Europe)",
	"ntp1.arnes.si (Slovenia, Europe)",
	"ntp2.arnes.si (Slovenia, Europe)",
	"ntp.landau.ac.ru (Moscow, Russia)",
	"time.esec.com.au (Australia)",
	"ntp.adelaide.edu.au (South Australia)",
	"ntp.shim.org (Singapore, Asia)",
	"time.nuri.net (Korea, Asia)",
	"ntp.cs.mu.oz.au (Melbourne, Australia)",
	"ntp.mel.nml.csiro.au (Melbourne, Australia)",
	"ntp.nml.csiro.au (Sydney, Australia)",
	"ntp.per.nml.csiro.au (Perth, Australia)",
	"swisstime.ethz.ch (Zurich, Switzerland)",
	"ntp.cesnet.cz (Prague, Czech Republic)",
	"ntpa2.kph.uni-mainz.de (Mainz, Germany)",
	"ntps1-0.cs.tu-berlin.de (Berlin, Germany)",
	"ntps1-1.cs.tu-berlin.de (Berlin, Germany)",
	"ntps1-2.uni-erlangen.de (Erlangen, Germany)",
	"canon.inria.fr (Rocquencourt, France)",
	"chronos.cru.fr (Britany, France)",
	"stdtime.gov.hk (Hong Kong, China)",
	"clock.cuhk.edu.hk (Hong Kong, China)",
	"time.ien.it (Torino, Italy)",
	"clock.tl.fukuoka-u.ac.jp (Fukuoka, Japan)",
	"cronos.cenam.mx (Queretaro, Mexico)",
	"ntp0.nl.net (Amsterdam, The Netherlands)",
	"ntp1.nl.net (Amsterdam, The Netherlands)",
	"ntp2.nl.net (Amsterdam, The Netherlands)",
	"time.service.uit.no (Norway)",
	"ntp.certum.pl (Poland)",
	"vega.cbk.poznan.pl (Borowiec, Poland)",
	"time1.stupi.se (Stockholm, Sweden)",
	"goodtime.ijs.si (Ljubljana, Slovenia)",
	"ntp2.ja.net (UK)",
	NULL
};

static GstDialogSignal signals[] = {
	/*	{ "calendar",          "day_selected",       G_CALLBACK (gst_time_calendar_change_cb) },
		{ "calendar",          "month_changed",      G_CALLBACK (gst_time_calendar_change_cb) },*/
	{ "timezone_button",         "clicked",         G_CALLBACK (timezone_button_clicked) },
	{ "ntp_use",                 "toggled",         G_CALLBACK (ntp_use_toggled) },
	{ "timeserver_button",       "clicked",         G_CALLBACK (server_button_clicked) },
	{ "location_combo",          "set_focus_child", G_CALLBACK (gst_dialog_modify_cb) },
	{ "ntp_add_server",          "clicked",         G_CALLBACK (on_ntp_addserver) },
	{ "ntp_add_server",          "clicked",         G_CALLBACK (gst_dialog_modify_cb) },
	{ "time_zone_help_button",   "clicked",         G_CALLBACK (on_timezone_help_button_clicked) },
	{ "time_server_help_button", "clicked",         G_CALLBACK (on_time_server_help_button_clicked) },
	{ NULL }
};

static void
gst_time_populate_ntp_list (GstTimeTool *time_tool)
{
	GstTool *tool = GST_TOOL (time_tool);
	GtkWidget *ntp_list, *item;
	GList *list_add = 0;
	GtkListStore *store;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GtkTreeIter iter;
	int i;


	/* ntp_list is a GtkTreeView */
	ntp_list = gst_dialog_get_widget (tool->main_dialog, "ntp_list2");

	/* set the model */
	store = gtk_list_store_new (2, G_TYPE_BOOLEAN, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (ntp_list),
				 GTK_TREE_MODEL (store));

	/* create the first column, it contains 2 cell renderers */
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
	
	for (i = 0; ntp_servers[i]; i++) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, FALSE, 1, ntp_servers[i], -1);
	}
}


static void
gst_time_init_timezone (GstTimeTool *time_tool)
{
	GstTool *tool = GST_TOOL (time_tool);
	GtkWidget *w;
	GPtrArray *locs;
	GList *combo_locs = NULL;
	int i;

	tzmap = e_tz_map_new (time_tool);
	g_return_if_fail (tzmap != NULL);
	
	w = gst_dialog_get_widget (tool->main_dialog, "map_window");
	gtk_container_add (GTK_CONTAINER (w), GTK_WIDGET (tzmap->map));
	gtk_widget_show (GTK_WIDGET (tzmap->map));

	w = gst_dialog_get_widget (tool->main_dialog, "location_combo");
	locs = tz_get_locations (e_tz_map_get_tz_db (tzmap));

	for (i = 0; g_ptr_array_index (locs, i); i++)
		gtk_combo_box_append_text (GTK_COMBO_BOX (w), tz_location_get_zone (g_ptr_array_index (locs, i)));
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

void
gst_time_update (GstTimeTool *tool)
{
	gchar *s, *m, *h;
	
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool->seconds), (gfloat) tool->sec);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool->minutes), (gfloat) tool->min);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool->hours), (gfloat) tool->hrs);
	
	/* we add the 0 if the number is <= 9, it's more pretty */
	s = g_strdup_printf ("%02d", tool->sec);
	m = g_strdup_printf ("%02d", tool->min);
	h = g_strdup_printf ("%02d", tool->hrs);
	
	gtk_entry_set_text (GTK_ENTRY (tool->seconds), s);
	gtk_entry_set_text (GTK_ENTRY (tool->minutes), m);
	gtk_entry_set_text (GTK_ENTRY (tool->hours), h);

	g_free (s);
	g_free (m);
	g_free (h);
}

static gboolean
gst_time_clock_tick (gpointer time_tool)
{
	GstTimeTool *tool = GST_TIME_TOOL (time_tool);
	GstTool *gst_tool = GST_TOOL (time_tool);
	struct tm *tm;
	time_t tt;

	tool->ticking = TRUE;
	
	gst_dialog_freeze (gst_tool->main_dialog);

	tt = time (NULL);
	tm = localtime (&tt);

	gst_time_set_full (time_tool, tm);
	gst_dialog_thaw (gst_tool->main_dialog);
	tool->ticking = FALSE;

	return TRUE;
}

static void
on_server_list_element_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
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
	
	if (toggle == TRUE) {
		/* Toggle is set to true, we have to delete entry in XML and set toggle to FALSE */
		toggle = FALSE;
		for (node = gst_xml_element_find_first (node, "server"); node != NULL; node = gst_xml_element_find_next (node, "server")) {
			if (strcmp (server, gst_xml_element_get_content (node)) == 0) 
				gst_xml_element_destroy (node);
		}
	} else {
		/* Toggle is set to false, we have to add a server entry in the XML and set toggle to TRUE */
		toggle = TRUE;
		node = gst_xml_element_add (node, "server");
				gst_xml_element_set_content (node, server);
	}

	gtk_list_store_set (store, &iter, 0, toggle, -1);
	gtk_tree_path_free (path);
	gst_dialog_modify (tool->main_dialog);
}

static void
on_timezone_help_button_clicked (GtkWidget *w, gpointer data)
{
	GstDialog *dialog = GST_DIALOG (data);
	GstTool *tool = gst_dialog_get_tool (dialog);

	gst_tool_show_help (tool, "tool-time-zone");
}

static void
on_time_server_help_button_clicked (GtkWidget *w, gpointer data)
{
	GstDialog *dialog = GST_DIALOG (data);
	GstTool *tool = gst_dialog_get_tool (dialog);

	gst_tool_show_help (tool, "tool-time-servers");
}

static void
timezone_button_clicked (GtkWidget *w, gpointer data)
{
	GtkWidget *d;
	GstDialog *dialog;
	GstTimeTool *time_tool;
	gchar *tz_name = NULL;
	gchar *old_tz_name = NULL;
	TzLocation *tz_location;
	gint correction;

	dialog = GST_DIALOG (data);
	time_tool = GST_TIME_TOOL (gst_dialog_get_tool (dialog));

	d = gst_dialog_get_widget (dialog, "time_zone_window");

	if (time_tool->time_zone_name)
		e_tz_map_set_tz_from_name (tzmap, time_tool->time_zone_name);

	gtk_window_set_transient_for (GTK_WINDOW (d), GTK_WINDOW (dialog));

	while (gtk_dialog_run (GTK_DIALOG (d)) == GTK_RESPONSE_HELP);

	tz_name     = e_tz_map_get_selected_tz_name (tzmap);
	tz_location = e_tz_map_get_location_by_name (tzmap, tz_name);

	old_tz_name = gst_time_tool_get_time_zone_name (time_tool);

	if ((!old_tz_name) || ((old_tz_name) && (strcmp (tz_name, old_tz_name) != 0))) {
		correction = tz_location_set_locally (tz_location);
		gst_time_tool_set_time_zone_name (time_tool, tz_name);
		gst_time_set_from_localtime (time_tool, correction);
		gst_dialog_modify (dialog);
	}

	gtk_widget_hide (d);
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
update_tz (GstTimeTool *time_tool)
{
	GtkWidget *l;

	l = gst_dialog_get_widget (GST_DIALOG (GST_TOOL (time_tool)->main_dialog), "tzlabel");

	if (time_tool->time_zone_name) {
		gtk_label_set_text (GTK_LABEL (l), time_tool->time_zone_name);
	}
}      

void
gst_time_tool_set_time_zone_name (GstTimeTool *time_tool, gchar *name)
{
	if (time_tool->time_zone_name) {
		g_free (time_tool->time_zone_name);
	}
	time_tool->time_zone_name = g_strdup (name);
	update_tz (time_tool);
}

gchar*
gst_time_tool_get_time_zone_name (GstTimeTool *time_tool)
{
	return time_tool->time_zone_name;
}

static void
ntp_use_toggled (GtkWidget *w, GstDialog *dialog)
{
	gboolean active, configured, ntp_installed;

	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
	configured = (gboolean) g_object_get_data (G_OBJECT (dialog->tool), "tool_configured");
	ntp_installed = (gboolean) g_object_get_data (G_OBJECT (dialog->tool), "ntpinstalled");
	
	if (configured && !ntp_installed && active) {
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

	if (ntp_installed)
		gst_dialog_modify (dialog);
	
	gtk_widget_set_sensitive (gst_dialog_get_widget (dialog, "timeserver_button"), active);
}

GST_TOOL_MAKE_TYPE(time,Time)

static void
gst_time_tool_type_init (GstTimeTool *tool)
{
	tool->running = FALSE;
	tool->ticking = FALSE;
}

static GstTool *
gst_time_tool_new (void)
{
	return GST_TOOL (g_type_create_instance (GST_TYPE_TIME_TOOL));
}

static void
gst_time_calendar_change_cb (GtkCalendar *calendar, gpointer data)
{
	GstTimeTool *tool = (GstTimeTool *)data;

	gst_time_clock_stop (tool);
	gst_dialog_modify (GST_TOOL (tool)->main_dialog);
}

static void 
gst_time_key_press_event_cb (GtkWidget *widget, GdkEventKey *event, GstTimeTool *tool)
{
	if (tool->ticking)
		return;

	if ((event->keyval == GDK_Return) ||
	    (event->keyval == GDK_Up) ||
	    (event->keyval == GDK_Down) ||
	    (event->keyval == GDK_Page_Up) ||
	    (event->keyval == GDK_Page_Down))
		g_signal_stop_emission_by_name (GTK_OBJECT (widget),
					        "key_press_event");

	
	gst_time_clock_stop (tool);
	gst_dialog_modify (GST_TOOL (tool)->main_dialog);
}

static void
gst_time_filter (GtkEntry *entry, const gchar *new_text,
		 gint length, gint *pos, GstTimeTool *tool)
{
	GtkWidget *widget = GTK_WIDGET (entry);
	const gchar *text;
	gchar new_val_string [4];
	gint new_val;
	gint max = (widget == tool->hours) ? 24 : 60;

	if (tool->ticking)
		return;
	if (length > 1)
		return;
	
	gst_time_clock_stop (tool);
	text = gtk_entry_get_text (entry);

	/*
	 * Get the resulting string after this insert text event
	 * and its numberical value
	 */
	if ((*pos == 0) && (strlen (text) > 0))
		snprintf (new_val_string, 4, "%c%s%c", *new_text, text, 0);
	else
		snprintf (new_val_string, 4, "%s%c%c", text, *new_text, 0);
	new_val = atoi (new_val_string);

	/* Stop this insert text event if : */
	if ((!isdigit(*new_text)) ||
	    (strlen (new_val_string) > 2) ||
	    (new_val >= GPOINTER_TO_INT (max))) {
		gdk_beep ();
		g_signal_stop_emission_by_name (GTK_OBJECT (entry), "insert_text");
		return;
	}

	if (widget == tool->hours) {
		tool->hrs = new_val;
	} else if (widget == tool->minutes) {
		tool->min = new_val;
	} else if (widget == tool->seconds) {
		tool->sec = new_val;
	}

	gst_dialog_modify (GST_TOOL (tool)->main_dialog);
}

static void
gst_time_focus_in (GtkWidget *widget, GdkEventFocus *event, GstTimeTool *tool)
{
	gst_time_clock_stop (tool);
}

static void
gst_time_focus_out (GtkWidget *widget, GdkEventFocus *event, GstTimeTool *tool)
{
	gchar *val = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
	gint   num = atoi (val);
	gchar *value;

	if (widget == tool->seconds) {
		tool->sec = num;
	} else if (widget == tool->minutes) {
		tool->min = num;
	} else if (widget == tool->hours) {
		tool->hrs = num;
	}

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), (gfloat) num);

	value = g_strdup_printf ("%02d", num);
	gtk_entry_set_text (GTK_ENTRY (widget), value);

	g_free (value);
	g_free (val);
}

static void
gst_time_change (GtkSpinButton *widget, gpointer data)
{
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
	val = g_strdup_printf ("%02d", value);
	gtk_entry_set_text (GTK_ENTRY (widget), val);
	g_free (val);
}

static GtkWidget *
gst_time_spin_button_create (GstTimeTool *tool, gchar *label)
{
	GtkWidget *spin = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, label);
	
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), 0);
	g_signal_connect (G_OBJECT (spin), "insert_text",
			  G_CALLBACK (gst_time_filter),
			  tool);
	/* g_signal_connect (G_OBJECT (spin), "key_press_event",
			  G_CALLBACK (gst_time_key_press_event_cb),
			  tool);*/
	g_signal_connect_after (G_OBJECT (spin), "focus_in_event",
			        G_CALLBACK (gst_time_focus_in),
			        tool);
	g_signal_connect_after (G_OBJECT (spin), "focus_out_event",
			        G_CALLBACK (gst_time_focus_out),
			        tool);
	g_signal_connect (G_OBJECT (spin), "value-changed",
			  G_CALLBACK (gst_time_change), 
			  tool);
	return spin;
}

static void
gst_time_load_widgets (GstTimeTool *tool)
{
	GstDialog *dialog = GST_TOOL (tool)->main_dialog;
	
	tool->seconds = gst_time_spin_button_create (tool, "seconds");
	tool->minutes = gst_time_spin_button_create (tool, "minutes");
	tool->hours   = gst_time_spin_button_create (tool, "hours");
	tool->map_hover_label = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "location_label");
}

void
gst_time_clock_stop (GstTimeTool *tool)
{
	if (!tool->running || tool->ticking)
		return;

	g_return_if_fail (tool->timeout != -1);
	
	gtk_timeout_remove (tool->timeout);
	tool->timeout = -1;
	tool->running = FALSE;
}


void
gst_time_clock_start (GstTimeTool *tool)
{
	if (tool->running)
		return;

	tool->timeout = gtk_timeout_add (1000, gst_time_clock_tick,
					 GST_TIME_TOOL (tool));
	tool->running = TRUE;
}

void
gst_time_set_full (GstTimeTool *time_tool, struct tm *tm)
{
	GtkWidget *calendar_widget;
	guint day, month, year;

	calendar_widget = gst_dialog_get_widget (GST_TOOL (time_tool)->main_dialog, "calendar");
	
	gtk_calendar_get_date (GTK_CALENDAR (calendar_widget), &year, &month, &day);
	year -= 1900;

	if ( (tm->tm_year != year) || (tm->tm_mon != month))
		gtk_calendar_select_month (GTK_CALENDAR (calendar_widget), tm->tm_mon, tm->tm_year + 1900);
	if (tm->tm_mday != day)
		gtk_calendar_select_day   (GTK_CALENDAR (calendar_widget), tm->tm_mday);

	time_tool->hrs = tm->tm_hour;
	time_tool->min = tm->tm_min;
	time_tool->sec = tm->tm_sec;

	gst_time_update (time_tool);
}

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

void
gst_time_connect_calendar_signals (GstTimeTool *tool)
{
	GtkWidget *calendar = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "calendar");

	g_signal_connect (G_OBJECT (calendar), "day_selected", G_CALLBACK (gst_time_calendar_change_cb), tool);
	g_signal_connect (G_OBJECT (calendar), "month_changed", G_CALLBACK (gst_time_calendar_change_cb), tool);
}

int
main (int argc, char *argv[])
{
	GstTool *tool;

	gst_init ("time-admin", argc, argv, NULL);
	tool = gst_time_tool_new ();
	gst_tool_construct (tool, "time", _("Time and Date Settings"));

	gst_tool_set_xml_funcs (tool, transfer_xml_to_gui, transfer_gui_to_xml, NULL);
	gst_dialog_connect_signals (tool->main_dialog, signals);
	gst_time_connect_calendar_signals (GST_TIME_TOOL (tool));

	gst_time_load_widgets (GST_TIME_TOOL (tool));
	gst_time_populate_ntp_list (GST_TIME_TOOL (tool));
	gst_time_init_timezone (GST_TIME_TOOL (tool));
	gst_tool_main (tool, TRUE);

	gst_time_clock_start (GST_TIME_TOOL (tool));
	gtk_main ();

	return 0;
}
