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
 */

#include <config.h>

#include <stdio.h>
#include <ctype.h>

#include <gnome.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "xst.h"
#include "time-tool.h"
#include "xst-spin-button.h"

#include "tz.h"
#include "timeserv.h"
#include "transfer.h"
#include "e-map/e-map.h"
#include "tz-map.h"

ETzMap *tzmap;

static void timezone_button_clicked (GtkWidget *w, gpointer data);
static void update_tz (GtkWidget *w, gpointer data);
static void server_button_clicked (GtkWidget *w, gpointer data);
static void ntp_use_toggled (GtkWidget *w, XstDialog *dialog);
#if 0
static void xst_time_modify_cb (GtkWidget *w, XstDialog *dialog);
#endif

static char *ntp_servers[] =
{
	"time.nrc.ca (Canada)",
	"ntp1.cmc.ec.gc.ca (Eastern Canada)",
	"ntp2.cmc.ec.gc.ca (Eastern Canada)",
	"clock.tricity.wsu.edu (Washington, USA)",
	"wuarchive.wustl.edu (Washington, USA)",
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
	0
};

static XstDialogSignal signals[] = {
	{ "calendar",          "day_selected",       xst_dialog_modify_cb },
	{ "calendar",          "month_changed",      xst_dialog_modify_cb },
#if 0
	{ "hour",              "changed",            xst_time_modify_cb },
	{ "minute",            "changed",            xst_time_modify_cb },
	{ "second",            "changed",            xst_time_modify_cb },
#endif	

	{ "timezone_button",   "clicked",            timezone_button_clicked },
	{ "ntp_use",           "toggled",            ntp_use_toggled },
	{ "timeserver_button", "clicked",            server_button_clicked },
	{ "location_combo",    "set_focus_child",    xst_dialog_modify_cb },
	{ "tz_combo_entry",    "changed",            update_tz },
	{ "ntp_list",          "selection_changed",  xst_dialog_modify_cb },
	{ "ntp_add_server",    "clicked",            on_ntp_addserver },
	{ "ntp_add_server",    "clicked",            xst_dialog_modify_cb },
	{ NULL }
};

#if 0
static void
xst_time_modify_cb (GtkWidget *w, XstDialog *dialog)
{
	if (dialog->frozen > 0) 
		return;
 
	xst_time_clock_stop (XST_TIME_TOOL (dialog->tool));
	xst_dialog_modify (dialog);
}
#endif

static void
xst_time_populate_ntp_list (XstTimeTool *time_tool)
{
	XstTool *tool = XST_TOOL (time_tool);
	GtkWidget *ntp_list, *item;
	GList *list_add = 0;
	int i;

	ntp_list = xst_dialog_get_widget (tool->main_dialog, "ntp_list");
	if (!ntp_list) return;  /* Broken interface file */
	
	for (i = 0; ntp_servers[i]; i++) {
		item = gtk_list_item_new_with_label (ntp_servers[i]);
		gtk_widget_show (item);
		list_add = g_list_append (list_add, item);
	}
	
	gtk_list_append_items (GTK_LIST (ntp_list), list_add);
}


static void
xst_time_init_timezone (XstTimeTool *time_tool)
{
	XstTool *tool = XST_TOOL (time_tool);
	GtkWidget *w, *w2;
	GPtrArray *locs;
	GList *combo_locs = NULL;
	int i;	

	tzmap = e_tz_map_new (time_tool);
	w = xst_dialog_get_widget (tool->main_dialog, "map_window");
	gtk_container_add (GTK_CONTAINER (w), GTK_WIDGET (tzmap->map));
	gtk_widget_show (GTK_WIDGET (tzmap->map));
	
	locs = tz_get_locations (e_tz_map_get_tz_db (tzmap));
	
	for (i = 0; i < locs->len; i++) {
		combo_locs = g_list_append (combo_locs,
					    g_strdup (tz_location_get_zone (g_ptr_array_index (locs, i))));
	}

	w = xst_dialog_get_widget (tool->main_dialog, "location_combo");
	gtk_combo_set_popdown_strings (GTK_COMBO (w), combo_locs);

	w = xst_dialog_get_widget (tool->main_dialog, "location_label_vbox");
	w2 = gtk_label_new ("");
	gtk_box_pack_end (GTK_BOX(w), w2, FALSE, FALSE, 0);
	gtk_widget_show (w2);
	/* gtk_misc_set_alignment (GTK_MISC (w2), 0.0, 0.5); */

	gtk_object_set_data (GTK_OBJECT (tool), "location_hover", w2);
}

