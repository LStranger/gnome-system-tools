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

#include <glib/gi18n.h>
#include "gst-polkit-action.h"
#include "gst-polkit-button.h"

#define GST_POLKIT_BUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_POLKIT_BUTTON, GstPolKitButtonPriv))

typedef struct GstPolKitButtonPriv GstPolKitButtonPriv;

struct GstPolKitButtonPriv {
	GstPolKitAction *action;
	gchar *label;
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

static void
update_button_state (GstPolKitButton *button)
{
	GstPolKitButtonPriv *priv;
	PolKitResult result;
	GtkWidget *image = NULL;
	const gchar *tooltip = NULL;
	gboolean sensitive = FALSE;

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (button);
	result = gst_polkit_action_get_result (priv->action);

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
	case POLKIT_RESULT_UNKNOWN:
		image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_BUTTON);
		sensitive = TRUE;
		break;
	case POLKIT_RESULT_NO:
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
action_state_changed (GstPolKitButton *button)
{
	update_button_state (button);
	g_signal_emit (button, signals[CHANGED], 0);
}

static void
gst_polkit_button_init (GstPolKitButton *button)
{
	GstPolKitButtonPriv *priv;

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (button);
	priv->action = gst_polkit_action_new (NULL, GTK_WIDGET (button));

	g_signal_connect_swapped (priv->action, "changed",
				  G_CALLBACK (action_state_changed), button);

	update_button_state (button);
}

static void
gst_polkit_button_finalize (GObject *object)
{
	GstPolKitButtonPriv *priv;

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (object);

	g_object_unref (priv->action);
	g_free (priv->label);

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
		g_value_set_string (value, gst_polkit_action_get_action (priv->action));
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
gst_polkit_button_clicked (GtkButton *button)
{
	GstPolKitButtonPriv *priv;
	PolKitResult result;

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (button);
	gst_polkit_action_authenticate (priv->action);
	result = gst_polkit_action_get_result (priv->action);

	if (result == POLKIT_RESULT_UNKNOWN) {
		GtkWidget *dialog, *toplevel;

		toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
		dialog = gtk_message_dialog_new (GTK_WINDOW (toplevel),
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Could not authenticate"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  _("An unexpected error has occurred."));

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}

	update_button_state (GST_POLKIT_BUTTON (button));
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
	return gst_polkit_action_get_action (priv->action);
}

void
gst_polkit_button_set_action (GstPolKitButton *button,
			      const gchar     *action)
{
	GstPolKitButtonPriv *priv;
	gchar *str;

	g_return_if_fail (GST_IS_POLKIT_BUTTON (button));

	priv = GST_POLKIT_BUTTON_GET_PRIVATE (button);
	gst_polkit_action_set_action (priv->action, action);
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
	return gst_polkit_action_get_authenticated (priv->action);
}
