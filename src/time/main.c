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

#include "xst.h"
#include "time-tool.h"

#include "tz.h"
#include "timeserv.h"
#include "transfer.h"
#include "e-map/e-map.h"
#include "tz-map.h"

ETzMap *tzmap;

static void timezone_button_clicked (GtkWidget *w, gpointer data);
static void update_tz (XstTimeTool *time_tool);
static void server_button_clicked (GtkWidget *w, gpointer data);
static void ntp_use_toggled (GtkWidget *w, XstDialog *dialog);

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
	"ntp.cs.mu.oz.au (Melbourne, Austrilia)",
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

static XstDialogSignal signals[] = {
	{ "calendar",          "day_selected",       G_CALLBACK (xst_dialog_modify_cb) },
	{ "calendar",          "month_changed",      G_CALLBACK (xst_dialog_modify_cb) },
	{ "timezone_button",   "clicked",            G_CALLBACK (timezone_button_clicked) },
	{ "ntp_use",           "toggled",            G_CALLBACK (ntp_use_toggled) },
	{ "timeserver_button", "clicked",            G_CALLBACK (server_button_clicked) },
	{ "location_combo",    "set_focus_child",    G_CALLBACK (xst_dialog_modify_cb) },
#warning FIXME
#if 0
	{ "tz_combo_entry",    "changed",            G_CALLBACK (update_tz) },
#endif
        /* Changed the Signal for the GtkTreeView --AleX
	  { "ntp_list",          "selection_changed",  xst_dialog_modify_cb },*/
	{ "ntp_list2",         "cursor_changed",     G_CALLBACK (xst_dialog_modify_cb) },
	{ "ntp_add_server",    "clicked",            G_CALLBACK (on_ntp_addserver) },
	{ "ntp_add_server",    "clicked",            G_CALLBACK (xst_dialog_modify_cb) },
	{ NULL }
};

static void
xst_time_populate_ntp_list (XstTimeTool *time_tool)
{
	XstTool *tool = XST_TOOL (time_tool);
	GtkWidget *ntp_list, *item;
	GList *list_add = 0;
	GtkListStore *store;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GtkTreeIter iter;
	int i;


	/* ntp_list is a GtkTreeView */
	ntp_list = xst_dialog_get_widget (tool->main_dialog, "ntp_list2");


	/* set the model */
	store = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (ntp_list),
				 GTK_TREE_MODEL (store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (ntp_list), TRUE);


	/* create the first column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Servers", cell,
							   "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (ntp_list), column);
	
	for (i = 0; ntp_servers[i]; i++) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, ntp_servers[i], -1);
	}
}


static void
xst_time_init_timezone (XstTimeTool *time_tool)
{
	XstTool *tool = XST_TOOL (time_tool);
	GtkWidget *w;
	GPtrArray *locs;
	GList *combo_locs = NULL;
	int i;	

	tzmap = e_tz_map_new (time_tool);
	g_return_if_fail (tzmap != NULL);
	
	w = xst_dialog_get_widget (tool->main_dialog, "map_window");
	gtk_container_add (GTK_CONTAINER (w), GTK_WIDGET (tzmap->map));
	gtk_widget_show (GTK_WIDGET (tzmap->map));
	
	locs = tz_get_locations (e_tz_map_get_tz_db (tzmap));
	
	for (i = 0; g_ptr_array_index (locs, i); i++) {
		combo_locs = g_list_append (combo_locs,
					    g_strdup (tz_location_get_zone (g_ptr_array_index (locs, i))));
	}

	w = xst_dialog_get_widget (tool->main_dialog, "location_combo");
	gtk_combo_set_popdown_strings (GTK_COMBO (w), combo_locs);
}

#define is_leap_year(yyy) ((((yyy % 4) == 0) && ((yyy % 100) != 0)) || ((yyy % 400) == 0));

static void
xst_time_update_date (XstTimeTool *tool, gint add)
{
	static const gint month_length[2][13] =
	{
		{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
		{ 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
	};
	GtkWidget *calendar;
	gint day, month, year;
	gint days_in_month;
	gboolean leap_year;

	calendar = xst_dialog_get_widget (XST_TOOL (tool)->main_dialog,
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
xst_time_update (XstTimeTool *tool)
{
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool->seconds), (gfloat) tool->sec);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool->minutes), (gfloat) tool->min);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool->hours), (gfloat) tool->hrs);
	
	/* we add the 0 if the number is <= 9, it's more pretty */
	gtk_entry_set_text (GTK_ENTRY (tool->seconds), g_strdup_printf ("%02d", tool->sec));
	gtk_entry_set_text (GTK_ENTRY (tool->minutes), g_strdup_printf ("%02d", tool->min));
	gtk_entry_set_text (GTK_ENTRY (tool->hours), g_strdup_printf ("%02d", tool->hrs));
}

