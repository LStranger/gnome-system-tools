/* Timezone map.
 *
 * Copyright (C) 2000 Helix Code, Inc.
 *
 * Authors: Hans Petter Jansson <hpj@helixcode.com>
 * 
 * Largely based on Michael Fulbright's work on Anaconda.
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

#include "config.h"
#include <gnome.h>
#include <glade/glade.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "tz.h"
#include "e-map/e-map.h"
#include "tz-map.h"


/* --- Forward declarations of internal functions --- */


static gboolean flash_selected_point (gpointer data);
static gboolean motion (GtkWidget *widget, GdkEventMotion *event, gpointer data);
static gboolean button_pressed (GtkWidget *w, GdkEventButton *event, gpointer data);


ETzMap *e_tz_map_new ()
{
	ETzMap *tzmap;
	GPtrArray *locs;
	TzLocation *tzl;
	int i;

	tzmap = g_new0 (ETzMap, 1);
	tzmap->tzdb = tz_load_db ();
	if (!tzmap->tzdb) g_error ("Unable to load system timezone database.");
	tzmap->map = e_map_new ();
	if (!tzmap->map) g_error ("Unable to create map widget.");

	locs = tz_get_locations (tzmap->tzdb);
	
	for (i = 0; g_ptr_array_index(locs, i); i++)
	{
		tzl = g_ptr_array_index (locs, i);
		
		e_map_add_point (tzmap->map, NULL, tzl->longitude, tzl->latitude,
				 TZ_MAP_POINT_NORMAL_RGBA);
	}
	
	gtk_timeout_add (100, flash_selected_point, (gpointer) tzmap);
        gtk_signal_connect(GTK_OBJECT (tzmap->map), "motion-notify-event",
			   GTK_SIGNAL_FUNC (motion), (gpointer) tzmap);
	gtk_signal_connect(GTK_OBJECT(tzmap->map), "button-press-event",
			   GTK_SIGNAL_FUNC (button_pressed), (gpointer) tzmap);
	
	return tzmap;
}


static gboolean flash_selected_point (gpointer data)
{
	ETzMap *tzmap;

	tzmap = (ETzMap *) data;
	if (!tzmap->point_selected) return TRUE;

        if (e_map_point_get_color_rgba (tzmap->point_selected) ==
	    TZ_MAP_POINT_SELECTED_1_RGBA)
	        e_map_point_set_color_rgba (tzmap->map, tzmap->point_selected,
					    TZ_MAP_POINT_SELECTED_2_RGBA);
	else
	        e_map_point_set_color_rgba (tzmap->map, tzmap->point_selected,
					    TZ_MAP_POINT_SELECTED_1_RGBA);

	return TRUE;
}


static gboolean motion (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
	ETzMap *tzmap;
	double longitude, latitude;

	tzmap = (ETzMap *) data;

	e_map_window_to_world (tzmap->map, (double) event->x, (double) event->y,
			       &longitude, &latitude);

	if (tzmap->point_hover && tzmap->point_hover != tzmap->point_selected)
	        e_map_point_set_color_rgba (tzmap->map, tzmap->point_hover,
					    TZ_MAP_POINT_NORMAL_RGBA);

	tzmap->point_hover =
	  e_map_get_closest_point (tzmap->map, longitude, latitude, TRUE);

	if (tzmap->point_hover != tzmap->point_selected)
	        e_map_point_set_color_rgba (tzmap->map, tzmap->point_hover,
					    TZ_MAP_POINT_HOVER_RGBA);

	return TRUE;
}


static gboolean button_pressed (GtkWidget *w, GdkEventButton *event, gpointer data)
{
	ETzMap *tzmap;
	double longitude, latitude;
	
	tzmap = (ETzMap *) data;

	e_map_window_to_world (tzmap->map, (double) event->x, (double) event->y,
			       &longitude, &latitude);
	
	if (event->button != 1)
	        e_map_zoom_out (tzmap->map);
	else
	{
		if (e_map_get_magnification (tzmap->map) <= 1.0)
		        e_map_zoom_to_location (tzmap->map, longitude, latitude);
	
		if (tzmap->point_selected)
		        e_map_point_set_color_rgba (tzmap->map,
						    tzmap->point_selected,
						    TZ_MAP_POINT_NORMAL_RGBA);
		tzmap->point_selected = tzmap->point_hover;
	}
	
	return TRUE;
}
