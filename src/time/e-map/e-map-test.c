#include <gnome.h>
#include "e-map.h"

GtkWidget *window, *scroll, *map;
EMapPoint *point;
int id;

static gint zoom_in(gpointer data);

static gint zoom_out(gpointer data)
{
  e_map_zoom_out(E_MAP(map));
  return(0);
}


static gint zoom_in(gpointer data)
{
  e_map_zoom_to_site(E_MAP(map), -60, -60);
  return(0);
}


static gint flash(gpointer data)
{
	if (e_map_point_get_color_rgba (point) == 0xf010d0ff)
	  e_map_point_set_color_rgba (map, point, 0xffffffff);
	else
	  e_map_point_set_color_rgba (map, point, 0xf010d0ff);
	
	return(TRUE);
}


int main(int argc, char *argv[])
{
  gnome_init("e-map-test", "0.0.0", argc, argv);
  
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  scroll = gtk_scrolled_window_new(GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
  map = GTK_WIDGET(e_map_new());
  
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(scroll));
  gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(map));

  e_map_set_smooth_zoom(E_MAP(map), TRUE);
  point = e_map_add_point(E_MAP(map), NULL, 10.0, 0.0, 0xf010d0ff);
  gtk_widget_show_all(window);
  id = gtk_timeout_add(100, flash, NULL);
  gtk_main();
  return(0);
}
