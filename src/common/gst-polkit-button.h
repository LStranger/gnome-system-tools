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

#include <gtk/gtk.h>

#ifndef __GST_POLKIT_BUTTON_H__
#define __GST_POLKIT_BUTTON_H__

G_BEGIN_DECLS

#define GST_TYPE_POLKIT_BUTTON        (gst_polkit_button_get_type ())
#define GST_POLKIT_BUTTON(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o),  GST_TYPE_POLKIT_BUTTON, GstPolKitButton))
#define GST_POLKIT_BUTTON_CLASS(c)    (G_TYPE_CHECK_CLASS_CAST ((c),     GST_TYPE_POLKIT_BUTTON, GstPolKitButtonClass))
#define GST_IS_POLKIT_BUTTON(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o),  GST_TYPE_POLKIT_BUTTON))
#define GST_IS_POLKIT_BUTTON_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c),     GST_TYPE_POLKIT_BUTTON))

typedef struct GstPolKitButton      GstPolKitButton;
typedef struct GstPolKitButtonClass GstPolKitButtonClass;

struct GstPolKitButton {
	GtkButton parent_instance;
};

struct GstPolKitButtonClass {
	GtkButtonClass parent_class;

	void (* changed) (GstPolKitButton *button);
};

GType                  gst_polkit_button_get_type   (void) G_GNUC_CONST;

GtkWidget *            gst_polkit_button_new        (const gchar     *action,
						     const gchar     *label);

G_CONST_RETURN gchar * gst_polkit_button_get_action (GstPolKitButton *button);
void                   gst_polkit_button_set_action (GstPolKitButton *button,
						     const gchar     *action);

G_CONST_RETURN gchar * gst_polkit_button_get_label  (GstPolKitButton *button);
void                   gst_polkit_button_set_label  (GstPolKitButton *button,
						     const gchar     *label);

gboolean               gst_polkit_button_get_authenticated (GstPolKitButton *button);

G_END_DECLS

#endif /* __GST_POLKIT_BUTTON_H__ */
