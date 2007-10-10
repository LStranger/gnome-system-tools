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
#include "gst-polkit-button.h"

#define GST_POLKIT_BUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_POLKIT_BUTTON, GstPolKitButtonPriv))

typedef struct GstPolKitButtonPriv GstPolKitButtonPriv;

struct GstPolKitButtonPriv {
	DBusConnection *system_bus;
	DBusConnection *session_bus;
	PolKitContext *pk_context;

	GtkWidget *invisible;

	gchar *action;
	gchar *label;

	guint during_authentication : 1;
	guint authenticated : 1;
};

enum {
	CHANGED,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_ACTION,
	PROP_LABEL,
	PROP_AUTHENTICATED
};

static void      gst_polkit_button_class_init    (GstPolKitButtonClass *class);
static void      gst_polkit_button_init          (GstPolKitButton      *button);
static void      gst_polkit_button_finalize      (GObject              *object);

static void      gst_polkit_button_get_property  (GObject              *object,
						  guint                 prop_id,
						  GValue               *value,
						  GParamSpec           *pspec);
static void      gst_polkit_button_set_property  (GObject              *object,
						  guint                 prop_id,
						  const GValue         *value,
						  GParamSpec           *pspec);

static void      gst_polkit_button_clicked       (GtkButton            *button);


static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GstPolKitButton, gst_polkit_button, GTK_TYPE_BUTTON)

static void
gst_polkit_button_class_init (GstPolKitButtonClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkButtonClass *button_class = GTK_BUTTON_CLASS (class);

	object_class->finalize = gst_polkit_button_finalize;
	object_class->get_property = gst_polkit_button_get_property;
	object_class->set_property = gst_polkit_button_set_property;

	button_class->clicked = gst_polkit_button_clicked;

	g_object_class_install_property (object_class,
					 PROP_ACTION,
					 g_param_spec_string ("action",
							      "Action",
							      "PolicyKit action to manipulate",
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_LABEL,
					 g_param_spec_string ("label",
							      "Label",
							      "Button label",
							      NULL,
							      G_PARAM_READWRITE));
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
			      G_STRUCT_OFFSET (GstPolKitButtonClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	g_type_class_add_private (object_class,
				  sizeof (GstPolKitButtonPriv));
}

static PolKitResult
can_caller_do_action (GstPolKitButton *button)
{
	GstPolKitButtonPriv *priv;
	PolKitAction *action;
	PolKitCaller *caller;
	PolKitResult result;
	DBusError error;

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (button);

	if (!priv->system_bus || !priv->pk_context || !priv->action)
		return POLKIT_RESULT_NO;

	dbus_error_init (&error);
	caller = polkit_caller_new_from_pid (priv->system_bus, getpid (), &error);

	if (dbus_error_is_set (&error)) {
		g_critical (error.message);
		dbus_error_free (&error);
		return POLKIT_RESULT_NO;
	}

	action = polkit_action_new ();
	polkit_action_set_action_id (action, priv->action);

	result = polkit_context_can_caller_do_action (priv->pk_context, action, caller);

	polkit_caller_unref (caller);
	polkit_action_unref (action);

	return result;
}

static void
update_button_state (GstPolKitButton *button)
{
	GstPolKitButtonPriv *priv;
	PolKitResult result;
	GtkWidget *image = NULL;
	gchar *tooltip = NULL;
	gboolean authenticated = FALSE;
	gboolean sensitive = FALSE;

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (button);
	result = can_caller_do_action (button);
	authenticated = (result == POLKIT_RESULT_YES);

	if (priv->authenticated != authenticated) {
		priv->authenticated = authenticated;
		g_object_notify (G_OBJECT (button), "authenticated");
		g_signal_emit (button, signals [CHANGED], 0);
	}

	switch (result) {
	case POLKIT_RESULT_YES:
		image = gtk_image_new_from_stock (GTK_STOCK_YES, GTK_ICON_SIZE_BUTTON);
		tooltip = N_("This action is allowed");
		break;
	case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH:
	case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_SESSION:
	case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_ALWAYS:
	case POLKIT_RESULT_ONLY_VIA_SELF_AUTH:
	case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_SESSION:
	case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_ALWAYS:
		image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_BUTTON);
		sensitive = TRUE;
		break;
	case POLKIT_RESULT_NO:
	case POLKIT_RESULT_UNKNOWN:
		image = gtk_image_new_from_stock (GTK_STOCK_NO, GTK_ICON_SIZE_BUTTON);
		tooltip = N_("This action is not allowed");
		break;
	default:
		g_warning ("Unhandled PolKitResult");
	}

	gtk_button_set_image (GTK_BUTTON (button), image);
	gtk_widget_set_tooltip_text (GTK_WIDGET (button), _(tooltip));
	gtk_widget_set_sensitive (GTK_WIDGET (button), sensitive);
}

static void
gst_polkit_button_init (GstPolKitButton *button)
{
	GstPolKitButtonPriv *priv;
	DBusError error;

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (button);

	dbus_error_init (&error);
	priv->pk_context = polkit_context_new ();
	priv->system_bus = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
	dbus_connection_set_exit_on_disconnect (priv->system_bus, FALSE);

	priv->invisible = gtk_invisible_new ();
	gtk_widget_show (priv->invisible);

	if (dbus_error_is_set (&error)) {
		g_critical ("Cannot create system bus: %s", error.message);
		dbus_error_free (&error);
	}

	priv->session_bus = dbus_bus_get (DBUS_BUS_SESSION, &error);
	dbus_connection_set_exit_on_disconnect (priv->session_bus, FALSE);

	if (dbus_error_is_set (&error)) {
		g_critical ("Cannot create session bus: %s", error.message);
		dbus_error_free (&error);
	}

	dbus_connection_setup_with_g_main (priv->session_bus, NULL);

	/* FIXME: listen when polkit configuration changes */

	if (!polkit_context_init (priv->pk_context, NULL)) {
		g_critical ("Could not initialize PolKitContext");
	}

	update_button_state (button);
}

static void
gst_polkit_button_finalize (GObject *object)
{
	GstPolKitButtonPriv *priv;

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (object);

	g_free (priv->action);
	g_free (priv->label);

	polkit_context_unref (priv->pk_context);
	dbus_connection_unref (priv->system_bus);
	dbus_connection_unref (priv->session_bus);

	G_OBJECT_CLASS (gst_polkit_button_parent_class)->finalize (object);
}

static void
gst_polkit_button_get_property (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
	GstPolKitButtonPriv *priv;

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_ACTION:
		g_value_set_string (value, priv->action);
		break;
	case PROP_LABEL:
		g_value_set_string (value, priv->label);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
gst_polkit_button_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
	GstPolKitButton *button;

	button = GST_POLKIT_BUTTON (object);

	switch (prop_id) {
	case PROP_ACTION:
		gst_polkit_button_set_action (button, g_value_get_string (value));
		break;
	case PROP_LABEL:
		gst_polkit_button_set_label (button, g_value_get_string (value));
		break;
	default:
		break;
	}
}

static void
async_reply_cb (DBusPendingCall *pending_call,
		gpointer         data)
{
	GstPolKitButton *button;
	GstPolKitButtonPriv *priv;
	DBusMessage *reply;
	DBusMessageIter iter;
	gboolean authenticated = FALSE;

	button = GST_POLKIT_BUTTON (data);
	priv = GST_POLKIT_BUTTON_GET_PRIVATE (button);

	reply = dbus_pending_call_steal_reply (pending_call);
	dbus_message_iter_init (reply, &iter);
	dbus_message_iter_get_basic (&iter, &authenticated);

	priv->during_authentication = FALSE;
	update_button_state (button);

	gtk_grab_remove (priv->invisible);

	dbus_message_unref (reply);
	dbus_pending_call_unref (pending_call);
}

static void
gst_polkit_button_clicked (GtkButton *button)
{
	GstPolKitButtonPriv *priv;
	DBusMessage *message;
	DBusPendingCall *pending_call;
	DBusMessageIter iter;
	DBusError error;
	GtkWidget *toplevel;
	guint32 xid;

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (button);
	xid = 0;

	if (!priv->action || !priv->session_bus)
		return;

	if (priv->during_authentication)
		return;

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));

	if (GTK_WIDGET_TOPLEVEL (toplevel) &&
	    GTK_WIDGET_REALIZED (toplevel))
		xid = GDK_WINDOW_XID (toplevel->window);

	priv->during_authentication = TRUE;
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
					      g_object_ref (button), g_object_unref);

		/* set grab on the invisible, so all the UI is
		 * frozen in order to simulate a modal dialog
		 */
		gtk_grab_add (priv->invisible);
	}
}