static gboolean
xst_time_clock_tick (gpointer time_tool)
{
	XstTimeTool *tool = XST_TIME_TOOL (time_tool);
	XstTool *xst_tool = XST_TOOL (time_tool);
	struct tm *tm;
	time_t tt;

	tool->ticking = TRUE;
	
	xst_dialog_freeze (xst_tool->main_dialog);

	tt = time (NULL);
	tm = localtime (&tt);

	xst_time_set_full (time_tool, tm);
	xst_dialog_thaw (xst_tool->main_dialog);
	tool->ticking = FALSE;

	return TRUE;
}

static GtkWidget *
timezone_construct_dialog (XstDialog *dialog)
{
	GtkWidget *content;
	GtkWidget *d;

        /* Added to test arguments  --AleX */
	g_return_if_fail (dialog!=NULL);
	g_return_if_fail (XST_IS_DIALOG(dialog));
	
	d = gtk_dialog_new_with_buttons (_("GNOME System Tools - Timezone"),
					      NULL,
					      GTK_DIALOG_MODAL,
					      GTK_STOCK_APPLY,
					      GTK_RESPONSE_APPLY,
					      GTK_STOCK_CLOSE,
					      GTK_RESPONSE_CLOSE, NULL);

	gtk_widget_set_usize (GTK_WIDGET (d), 320, 320);

	content = xst_dialog_get_widget (dialog, "time_zone_dialog_content");

	/* FIXME: Yes, this is a hack. */
	content->parent = NULL;

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (d)->vbox), content, TRUE,
			    TRUE, 8);

	return GTK_WIDGET (d);
}

static void
timezone_button_clicked (GtkWidget *w, gpointer data)
{
	static GtkWidget *d = NULL;
	XstDialog *dialog;
	XstTimeTool *time_tool;
	gint result;

	dialog = XST_DIALOG (data);
	time_tool = XST_TIME_TOOL (xst_dialog_get_tool (dialog));

	if (!d) {
		d = timezone_construct_dialog (dialog);
	}

	if (time_tool->time_zone_name) {
		e_tz_map_set_tz_from_name (tzmap, time_tool->time_zone_name);
	}

	gtk_widget_show (d);

	result = gtk_dialog_run (GTK_DIALOG (d));
	if (result == GTK_RESPONSE_APPLY) {
		gchar *tz_name;
		TzLocation *tz_location;
		gint correction;

		tz_name     = e_tz_map_get_selected_tz_name (tzmap);
		tz_location = e_tz_map_get_location_by_name (tzmap, tz_name);

		correction = tz_location_set_locally (tz_location);
		xst_time_tool_set_time_zone_name (time_tool, tz_name);
		xst_time_set_from_localtime (time_tool, correction);
		xst_dialog_modify (dialog);
	}

	gtk_widget_hide (d);
}


/* Function Added to construct Time Server Dialog using GtkDialog --AleX */
static GtkWidget *
server_construct_dialog (XstDialog *dialog)
{
	GtkWidget *content;
	GtkWidget *d;

	/* Added to test arguments --AleX */
	g_return_if_fail (dialog!=NULL);
	g_return_if_fail (XST_IS_DIALOG(dialog));

	d = gtk_dialog_new_with_buttons (_("GNOME System Tools - Time Servers"),
					      NULL,
					      GTK_DIALOG_MODAL,
					      GTK_STOCK_CLOSE,
					      GTK_RESPONSE_CLOSE, NULL);

	gtk_widget_set_usize (GTK_WIDGET (d), 550, 400);

	content = xst_dialog_get_widget (dialog, "server_dialog_content");

	/* FIXME: Yes, this is a hack. */
	content->parent = NULL;

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (d)->vbox), content, TRUE,
			    TRUE, 8);

	return GTK_WIDGET (d);
}

static void
server_button_clicked (GtkWidget *w, gpointer data)
{
	static GtkWidget *d = NULL;
	XstDialog *dialog;

	dialog = XST_DIALOG (data);
	
	if (!d) 
	      d = server_construct_dialog (dialog);

	gtk_dialog_run (GTK_DIALOG (d));
	gtk_widget_hide (d);
}

