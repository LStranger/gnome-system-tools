/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

#include "e-map.h"

GtkWidget *window, *scroll;
EMap *map;
EMapPoint *point = NULL, *highlight_point = NULL;
int id;


static gint
flash(gpointer data)
{
	if (!point) return TRUE;

	if (e_map_point_get_color_rgba (point) == 0xf010d0ff)
		e_map_point_set_color_rgba (map, point, 0x000000ff);
	else
		e_map_point_set_color_rgba (map, point, 0xf010d0ff);
	
	return(TRUE);
}


static gboolean
motion (GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
	double longitude, latitude;
	
	e_map_window_to_world (map, (double) event->x, (double) event->y,
			       &longitude, &latitude);

	if (highlight_point && highlight_point != point)
		e_map_point_set_color_rgba (map, highlight_point, 0xf010d0ff);

	highlight_point =
		e_map_get_closest_point (map, longitude, latitude, TRUE);
	
	if (highlight_point && highlight_point != point)
		e_map_point_set_color_rgba (map, highlight_point, 0xffff60ff);

	return(TRUE);
}


static gboolean
button_pressed (GtkWidget *w, GdkEventButton *event, gpointer data)
{
	double longitude, latitude;
	
	e_map_window_to_world (map, (double) event->x, (double) event->y,
			       &longitude, &latitude);

	if (event->button != 1)
		e_map_zoom_out (map);
	else
		e_map_zoom_to_location (map, longitude, latitude);

	if (point) e_map_point_set_color_rgba (map, point, 0xf010d0ff);

	point = highlight_point;
	
	return TRUE;
}


int
main (int argc, char *argv[])
{
	gtk_init (&argc, &argv);
  
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	scroll = gtk_scrolled_window_new(GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
	map = e_map_new();
  
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(scroll));
	gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(map));

	e_map_set_smooth_zoom(E_MAP(map), TRUE);
	e_map_add_point(E_MAP(map), NULL, 40.0, 0.0, 0xf010d0ff);
	e_map_add_point(E_MAP(map), NULL, 10.0, 0.0, 0xf010d0ff);
	point = e_map_add_point(E_MAP(map), NULL, 25.0, 40.0, 0xf010d0ff);
	
	g_signal_connect(G_OBJECT (map), "motion-notify-event",
			 G_CALLBACK (motion), NULL);
	g_signal_connect(G_OBJECT(map), "button-press-event",
			 G_CALLBACK (button_pressed), NULL);

	gtk_widget_show_all(window);
	id = g_timeout_add(100, flash, NULL);
	gtk_main();
	return(0);
}