GtkWidget *
gst_polkit_button_new (const gchar *action,
		       const gchar *label)
{
	return g_object_new (GST_TYPE_POLKIT_BUTTON,
			     "action", action,
			     "label", label,
			     NULL);
}

G_CONST_RETURN gchar *
gst_polkit_button_get_action (GstPolKitButton *button)
{
	GstPolKitButtonPriv *priv;

	g_return_val_if_fail (GST_IS_POLKIT_BUTTON (button), NULL);

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (button);
	return priv->action;
}

void
gst_polkit_button_set_action (GstPolKitButton *button,
			      const gchar     *action)
{
	GstPolKitButtonPriv *priv;
	gchar *str;

	g_return_if_fail (GST_IS_POLKIT_BUTTON (button));

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (button);

	str = g_strdup (action);
	g_free (priv->action);
	priv->action = str;

	update_button_state (button);

	g_object_notify (G_OBJECT (button), "action");
}

G_CONST_RETURN gchar *
gst_polkit_button_get_label (GstPolKitButton *button)
{
	GstPolKitButtonPriv *priv;

	g_return_val_if_fail (GST_IS_POLKIT_BUTTON (button), NULL);

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (button);
	return priv->label;
}

void
gst_polkit_button_set_label (GstPolKitButton *button,
			     const gchar     *label)
{
	GstPolKitButtonPriv *priv;
	gchar *str;

	g_return_if_fail (GST_IS_POLKIT_BUTTON (button));

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (button);

	str = g_strdup (label);
	g_free (priv->label);
	priv->label = str;

	gtk_button_set_label (GTK_BUTTON (button), priv->label);

	g_object_notify (G_OBJECT (button), "label");
}

gboolean
gst_polkit_button_get_authenticated (GstPolKitButton *button)
{
	GstPolKitButtonPriv *priv;

	g_return_val_if_fail (GST_IS_POLKIT_BUTTON (button), FALSE);

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (button);

	return priv->authenticated;
}