static void
update_tz (XstTimeTool *time_tool)
{
	GtkWidget *l;

	l = xst_dialog_get_widget (XST_DIALOG (XST_TOOL (time_tool)->main_dialog), "tzlabel");

	if (time_tool->time_zone_name) {
		gtk_label_set_text (GTK_LABEL (l), time_tool->time_zone_name);
	}
}      

void
xst_time_tool_set_time_zone_name (XstTimeTool *time_tool, gchar *name)
{
	if (time_tool->time_zone_name) {
		g_free (time_tool->time_zone_name);
	}
	time_tool->time_zone_name = g_strdup (name);
	update_tz (time_tool);
}

static void
ntp_use_toggled (GtkWidget *w, XstDialog *dialog)
{
	gboolean active, configured, ntp_installed;

	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
	configured = (gboolean) g_object_get_data (G_OBJECT (dialog->tool), "tool_configured");
	ntp_installed = (gboolean) g_object_get_data (G_OBJECT (dialog->tool), "ntpinstalled");
	
	if (configured && !ntp_installed && active) {
		GtkWidget *dialog;
		
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), FALSE);
		dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, _("You don't have NTP support installed. Please install NTP support\nin the system to enable server synchronization."));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}

	if (ntp_installed)
		xst_dialog_modify (dialog);
	
	gtk_widget_set_sensitive (xst_dialog_get_widget (dialog, "timeserver_button"), active);
}

XST_TOOL_MAKE_TYPE(time,Time)

static void
xst_time_tool_type_init (XstTimeTool *tool)
{
	tool->running = FALSE;
	tool->ticking = FALSE;
}

static XstTool *
xst_time_tool_new (void)
{
	return XST_TOOL (g_type_create_instance (XST_TYPE_TIME_TOOL));
}

static void 
xst_time_key_press_event_cb (GtkWidget *widget, GdkEventKey *event, XstTimeTool *tool)
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

	
	xst_time_clock_stop (tool);
	xst_dialog_modify (XST_TOOL (tool)->main_dialog);
}

static void
xst_time_filter (GtkEntry *entry, const gchar *new_text,
		 gint length, gint *pos, XstTimeTool *tool)
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
	
	xst_time_clock_stop (tool);
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

	xst_dialog_modify (XST_TOOL (tool)->main_dialog);
}

static void
xst_time_focus_out (GtkWidget *widget, GdkEventFocus *event, XstTimeTool *tool)
{
	gint num = atoi (gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1));

	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "focus_out_event");

	if (widget == tool->seconds) {
		tool->sec = num;
	} else if (widget == tool->minutes) {
		tool->min = num;
	} else if (widget == tool->hours) {
		tool->hrs = num;
	}

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), (gfloat) num);
	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "focus_out_event");
}

static void
xst_time_change (GtkSpinButton *widget, gpointer data)
{
	XstTimeTool *tool = data;
	gint value = gtk_spin_button_get_value (widget);

	g_return_if_fail (GTK_IS_SPIN_BUTTON (widget));
	g_return_if_fail (XST_IS_TIME_TOOL (tool));

	if (widget == GTK_SPIN_BUTTON (tool->seconds)) {
		if (value > 59) {
			gtk_spin_button_set_value (widget, value - 60);
			gtk_spin_button_spin (GTK_SPIN_BUTTON (tool->minutes), GTK_SPIN_STEP_FORWARD, 1);
		} else if (value < 0) {
			gtk_spin_button_set_value (widget, value + 60);
			gtk_spin_button_spin (GTK_SPIN_BUTTON (tool->minutes), GTK_SPIN_STEP_BACKWARD, 1);
		}
	} else if (widget == GTK_SPIN_BUTTON (tool->minutes)) {
		if (value > 59) {
			gtk_spin_button_set_value (widget, value - 60);
			gtk_spin_button_spin (GTK_SPIN_BUTTON (tool->hours), GTK_SPIN_STEP_FORWARD, 1);
		} else if (value < 0) {
			gtk_spin_button_set_value (widget, value + 60);
			gtk_spin_button_spin (GTK_SPIN_BUTTON (tool->hours), GTK_SPIN_STEP_BACKWARD, 1);
		}
	} else if (widget == GTK_SPIN_BUTTON (tool->hours)) {
		if (value > 23) {
			gtk_spin_button_set_value (widget, value - 24);
			xst_time_update_date (tool, +1);
		} else if (value < 0) {
			xst_time_update_date (tool, -1);
			gtk_spin_button_set_value (widget, value + 24);
		}
	}

	xst_dialog_modify (XST_TOOL (tool)->main_dialog);
	xst_time_clock_stop (tool);
	
	/* We have to set it to 01 instead of 1, it's more pretty */
	gtk_entry_set_text (GTK_ENTRY (widget), g_strdup_printf ("%02d", value));
}