static void
xst_time_update_date (XstTimeTool *tool, gint add)
{
	g_print ("Update the calendar ..\n");
}

static void
xst_time_update_hours (XstTimeTool *tool, gint add)
{
	gchar buf [3];

	if (add != 0) {
		tool->hrs += add;
		if (tool->hrs < 0) {
			tool->hrs += 24;
			xst_time_update_date (tool, -1);
		}
		if (tool->hrs > 23) {
			tool->hrs -= 24;
			xst_time_update_date (tool, 1);
		}
	}

	snprintf (buf, 3, "%2d", tool->hrs);
	gtk_entry_set_text (GTK_ENTRY (tool->hours), buf);
}

static void
xst_time_update_minutes (XstTimeTool *tool, gint add)
{
	gchar buf [3];

	if (add != 0) {
		tool->min += add;
		if (tool->min < 0) {
			tool->min += 60;
			xst_time_update_hours (tool, -1);
		}
		if (tool->min > 59) {
			tool->min -= 60;
			xst_time_update_hours (tool, 1);
		}
	}

	snprintf (buf, 3, "%02d", tool->min);
	gtk_entry_set_text (GTK_ENTRY (tool->minutes), buf);
}

static void
xst_time_update_seconds (XstTimeTool *tool, gint add)
{
	gchar buf [3];
#if 1	
	gint start_pos;
	gint end_pos;
#endif	

	if (add != 0) {
		tool->sec += add;
		if (tool->sec < 0) {
			tool->sec += 60;
			xst_time_update_minutes (tool, -1);
		}
		if (tool->sec > 59) {
			tool->sec -= 60;
			xst_time_update_minutes (tool, 1);
		}
	}

#if 1	
	start_pos = GTK_EDITABLE(tool->seconds)->selection_start_pos;
        end_pos   = GTK_EDITABLE(tool->seconds)->selection_end_pos;
#endif	
	
	snprintf (buf, 3, "%02d", tool->sec);
	gtk_entry_set_text (GTK_ENTRY (tool->seconds), buf);

#if 1
	if (GTK_WIDGET_HAS_FOCUS (tool->seconds))
	    gtk_editable_select_region (GTK_EDITABLE (tool->seconds),
					start_pos, end_pos);
#endif	
}

void
xst_time_update (XstTimeTool *tool)
{
	xst_time_update_seconds (tool, 0);
	xst_time_update_minutes (tool, 0);
	xst_time_update_hours   (tool, 0);
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

	if (tm->tm_sec == tool->sec)
		goto xst_time_clock_tick_no_more;
	tool->sec = tm->tm_sec;
	xst_time_update_seconds (tool, 0);

	if (tm->tm_min == tool->min)
		goto xst_time_clock_tick_no_more;
	tool->min = tm->tm_min;
	xst_time_update_minutes (tool, 0);

	if (tm->tm_hour == tool->hrs);
		goto xst_time_clock_tick_no_more;
	tool->hrs = tm->tm_hour;
	xst_time_update_hours (tool, 0);

xst_time_clock_tick_no_more:
	xst_dialog_thaw (xst_tool->main_dialog);
	tool->ticking = FALSE;
	
	return TRUE;
}

static void
timezone_button_clicked (GtkWidget *w, gpointer data)
{
	static GtkWidget *d = NULL;

	if (!d) {
		d = xst_dialog_get_widget (XST_DIALOG (data), "Time zone");
		gnome_dialog_close_hides (GNOME_DIALOG (d), TRUE);
	}

	gtk_widget_show (d);
	gdk_window_show (d->window);
	gdk_window_raise (d->window);

	gnome_dialog_run_and_close (GNOME_DIALOG (d));
}

static void
server_button_clicked (GtkWidget *w, gpointer data)
{
	static GtkWidget *d = NULL;

	if (!d) {
		d = xst_dialog_get_widget (XST_DIALOG (data), "Time servers");
		gnome_dialog_close_hides (GNOME_DIALOG (d), TRUE);
	}

	gtk_widget_show (d);
	gdk_window_show (d->window);
	gdk_window_raise (d->window);

	gnome_dialog_run_and_close (GNOME_DIALOG (d));
}

static void
update_tz (GtkWidget *w, gpointer data)
{
	GtkWidget *l;

	l = xst_dialog_get_widget (XST_DIALOG (data), "tzlabel");
	gtk_label_set_text (GTK_LABEL (l), gtk_entry_get_text (GTK_ENTRY (w)));
}      

