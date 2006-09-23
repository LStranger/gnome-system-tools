/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2005 Carlos Garnacho.
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
 * Authors: Carlos Garnacho Parro <carlosg@gnome.org>
 */

#include <glib.h>
#include <glib/gi18n.h>
#include <dbus/dbus.h>
#include "time-tool.h"
#include "gst.h"
#include "ntp-servers-list.h"

#define GST_TIME_TOOL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_TIME_TOOL, GstTimeToolPrivate))
#define APPLY_CONFIG_TIMEOUT 2000

#define SCREENSAVER_SERVICE "org.gnome.ScreenSaver"
#define SCREENSAVER_PATH "/org/gnome/ScreenSaver"
#define SCREENSAVER_INTERFACE "org.gnome.ScreenSaver"

typedef struct _GstTimeToolPrivate GstTimeToolPrivate;

struct _GstTimeToolPrivate {
	guint clock_timeout;
	guint apply_timeout;

	DBusConnection *bus_connection;
	gint cookie;
};


static void  gst_time_tool_class_init     (GstTimeToolClass *class);
static void  gst_time_tool_init           (GstTimeTool      *tool);
static void  gst_time_tool_finalize       (GObject          *object);

static GObject *gst_time_tool_constructor (GType                  type,
					   guint                  n_construct_properties,
					   GObjectConstructParam *construct_params);
static void  gst_time_tool_update_gui     (GstTool *tool);
static void  gst_time_tool_update_config  (GstTool *tool);

static void  on_ntp_use_toggled           (GtkWidget *widget,
					   gpointer   data);


G_DEFINE_TYPE (GstTimeTool, gst_time_tool, GST_TYPE_TOOL);

static void
gst_time_tool_class_init (GstTimeToolClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GstToolClass *tool_class = GST_TOOL_CLASS (class);
	
	object_class->constructor = gst_time_tool_constructor;
	object_class->finalize = gst_time_tool_finalize;
	tool_class->update_gui = gst_time_tool_update_gui;
	tool_class->update_config = gst_time_tool_update_config;

	g_type_class_add_private (object_class,
				  sizeof (GstTimeToolPrivate));
}

static void
get_ntp_service (GstTimeTool *tool)
{
	GstTimeToolPrivate *priv = GST_TIME_TOOL_GET_PRIVATE (tool);
	GObject *service;
	OobsList *list;
	OobsListIter iter;
	gboolean valid;
	GstServiceRole role;

	list = oobs_services_config_get_services (tool->services_config);
	valid = oobs_list_get_iter_first (list, &iter);

	while (valid) {
		service = oobs_list_get (list, &iter);
		role = gst_service_get_role (OOBS_SERVICE (service));

		if (role == GST_ROLE_NTP_SERVER)
			tool->ntp_service = g_object_ref (service);

		g_object_unref (service);
		valid = oobs_list_iter_next (list, &iter);
	}
}

static void
gst_time_tool_init (GstTimeTool *tool)
{
	OobsList *list;
	GstTimeToolPrivate *priv = GST_TIME_TOOL_GET_PRIVATE (tool);

	priv->bus_connection = dbus_bus_get (DBUS_BUS_SESSION, NULL);
	priv->cookie = 0;

	tool->time_config = oobs_time_config_get (GST_TOOL (tool)->session);
	tool->ntp_config = oobs_ntp_config_get (GST_TOOL (tool)->session);
	tool->services_config = oobs_services_config_get (GST_TOOL (tool)->session);

	get_ntp_service (tool);
}