static GtkWidget *
xst_time_spin_button_create (XstTimeTool *tool, gchar *label)
{
	GtkWidget *spin = xst_dialog_get_widget (XST_TOOL (tool)->main_dialog, label);
	
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), 0);
	g_signal_connect (G_OBJECT (spin), "insert_text",
			  G_CALLBACK (xst_time_filter),
			  tool);
	g_signal_connect (G_OBJECT (spin), "key_press_event",
			  G_CALLBACK (xst_time_key_press_event_cb),
			  tool);
	g_signal_connect_after (G_OBJECT (spin), "focus_out_event",
			        G_CALLBACK (xst_time_focus_out),
			        tool);
	g_signal_connect (G_OBJECT (spin), "value-changed",
			  G_CALLBACK (xst_time_change), 
			  tool);
	return spin;
}

static void
xst_time_load_widgets (XstTimeTool *tool)
{
	XstDialog *dialog = XST_TOOL (tool)->main_dialog;
	
	tool->seconds = xst_time_spin_button_create (tool, "seconds");
	tool->minutes = xst_time_spin_button_create (tool, "minutes");
	tool->hours   = xst_time_spin_button_create (tool, "hours");
	tool->map_hover_label = xst_dialog_get_widget (XST_TOOL (tool)->main_dialog, "location_label");
}

void
xst_time_clock_stop (XstTimeTool *tool)
{
	if (!tool->running || tool->ticking)
		return;

	g_return_if_fail (tool->timeout != -1);
	
	gtk_timeout_remove (tool->timeout);
	tool->timeout = -1;
	tool->running = FALSE;
}


void
xst_time_clock_start (XstTimeTool *tool)
{
	if (tool->running)
		return;

	tool->timeout = gtk_timeout_add (1000, xst_time_clock_tick,
					 XST_TIME_TOOL (tool));
	tool->running = TRUE;
}

void
xst_time_set_full (XstTimeTool *time_tool, struct tm *tm)
{
	GtkWidget *calendar_widget;
	gint day, month, year;

	calendar_widget = xst_dialog_get_widget (XST_TOOL (time_tool)->main_dialog, "calendar");
	
	gtk_calendar_get_date (GTK_CALENDAR (calendar_widget), &year, &month, &day);
	year -= 1900;

	if ( (tm->tm_year != year) || (tm->tm_mon != month))
		gtk_calendar_select_month (GTK_CALENDAR (calendar_widget), tm->tm_mon, tm->tm_year + 1900);
	if (tm->tm_mday != day)
		gtk_calendar_select_day   (GTK_CALENDAR (calendar_widget), tm->tm_mday);

	time_tool->hrs = tm->tm_hour;
	time_tool->min = tm->tm_min;
	time_tool->sec = tm->tm_sec;

	xst_time_update (time_tool);
}

void
xst_time_set_from_localtime (XstTimeTool *time_tool, gint correction)
{
	struct tm *tm;
	time_t tt;

	tt = time (NULL);
	tt += correction; 
	tm = localtime (&tt);

	xst_time_set_full (time_tool, tm);
}

int
main (int argc, char *argv[])
{
	XstTool *tool;

	xst_init ("time-admin", argc, argv, NULL);
	tool = xst_time_tool_new ();
	xst_tool_construct (tool, "time", _("Time and Date Settings"));

	xst_tool_set_xml_funcs (tool, transfer_xml_to_gui, transfer_gui_to_xml, NULL);
	xst_dialog_connect_signals (tool->main_dialog, signals);

	xst_time_load_widgets (XST_TIME_TOOL (tool));
	xst_time_populate_ntp_list (XST_TIME_TOOL (tool));
	xst_time_init_timezone (XST_TIME_TOOL (tool));
	xst_tool_main (tool, TRUE);

	xst_time_clock_start (XST_TIME_TOOL (tool));
	gtk_main ();

	return 0;
}
