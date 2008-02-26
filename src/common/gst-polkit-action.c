/* Copyright (C) 2007 Carlos Garnacho
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
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
 * Authors: Carlos Garnacho  <carlosg@gnome.org>
 */

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <polkit/polkit.h>
#include <polkit-dbus/polkit-dbus.h>
#include <gdk/gdkx.h>
#include <glib/gi18n.h>
#include "gst-polkit-action.h"

#define GST_POLKIT_ACTION_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_POLKIT_ACTION, GstPolKitActionPriv))

typedef struct GstPolKitActionPriv GstPolKitActionPriv;

struct GstPolKitActionPriv {
	DBusConnection *system_bus;
	DBusConnection *session_bus;
	PolKitContext *pk_context;

	GtkWidget *widget;
	GtkWidget *invisible;
	GMainLoop *main_loop;

	gchar *action;

	PolKitResult result;
	guint retrieved_info : 1;
};

enum {
	CHANGED,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_ACTION,
	PROP_WIDGET,
	PROP_AUTHENTICATED
};

static void      gst_polkit_action_class_init    (GstPolKitActionClass *class);
static void      gst_polkit_action_init          (GstPolKitAction      *action);
static void      gst_polkit_action_finalize      (GObject              *object);

static void      gst_polkit_action_get_property  (GObject              *object,
						  guint                 prop_id,
						  GValue               *value,
						  GParamSpec           *pspec);
static void      gst_polkit_action_set_property  (GObject              *object,
						  guint                 prop_id,
						  const GValue         *value,
						  GParamSpec           *pspec);

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GstPolKitAction, gst_polkit_action, G_TYPE_OBJECT)