static void
inhibit_screensaver (GstTimeTool *tool,
		     gboolean     inhibit)
{
	GstTimeToolPrivate *priv = GST_TIME_TOOL_GET_PRIVATE (tool);
	DBusMessage *message, *reply;
	DBusMessageIter iter;

	if (inhibit) {
		const gchar *appname = "Time-admin";
		const gchar *reason = "Changing time";

		g_return_if_fail (priv->cookie == 0);

		message = dbus_message_new_method_call (SCREENSAVER_SERVICE,
							SCREENSAVER_PATH,
							SCREENSAVER_INTERFACE,
							"Inhibit");
		/* set args */
		dbus_message_iter_init_append (message, &iter);
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &appname);
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &reason);

		reply = dbus_connection_send_with_reply_and_block (priv->bus_connection, message, -1, NULL);

		if (reply) {
			/* get cookie */
			dbus_message_iter_init (reply, &iter);
			dbus_message_iter_get_basic (&iter, &priv->cookie);

			dbus_message_unref (message);
			dbus_message_unref (reply);
		}
	} else if (!inhibit && priv->cookie != 0) {
		
		message = dbus_message_new_method_call (SCREENSAVER_SERVICE,
							SCREENSAVER_PATH,
							SCREENSAVER_INTERFACE,
							"UnInhibit");
		/* set args */
		dbus_message_iter_init_append (message, &iter);
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &priv->cookie);

		dbus_connection_send (priv->bus_connection, message, NULL);
		dbus_message_unref (message);

		priv->cookie = 0;
	}
}

static gboolean
on_apply_timeout (GstTimeTool *tool)
{
	GstTimeToolPrivate *priv = GST_TIME_TOOL_GET_PRIVATE (tool);
	guint year, month, day, hour, minute, second;

	gtk_calendar_get_date (GTK_CALENDAR (tool->calendar), &year, &month, &day);
	hour   = (guint) gtk_spin_button_get_value (GTK_SPIN_BUTTON (tool->hours));
	minute = (guint) gtk_spin_button_get_value (GTK_SPIN_BUTTON (tool->minutes));
	second = (guint) gtk_spin_button_get_value (GTK_SPIN_BUTTON (tool->seconds));

	inhibit_screensaver (tool, TRUE);

	oobs_time_config_set_time (OOBS_TIME_CONFIG (tool->time_config),
				   (gint) year, (gint) month, (gint) day,
				   (gint) hour, (gint) minute, (gint)second);

	oobs_object_commit (tool->time_config);
	gst_time_tool_start_clock (tool);

	inhibit_screensaver (tool, FALSE);

	return FALSE;
}

static void
update_apply_timeout (GstTimeTool *tool)
{
	GstTimeToolPrivate *priv = GST_TIME_TOOL_GET_PRIVATE (tool);

	gst_time_tool_stop_clock (tool);

	if (priv->apply_timeout) {
		g_source_remove (priv->apply_timeout);
		priv->apply_timeout = 0;
	}

	priv->apply_timeout = g_timeout_add (APPLY_CONFIG_TIMEOUT, (GSourceFunc) on_apply_timeout, tool);
}

static void
on_value_changed (GtkWidget *widget, gpointer data)
{
	gint value;
	gchar *str;

	value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));
	str = g_strdup_printf ("%02d", (gint) value);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), value);
	gtk_entry_set_text (GTK_ENTRY (widget), str);
	g_free (str);
}

static void
on_editable_changed (GtkWidget *widget, gpointer data)
{
	update_apply_timeout (GST_TIME_TOOL (data));
}

#define is_leap_year(yyy) ((((yyy % 4) == 0) && ((yyy % 100) != 0)) || ((yyy % 400) == 0));