static void
ntp_use_toggled (GtkWidget *w, XstDialog *dialog)
{
	gboolean active, configured, ntp_installed;

	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
	configured = (gboolean) gtk_object_get_data (GTK_OBJECT (dialog->tool), "tool_configured");
	ntp_installed = (gboolean) gtk_object_get_data (GTK_OBJECT (dialog->tool), "ntpinstalled");
	
	if (configured && !ntp_installed && active) {
		GtkWidget *dialog;
		
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), FALSE);
		dialog = gnome_ok_dialog (_("You don't have NTP support installed. Please install NTP support\nin the system to enable server synchronization."));
		gtk_window_set_title (GTK_WINDOW (dialog), _("NTP support missing."));
		gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
		return;
	}

	if (ntp_installed)
		xst_dialog_modify (dialog);
	
	gtk_widget_set_sensitive (xst_dialog_get_widget (dialog, "timeserver_button"), active);
}

#if 0
static void
xst_time_set_spin (XstDialog *dialog, GtkWidget *widget, gint max, gboolean pad)
{
	GtkSpinButton *spin;
	GtkObject *adj;

	spin = XST_SPIN_BUTTON (widget);
	g_return_if_fail (GTK_IS_SPIN_BUTTON (spin));

	adj = gtk_adjustment_new (0.0, 0.0, max - 1.0,
				  1.0, 5.0, 5.0);

	gtk_spin_button_set_adjustment (spin, GTK_ADJUSTMENT (adj));
	gtk_spin_button_set_wrap       (spin, TRUE);
	gtk_object_set_user_data       (GTK_OBJECT(spin), GINT_TO_POINTER (pad ? 2 : 0));
}
#endif

XST_TOOL_MAKE_TYPE(time,Time)

static void
xst_time_tool_type_init (XstTimeTool *tool)
{
	tool->running = FALSE;
	tool->ticking = FALSE;
	/* empty */
}

static XstTool *
xst_time_tool_new (void)
{
	return XST_TOOL (gtk_type_new (XST_TYPE_TIME_TOOL));
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
		gtk_signal_emit_stop_by_name (GTK_OBJECT (widget),
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

#if 0
	g_print ("Filter .. %d **%c**\n", length, *new_text);
#endif
	
	if (tool->ticking) {
		return;
	}

	if (length > 1)
		return;
	
	xst_time_clock_stop (tool);
	text = gtk_entry_get_text (entry);
	
	/* Join the string so that we know what the result will be */
	if ((*pos == 0) && (strlen (text) > 0))
		snprintf (new_val_string, 4, "%c%s%c", *new_text, text, 0);
	else
		snprintf (new_val_string, 4, "%s%c%c", text, *new_text, 0);
	new_val = atoi (new_val_string);

	if ((!isdigit(*new_text)) ||
	    (strlen (new_val_string) > 2) ||
	    (new_val >= GPOINTER_TO_INT (max))) {
		gdk_beep ();
		gtk_signal_emit_stop_by_name (GTK_OBJECT (entry), "insert_text");
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

#if 1
static void
xst_time_focus_out (GtkWidget *widget, GdkEventFocus *event, XstTimeTool *tool)
{
	gint num = atoi (gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1));

	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "focus_out_event");

#if 0
	g_print ("Focus out %i, *%s*\n", num, gtk_entry_get_text
		(GTK_ENTRY (widget)));
#endif	
		
	if (widget == tool->seconds) {
		tool->sec = num;
		xst_time_update_seconds (tool, 0);
	} else if (widget == tool->minutes) {
		tool->min = num;
		xst_time_update_minutes (tool, 0);
	} else if (widget == tool->hours) {
		tool->hrs = num;
		xst_time_update_hours (tool, 0);
	}

	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "focus_out_event");
}
#endif

static gint
xst_time_button_press_event_cb (GtkWidget *widget, GdkEventButton *event,
				XstTimeTool *tool)
{
	XstSpinButton *spin = XST_SPIN_BUTTON (widget);
	gboolean up;

	if (event->window != spin->panel)
		return FALSE;

	xst_dialog_modify (XST_TOOL (tool)->main_dialog);
	
	up = (event->y <= widget->requisition.height / 2);
	xst_time_clock_stop (tool);
#if 0	
	gtk_signal_emit_stop_by_name (GTK_OBJECT (spin), "button_press_event");
#endif	

	if (widget == tool->seconds)
		xst_time_update_seconds (tool, up ? 1 : -1);
	else if (widget == tool->minutes)
		xst_time_update_minutes (tool, up ? 1 : -1);
	else if (widget == tool->hours) 
		xst_time_update_hours   (tool, up ? 1 : -1);

	return TRUE;
}

