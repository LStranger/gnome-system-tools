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
#include <polkit/polkit.h>

#ifndef __GST_POLKIT_ACTION_H__
#define __GST_POLKIT_ACTION_H__

G_BEGIN_DECLS

#define GST_TYPE_POLKIT_ACTION        (gst_polkit_action_get_type ())
#define GST_POLKIT_ACTION(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o),  GST_TYPE_POLKIT_ACTION, GstPolKitAction))
#define GST_POLKIT_ACTION_CLASS(c)    (G_TYPE_CHECK_CLASS_CAST ((c),     GST_TYPE_POLKIT_ACTION, GstPolKitActionClass))
#define GST_IS_POLKIT_ACTION(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o),  GST_TYPE_POLKIT_ACTION))
#define GST_IS_POLKIT_ACTION_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c),     GST_TYPE_POLKIT_ACTION))

typedef struct GstPolKitAction      GstPolKitAction;
typedef struct GstPolKitActionClass GstPolKitActionClass;

struct GstPolKitAction {
	GObject parent_instance;
};

struct GstPolKitActionClass {
	GObjectClass parent_class;

	void (* changed) (GstPolKitAction *action);
};

GType                  gst_polkit_action_get_type   (void) G_GNUC_CONST;

GstPolKitAction *      gst_polkit_action_new        (const gchar     *action,
						     GtkWidget       *widget);

G_CONST_RETURN gchar * gst_polkit_action_get_action (GstPolKitAction *action);
void                   gst_polkit_action_set_action (GstPolKitAction *action,
						     const gchar     *action_str);

PolKitResult           gst_polkit_action_get_result        (GstPolKitAction *action);
gboolean               gst_polkit_action_get_authenticated (GstPolKitAction *action);

gboolean               gst_polkit_action_authenticate (GstPolKitAction *action);

G_END_DECLS

#endif /* __GST_POLKIT_ACTION_H__ */