static void
change_calendar (GtkWidget *calendar, gint increment)
{
	gint day, month, year;
	gint days_in_month;
	gboolean leap_year;

	static const gint month_length[2][13] = {
		{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
		{ 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
	};

	gtk_calendar_get_date (GTK_CALENDAR (calendar),
			       (guint*) &year, (guint*) &month, (guint*) &day);

	leap_year = is_leap_year (year);
	days_in_month = month_length [leap_year][month+1];

	if (increment != 0) {
		day += increment;

		if (day < 1) {
			day = month_length [leap_year][month] + day;
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

		gtk_calendar_select_month (GTK_CALENDAR (calendar),
					   month, year);
		gtk_calendar_select_day (GTK_CALENDAR (calendar),
					 day);
	}
}

static void
on_spin_button_wrapped (GtkWidget *widget, gpointer data)
{
	GstTimeTool *tool = GST_TIME_TOOL (data);
	gint value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));

	if (widget == tool->seconds)
		gtk_spin_button_spin (GTK_SPIN_BUTTON (tool->minutes),
				      (value == 0) ? GTK_SPIN_STEP_FORWARD : GTK_SPIN_STEP_BACKWARD, 1);
	else if (widget == tool->minutes)
		gtk_spin_button_spin (GTK_SPIN_BUTTON (tool->hours),
				      (value == 0) ? GTK_SPIN_STEP_FORWARD : GTK_SPIN_STEP_BACKWARD, 1);
	else if (widget == tool->hours)
		change_calendar (tool->calendar, (value == 0) ? 1 : -1);
}

static void
on_calendar_day_selected (GtkWidget *widget, gpointer data)
{
	update_apply_timeout (GST_TIME_TOOL (data));
}

static GtkWidget*
prepare_spin_button (GstTool *tool, const gchar *widget_name)
{
	GtkWidget *widget;

	widget = gst_dialog_get_widget (tool->main_dialog, widget_name);

	g_signal_connect (G_OBJECT (widget), "changed",
			  G_CALLBACK (on_editable_changed), tool);
	g_signal_connect (G_OBJECT (widget), "wrapped",
			  G_CALLBACK (on_spin_button_wrapped), tool);
	/*
	g_signal_connect (G_OBJECT (widget), "value-changed",
			  G_CALLBACK (on_value_changed), tool);
	*/

	return widget;
}

void
init_timezone (GstTimeTool *time_tool)
{
	GstTool *tool = GST_TOOL (time_tool);
	GtkWidget *w;
	GPtrArray *locs;
	GList *combo_locs = NULL;
	int i;

	time_tool->tzmap = e_tz_map_new (tool);
	g_return_if_fail (time_tool->tzmap != NULL);
	
	w = gst_dialog_get_widget (tool->main_dialog, "map_window");
	gtk_container_add (GTK_CONTAINER (w), GTK_WIDGET (time_tool->tzmap->map));
	gtk_widget_show (GTK_WIDGET (time_tool->tzmap->map));

	w = gst_dialog_get_widget (tool->main_dialog, "location_combo");
	locs = tz_get_locations (e_tz_map_get_tz_db (time_tool->tzmap));

	for (i = 0; g_ptr_array_index (locs, i); i++)
		gtk_combo_box_append_text (GTK_COMBO_BOX (w),
					   tz_location_get_zone (g_ptr_array_index (locs, i)));

	time_tool->timezone_dialog = gst_dialog_get_widget (tool->main_dialog, "time_zone_window");
}

static gboolean
check_ntp_support (GstTool  *tool,
		   gboolean  active)
{
	GtkWidget *message, *widget;

	if (GST_TIME_TOOL (tool)->ntp_service)
		return TRUE;

	widget = gst_dialog_get_widget (tool->main_dialog, "ntp_use");
	g_signal_handlers_block_by_func (widget, on_ntp_use_toggled, tool);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
	g_signal_handlers_unblock_by_func (widget, on_ntp_use_toggled, tool);

	message = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
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

	return FALSE;
}

static void
on_ntp_use_toggled (GtkWidget *widget,
		    gpointer   data)
{
	GstTool *tool;
	gboolean active;

	tool = (GstTool *) data;
	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

	if (check_ntp_support (tool, active)) {
		oobs_service_set_runlevel_configuration (GST_TIME_TOOL (tool)->ntp_service,
							 oobs_services_config_get_default_runlevel (GST_TIME_TOOL (tool)->services_config),
							 (active) ? OOBS_SERVICE_START : OOBS_SERVICE_STOP,
							 /* FIXME: hardcoded priority? */
							 50);

		oobs_object_commit (GST_TIME_TOOL (tool)->services_config);
		gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "timeserver_button"), active);
		gtk_widget_set_sensitive (GST_TIME_TOOL (tool)->synchronize_now, !active);
	}
}

static void
on_synchronize_now_clicked (GtkWidget *widget, gpointer data)
{
	GstTimeTool *tool;

	tool = GST_TIME_TOOL (data);
	gst_tool_commit_async (tool, tool->ntp_config, _("Synchronizing system clock"));
}

