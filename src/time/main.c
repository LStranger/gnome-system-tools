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
static void update_tz (GtkWidget *w, gpointer data);
static void server_button_clicked (GtkWidget *w, gpointer data);
static void ntp_use_toggled (GtkWidget *w, XstDialog *dialog);

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
	{ "hour",              "changed",            xst_dialog_modify_cb },
	{ "minute",            "changed",            xst_dialog_modify_cb },
	{ "second",            "changed",            xst_dialog_modify_cb },
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


static gint
xst_time_clock_tick (gpointer time_tool)
{
	XstTool *xst_tool = XST_TOOL (time_tool);
	XstTimeTool *tool = XST_TIME_TOOL (time_tool);
	GtkAdjustment *adjustment;

	xst_dialog_freeze (xst_tool->main_dialog);

#if 0
	/* Seconds */
	adjustment = XST_SPIN_BUTTON (tool->seconds)->adjustment;
	if (adjustment->value != adjustment->upper) {
		gtk_adjustment_set_value (adjustment, adjustment->value + 1);
		return TRUE;
	}
	gtk_adjustment_set_value (adjustment, 0);

	/* Minutes */
	adjustment = XST_SPIN_BUTTON (tool->minutes)->adjustment;
	if (adjustment->value != adjustment->upper) {
		gtk_adjustment_set_value (adjustment, adjustment->value + 1);
		return TRUE;
	}
	gtk_adjustment_set_value (adjustment, 0);

	/* Hours */
	adjustment = XST_SPIN_BUTTON (tool->hours)->adjustment;
	if (adjustment->value != adjustment->upper) {
		gtk_adjustment_set_value (adjustment, adjustment->value + 1);
		return TRUE;
	}
#endif
	g_print ("Clock tick ..\n");
	
	xst_dialog_thaw (xst_tool->main_dialog);
	
	return TRUE;
}


/*static void
tz_select_combo (GtkList *list, GtkWidget *widget, gpointer data)
{
	GtkListItem *li;
	gchar *text;
	
	li = GTK_LIST_ITEM (widget);
	gtk_label_get (GTK_LABEL (GTK_BIN (li)->child), &text);
	
	e_tz_map_set_tz_from_name (tzmap, text);
}*/

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

	spin = GTK_SPIN_BUTTON (widget);
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
	/* empty */
}

static XstTool *
xst_time_tool_new (void)
{
	return XST_TOOL (gtk_type_new (XST_TYPE_TIME_TOOL));
}

static void
xst_time_load_widgets (XstTimeTool *tool)
{
	XstDialog *dialog = XST_TOOL (tool)->main_dialog;

	tool->seconds = xst_dialog_get_widget (dialog, "second");
	tool->minutes = xst_dialog_get_widget (dialog, "minute");
	tool->hours   = xst_dialog_get_widget (dialog, "hour");

#if 0
	adjustment = gtk_adjustment_new (1, 0, 59, 1, 1, 1);
	tool->seconds = xst_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
	gtk_widget_show (tool->seconds);
	
	adjustment = gtk_adjustment_new (1, 0, 59, 1, 1, 1);
	tool->minutes = xst_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
	gtk_widget_show (tool->minutes);

	adjustment = gtk_adjustment_new (1, 0, 50, 1, 1, 1);	
	tool->hours   = xst_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
	gtk_widget_show (tool->hours);

	gtk_box_pack_start (GTK_BOX (hbox), tool->hours,   FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), tool->minutes, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), tool->seconds, FALSE, FALSE, 0);
#endif	
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

#if 0	
	xst_time_set_spin (tool->main_dialog, XST_TIME_TOOL (tool)->hours,   24, FALSE);
	xst_time_set_spin (tool->main_dialog, XST_TIME_TOOL (tool)->minutes, 60, TRUE);
	xst_time_set_spin (tool->main_dialog, XST_TIME_TOOL (tool)->seconds, 60, TRUE);
#endif	

	xst_time_populate_ntp_list (XST_TIME_TOOL (tool));
	xst_time_init_timezone (XST_TIME_TOOL (tool));

	xst_tool_main (tool, TRUE);
	gtk_timeout_add (1000, xst_time_clock_tick, XST_TIME_TOOL (tool));
	gtk_main ();

	return 0;
}
