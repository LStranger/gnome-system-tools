/* Map widget.
 *
 * Copyright (C) 2000 Helix Code, Inc.
 *
 * Authors: Hans Petter Jansson <hpj@helixcode.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef E_MAP_H
#define E_MAP_H

#include <libgnome/gnome-defs.h>
#include <gtk/gtkwidget.h>

#define TYPE_E_MAP            (e_map_get_type ())
#define E_MAP(obj)            (GTK_CHECK_CAST ((obj), TYPE_E_MAP, EMap))
#define E_MAP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), TYPE_E_MAP, EMapClass))
#define IS_E_MAP(obj)         (GTK_CHECK_TYPE ((obj), TYPE_E_MAP))
#define IS_E_MAP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), TYPE_E_MAP))

typedef struct _EMap EMap;
typedef struct _EMapClass EMapClass;
typedef struct _EMapPoint EMapPoint;

struct _EMap
{
	GtkWidget widget;

	/* Private data */
	gpointer priv;
};

struct _EMapClass
{
	GtkWidgetClass parent_class;

	/* Notification signals */
	void (*zoom_fit) (EMap * view);

	/* GTK+ scrolling interface */
	void (*set_scroll_adjustments) (GtkWidget * widget,
					GtkAdjustment * hadj,
					GtkAdjustment * vadj);
};

/* The definition of Dot */

struct _EMapPoint
{
	gchar *name;  /* Can be NULL */
	double longitude, latitude;
	guint32 rgba;
	gpointer user_data;
};


GtkType
e_map_get_type (void);

EMap *
e_map_new (void);

void
e_map_set_smooth_zoom (EMap *map,
		       gboolean state);

gboolean
e_map_get_smooth_zoom (EMap *map);

void
e_map_window_to_world (EMap *map,
		       double win_x,
		       double win_y,
		       double *world_longitude,
		       double *world_latitude);

void
e_map_world_to_window (EMap *map,
		       double world_longitude,
		       double world_latitude,
		       double *win_x,
		       double *win_y);

void
e_map_zoom_to_site (EMap *map,
		    double longitude,
		    double latitude);

void
e_map_zoom_out (EMap *map);

#endif