static GObject*
gst_time_tool_constructor (GType                  type,
			   guint                  n_construct_properties,
			   GObjectConstructParam *construct_params)
{
	GObject *object;
	GstTimeTool *time_tool;

	object = (* G_OBJECT_CLASS (gst_time_tool_parent_class)->constructor) (type,
									       n_construct_properties,
									       construct_params);
	time_tool = GST_TIME_TOOL (object);
	time_tool->map_hover_label = gst_dialog_get_widget (GST_TOOL (time_tool)->main_dialog, "location_label");
	time_tool->hours    = prepare_spin_button (GST_TOOL (time_tool), "hours");
	time_tool->minutes  = prepare_spin_button (GST_TOOL (time_tool), "minutes");
	time_tool->seconds  = prepare_spin_button (GST_TOOL (time_tool), "seconds");

	time_tool->calendar = gst_dialog_get_widget (GST_TOOL (time_tool)->main_dialog, "calendar");
	g_signal_connect (G_OBJECT (time_tool->calendar), "day-selected",
			  G_CALLBACK (on_calendar_day_selected), time_tool);

	time_tool->ntp_use  = gst_dialog_get_widget (GST_TOOL (time_tool)->main_dialog, "ntp_use");
	g_signal_connect (G_OBJECT (time_tool->ntp_use), "toggled",
			  G_CALLBACK (on_ntp_use_toggled), time_tool);

	time_tool->ntp_list = ntp_servers_list_get (time_tool);
	init_timezone (time_tool);

	time_tool->synchronize_now = gst_dialog_get_widget (GST_TOOL (time_tool)->main_dialog, "synchronize_now_button");
	g_signal_connect (G_OBJECT (time_tool->synchronize_now), "clicked",
			  G_CALLBACK (on_synchronize_now_clicked), time_tool);

	return object;
}

static void
gst_time_tool_finalize (GObject *object)
{
	GstTimeTool *tool = GST_TIME_TOOL (object);
	GstTimeToolPrivate *priv = GST_TIME_TOOL_GET_PRIVATE (object);

	/* FIXME: missing things to free */
	g_object_unref (tool->time_config);

	(* G_OBJECT_CLASS (gst_time_tool_parent_class)->finalize) (object);
}

static void
update_servers_list (GstTimeTool *tool)
{
	GstTimeToolPrivate *priv;
	OobsList *list;
	OobsListIter iter;
	GObject *server;
	gboolean valid;

	/* FIXME: restore NTP servers to NULL */
	list = oobs_ntp_config_get_servers (tool->ntp_config);
	valid = oobs_list_get_iter_first (list, &iter);

	while (valid) {
		server = oobs_list_get (list, &iter);
		ntp_servers_list_check (GST_TIME_TOOL (tool)->ntp_list,
					&iter, OOBS_NTP_SERVER (server));

		g_object_unref (server);
		valid = oobs_list_iter_next (list, &iter);
	}
}

static void
gst_time_tool_update_gui (GstTool *tool)
{
	GstTimeTool *time_tool = GST_TIME_TOOL (tool);
	GtkWidget *timezone, *ntp_use, *timeserver_button;
	gboolean is_active = FALSE;
	gint active;

	gst_time_tool_start_clock (GST_TIME_TOOL (tool));
	timezone = gst_dialog_get_widget (tool->main_dialog, "tzlabel");
	ntp_use = gst_dialog_get_widget (tool->main_dialog, "ntp_use");
	timeserver_button = gst_dialog_get_widget (tool->main_dialog, "timeserver_button");

	gtk_label_set_text (GTK_LABEL (timezone),
			    oobs_time_config_get_timezone (OOBS_TIME_CONFIG (time_tool->time_config)));

	update_servers_list (GST_TIME_TOOL (tool));

	if (time_tool->ntp_service) {
		oobs_service_get_runlevel_configuration (time_tool->ntp_service,
							 oobs_services_config_get_default_runlevel (GST_TIME_TOOL (tool)->services_config),
							 &active,
							 NULL);
		is_active = (active == OOBS_SERVICE_START);
	}

	g_signal_handlers_block_by_func (ntp_use, on_ntp_use_toggled, tool);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ntp_use), is_active);
	gtk_widget_set_sensitive (timeserver_button, is_active);
	gtk_widget_set_sensitive (time_tool->synchronize_now, !is_active);
	g_signal_handlers_unblock_by_func (ntp_use, on_ntp_use_toggled, tool);
}

static void
gst_time_tool_update_config (GstTool *tool)
{
	GstTimeTool *time_tool;

	time_tool = GST_TIME_TOOL (tool);
	oobs_object_update (time_tool->time_config);
	oobs_object_update (time_tool->ntp_config);
	oobs_object_update (time_tool->services_config);

	get_ntp_service (tool);
}