static void
xst_time_load_widgets (XstTimeTool *tool)
{
	XstDialog *dialog = XST_TOOL (tool)->main_dialog;
	GtkWidget *hbox;
	GtkWidget *label;

	hbox = xst_dialog_get_widget (dialog, "clock_hbox");
	
	tool->seconds = xst_spin_button_new ();
	tool->minutes = xst_spin_button_new ();
	tool->hours   = xst_spin_button_new ();

	gtk_box_pack_start_defaults (GTK_BOX (hbox), tool->hours);
	label =	gtk_label_new (":");
	gtk_box_pack_start_defaults (GTK_BOX (hbox), label);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), tool->minutes);
	label =	gtk_label_new (":");
	gtk_box_pack_start_defaults (GTK_BOX (hbox), label);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), tool->seconds);
	gtk_widget_show_all (hbox);

	xst_spin_button_set_numeric (XST_SPIN_BUTTON (tool->seconds), FALSE);
	xst_spin_button_set_numeric (XST_SPIN_BUTTON (tool->minutes), FALSE);
	xst_spin_button_set_numeric (XST_SPIN_BUTTON (tool->hours),   FALSE);

	xst_spin_button_set_wrap (XST_SPIN_BUTTON (tool->seconds), TRUE);
	xst_spin_button_set_wrap (XST_SPIN_BUTTON (tool->minutes), TRUE);
	xst_spin_button_set_wrap (XST_SPIN_BUTTON (tool->hours),   TRUE);
#if 0
	xst_spin_button_set_adjustment (XST_SPIN_BUTTON (tool->seconds), NULL);
	xst_spin_button_set_adjustment (XST_SPIN_BUTTON (tool->minutes), NULL);
	xst_spin_button_set_adjustment (XST_SPIN_BUTTON (tool->hours),   NULL);
#endif	

#if 1
	gtk_signal_connect (GTK_OBJECT (tool->seconds), "button_press_event",
			    GTK_SIGNAL_FUNC (xst_time_button_press_event_cb),
			    tool);
	gtk_signal_connect (GTK_OBJECT (tool->minutes), "button_press_event",
			    GTK_SIGNAL_FUNC (xst_time_button_press_event_cb),
			    tool);
	gtk_signal_connect (GTK_OBJECT (tool->hours), "button_press_event",
			    GTK_SIGNAL_FUNC (xst_time_button_press_event_cb),
			    tool);
#endif	
#if 1
	gtk_signal_connect (GTK_OBJECT (tool->hours), "insert_text",
			    GTK_SIGNAL_FUNC (xst_time_filter),
			    tool);
	gtk_signal_connect (GTK_OBJECT (tool->seconds), "insert_text",
			    GTK_SIGNAL_FUNC (xst_time_filter),
			    tool);
	gtk_signal_connect (GTK_OBJECT (tool->minutes), "insert_text",
			    GTK_SIGNAL_FUNC (xst_time_filter),
			    tool);
	
#endif	
#if 1
	gtk_signal_connect (GTK_OBJECT (tool->hours), "key_press_event",
			    GTK_SIGNAL_FUNC (xst_time_key_press_event_cb),
			    tool);
	gtk_signal_connect (GTK_OBJECT (tool->seconds), "key_press_event",
			    GTK_SIGNAL_FUNC (xst_time_key_press_event_cb),
			    tool);
	gtk_signal_connect (GTK_OBJECT (tool->minutes), "key_press_event",
			    GTK_SIGNAL_FUNC (xst_time_key_press_event_cb),
			    tool);
	
#endif	
#if 1
	gtk_signal_connect_after (GTK_OBJECT (tool->seconds), "focus_out_event",
			    GTK_SIGNAL_FUNC (xst_time_focus_out),
			    tool);
	gtk_signal_connect_after (GTK_OBJECT (tool->minutes), "focus_out_event",
			    GTK_SIGNAL_FUNC (xst_time_focus_out),
			    tool);
	gtk_signal_connect_after (GTK_OBJECT (tool->hours), "focus_out_event",
			    GTK_SIGNAL_FUNC (xst_time_focus_out),
			    tool);
#endif	

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