static void
gst_polkit_action_class_init (GstPolKitActionClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = gst_polkit_action_finalize;
	object_class->get_property = gst_polkit_action_get_property;
	object_class->set_property = gst_polkit_action_set_property;

	g_object_class_install_property (object_class,
					 PROP_ACTION,
					 g_param_spec_string ("action",
							      "Action",
							      "PolicyKit action to manipulate",
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_WIDGET,
					 g_param_spec_object ("widget",
							      "widget",
							      "widget to get window XID from",
							      GTK_TYPE_WIDGET,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_AUTHENTICATED,
					 g_param_spec_boolean ("authenticated",
							       "Authenticated",
							       "Whether the action is authenticated",
							       FALSE,
							       G_PARAM_READABLE));
	signals [CHANGED] =
		g_signal_new ("changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GstPolKitActionClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	g_type_class_add_private (object_class,
				  sizeof (GstPolKitActionPriv));
}

static PolKitResult
can_caller_do_action (GstPolKitAction *action)
{
	GstPolKitActionPriv *priv;
	PolKitAction *pk_action;
	PolKitCaller *caller;
	PolKitResult result;
	DBusError error;

	priv = GST_POLKIT_ACTION_GET_PRIVATE (action);

	if (!priv->system_bus || !priv->pk_context || !priv->action)
		return POLKIT_RESULT_NO;

	dbus_error_init (&error);
	caller = polkit_caller_new_from_pid (priv->system_bus, getpid (), &error);

	if (dbus_error_is_set (&error)) {
		g_critical (error.message);
		dbus_error_free (&error);
		return POLKIT_RESULT_NO;
	}

	pk_action = polkit_action_new ();
	polkit_action_set_action_id (pk_action, priv->action);

	result = polkit_context_can_caller_do_action (priv->pk_context, pk_action, caller);

	polkit_caller_unref (caller);
	polkit_action_unref (pk_action);

	return result;
}

static void
gst_polkit_action_init (GstPolKitAction *action)
{
	GstPolKitActionPriv *priv;
	DBusError error;

	priv = GST_POLKIT_ACTION_GET_PRIVATE (action);

	dbus_error_init (&error);
	priv->pk_context = polkit_context_new ();
	priv->system_bus = dbus_bus_get (DBUS_BUS_SYSTEM, &error);

	priv->main_loop = g_main_loop_new (NULL, FALSE);

	priv->invisible = gtk_invisible_new ();
	gtk_widget_show (priv->invisible);

	if (dbus_error_is_set (&error)) {
		g_critical ("Cannot create system bus: %s", error.message);
		dbus_error_free (&error);
	} else {
		dbus_connection_set_exit_on_disconnect (priv->system_bus, FALSE);
	}

	priv->session_bus = dbus_bus_get (DBUS_BUS_SESSION, &error);

	if (dbus_error_is_set (&error)) {
		g_critical ("Cannot create session bus: %s", error.message);
		dbus_error_free (&error);
	} else {
		dbus_connection_set_exit_on_disconnect (priv->session_bus, FALSE);
		dbus_connection_setup_with_g_main (priv->session_bus, NULL);
	}

	/* FIXME: listen when polkit configuration changes */

	if (!polkit_context_init (priv->pk_context, NULL)) {
		g_critical ("Could not initialize PolKitContext");
	}
}

static void
gst_polkit_action_finalize (GObject *object)
{
	GstPolKitActionPriv *priv;

	priv = GST_POLKIT_ACTION_GET_PRIVATE (object);

	g_free (priv->action);

	polkit_context_unref (priv->pk_context);
	dbus_connection_unref (priv->system_bus);
	dbus_connection_unref (priv->session_bus);

	G_OBJECT_CLASS (gst_polkit_action_parent_class)->finalize (object);
}

static void
gst_polkit_action_get_property (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
	GstPolKitActionPriv *priv;

	priv = GST_POLKIT_ACTION_GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_ACTION:
		g_value_set_string (value, priv->action);
		break;
	case PROP_WIDGET:
		g_value_set_object (value, priv->widget);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
gst_polkit_action_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
	GstPolKitAction *action;
	GstPolKitActionPriv *priv;

	action = GST_POLKIT_ACTION (object);
	priv = GST_POLKIT_ACTION_GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_ACTION:
		gst_polkit_action_set_action (action, g_value_get_string (value));
		break;
	case PROP_WIDGET:
		priv->widget = g_value_dup_object (value);
		break;
	default:
		break;
	}
}

GstPolKitAction *
gst_polkit_action_new (const gchar *action,
		       GtkWidget   *widget)
{
	return g_object_new (GST_TYPE_POLKIT_ACTION,
			     "action", action,
			     "widget", widget,
			     NULL);
}

G_CONST_RETURN gchar *
gst_polkit_action_get_action (GstPolKitAction *action)
{
	GstPolKitActionPriv *priv;

	g_return_val_if_fail (GST_IS_POLKIT_ACTION (action), NULL);

	priv = GST_POLKIT_ACTION_GET_PRIVATE (action);
	return priv->action;
}

void
gst_polkit_action_set_action (GstPolKitAction *action,
			      const gchar     *action_str)
{
	GstPolKitActionPriv *priv;
	gchar *str;

	g_return_if_fail (GST_IS_POLKIT_ACTION (action));

	priv = GST_POLKIT_ACTION_GET_PRIVATE (action);

	str = g_strdup (action_str);
	g_free (priv->action);
	priv->action = str;

	priv->result = can_caller_do_action (action);
	priv->retrieved_info = TRUE;

	g_object_notify (G_OBJECT (action), "action");
}

PolKitResult
gst_polkit_action_get_result (GstPolKitAction *action)
{
	GstPolKitActionPriv *priv;

	g_return_val_if_fail (GST_IS_POLKIT_ACTION (action), FALSE);

	priv = GST_POLKIT_ACTION_GET_PRIVATE (action);

	if (!priv->retrieved_info) {
		priv->result = can_caller_do_action (action);
		priv->retrieved_info = TRUE;
	}

	return priv->result;
}

gboolean
gst_polkit_action_get_authenticated (GstPolKitAction *action)
{
	PolKitResult result;

	g_return_val_if_fail (GST_IS_POLKIT_ACTION (action), FALSE);

	result = gst_polkit_action_get_result (action);
	return (result == POLKIT_RESULT_YES);
}

static void
async_reply_cb (DBusPendingCall *pending_call,
		gpointer         data)
{
	GstPolKitAction *action;
	GstPolKitActionPriv *priv;
	DBusMessage *reply;
	DBusMessageIter iter;
	DBusError error;
	gboolean authenticated = FALSE;
	gboolean was_authenticated;

	action = GST_POLKIT_ACTION (data);
	priv = GST_POLKIT_ACTION_GET_PRIVATE (action);
	dbus_error_init (&error);

	reply = dbus_pending_call_steal_reply (pending_call);

	if (dbus_set_error_from_message (&error, reply)) {
		g_critical (error.message);
		dbus_error_free (&error);
		priv->result = POLKIT_RESULT_UNKNOWN;
	} else {
		dbus_message_iter_init (reply, &iter);
		dbus_message_iter_get_basic (&iter, &authenticated);

		was_authenticated = (priv->result == POLKIT_RESULT_YES);

		if (was_authenticated != authenticated) {
			priv->result = (authenticated) ?
				POLKIT_RESULT_YES : can_caller_do_action (action);

			g_object_notify (G_OBJECT (action), "authenticated");
			g_signal_emit (action, signals [CHANGED], 0);
		}
	}

	gtk_grab_remove (priv->invisible);
	g_main_loop_quit (priv->main_loop);

	dbus_message_unref (reply);
	dbus_pending_call_unref (pending_call);
}

gboolean
gst_polkit_action_authenticate (GstPolKitAction *action)
{
	GstPolKitActionPriv *priv;
	DBusMessage *message;
	DBusPendingCall *pending_call;
	DBusMessageIter iter;
	DBusError error;
	GtkWidget *toplevel;
	guint32 xid;

	g_return_if_fail (GST_IS_POLKIT_ACTION (action));

	priv = GST_POLKIT_ACTION_GET_PRIVATE (action);
	xid = 0;

	if (!priv->action || !priv->session_bus)
		return FALSE;

	if (gst_polkit_action_get_authenticated (action))
		return TRUE;

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (priv->widget));

	if (GTK_WIDGET_TOPLEVEL (toplevel) &&
	    GTK_WIDGET_REALIZED (toplevel))
		xid = GDK_WINDOW_XID (toplevel->window);

	dbus_error_init (&error);
	message = dbus_message_new_method_call ("org.gnome.PolicyKit",
						"/org/gnome/PolicyKit/Manager",
						"org.gnome.PolicyKit.Manager",
						"ShowDialog");

	dbus_message_iter_init_append (message, &iter);
	dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &priv->action);
	dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &xid);

	dbus_connection_send_with_reply (priv->session_bus, message, &pending_call, -1);

	if (pending_call) {
		dbus_pending_call_set_notify (pending_call, async_reply_cb,
					      g_object_ref (action), g_object_unref);

		/* set grab on the invisible, so all the UI is
		 * frozen in order to simulate a modal dialog
		 */
		gtk_grab_add (priv->invisible);

		/* wait here for the pending call to return */
		g_main_loop_run (priv->main_loop);
	}

	return gst_polkit_action_get_authenticated (action);
}