GstTool*
gst_time_tool_new (void)
{
	return g_object_new (GST_TYPE_TIME_TOOL,
			     "name", "time",
			     "title", _("Time and Date Settings"),
			     "icon", "config-date",
			     NULL);
}

static void
freeze_clock (GstTimeTool *tool)
{
	g_signal_handlers_block_by_func (tool->hours,   on_editable_changed, tool);
	g_signal_handlers_block_by_func (tool->minutes, on_editable_changed, tool);
	g_signal_handlers_block_by_func (tool->seconds, on_editable_changed, tool);
	g_signal_handlers_block_by_func (tool->calendar, on_calendar_day_selected, tool);
}

static void
thaw_clock (GstTimeTool *tool)
{
	g_signal_handlers_unblock_by_func (tool->hours,   on_editable_changed, tool);
	g_signal_handlers_unblock_by_func (tool->minutes, on_editable_changed, tool);
	g_signal_handlers_unblock_by_func (tool->seconds, on_editable_changed, tool);
	g_signal_handlers_unblock_by_func (tool->calendar, on_calendar_day_selected, tool);
}

void
gst_time_tool_update_clock (GstTimeTool *tool)
{
	gint year, month, day, hour, minute, second;

	oobs_time_config_get_time (OOBS_TIME_CONFIG (tool->time_config),
				   &year, &month,  &day,
				   &hour, &minute, &second);

	freeze_clock (tool);

	gtk_calendar_select_month (GTK_CALENDAR (tool->calendar), month, year);
	gtk_calendar_select_day   (GTK_CALENDAR (tool->calendar), day);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool->hours),   (gfloat) hour);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool->minutes), (gfloat) minute);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool->seconds), (gfloat) second);

	thaw_clock (tool);
}

static gboolean
clock_tick (gpointer data)
{
	GstTimeTool *tool = (GstTimeTool *) data;

	gst_time_tool_update_clock (tool);

	return TRUE;
}

void
gst_time_tool_start_clock (GstTimeTool *tool)
{
	GstTimeToolPrivate *priv = GST_TIME_TOOL_GET_PRIVATE (tool);

	if (!priv->clock_timeout) {
		/* Do a preliminary update, just for showing
		   something with sense in the gui immediatly */
		gst_time_tool_update_clock (tool);

		priv->clock_timeout = g_timeout_add (1000, (GSourceFunc) clock_tick, tool);
	}
}

void
gst_time_tool_stop_clock (GstTimeTool *tool)
{
	GstTimeToolPrivate *priv = GST_TIME_TOOL_GET_PRIVATE (tool);

	if (priv->clock_timeout) {
		g_source_remove (priv->clock_timeout);
		priv->clock_timeout = 0;
	}
}

void
gst_time_tool_run_timezone_dialog (GstTimeTool *time_tool)
{
	GstTool *tool;
	GtkWidget *label;
	gchar *timezone;
	gchar *tz_name = NULL;
	gchar *old_tz_name = NULL;
	TzLocation *tz_location;
	gint correction;

	tool  = GST_TOOL (time_tool);
	label = gst_dialog_get_widget (tool->main_dialog, "tzlabel");

	timezone = oobs_time_config_get_timezone (OOBS_TIME_CONFIG (time_tool->time_config));
	e_tz_map_set_tz_from_name (time_tool->tzmap, timezone);

	gtk_window_set_transient_for (GTK_WINDOW (time_tool->timezone_dialog),
				      GTK_WINDOW (tool->main_dialog));

	gtk_widget_show_all (time_tool->timezone_dialog);
	gtk_dialog_run (GTK_DIALOG (time_tool->timezone_dialog));

	gtk_widget_hide_all (time_tool->timezone_dialog);
	tz_name     = e_tz_map_get_selected_tz_name (time_tool->tzmap);
	tz_location = e_tz_map_get_location_by_name (time_tool->tzmap, tz_name);

	if (!timezone || strcmp (tz_name, timezone) != 0) {
		oobs_time_config_set_timezone (OOBS_TIME_CONFIG (time_tool->time_config), tz_name);
		oobs_object_commit (time_tool->time_config);
		gtk_label_set_text (GTK_LABEL (label), tz_name);
	}
}
