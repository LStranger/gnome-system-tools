#include <gnome.h>
#include "e-map.h"

GtkWidget *window, *scroll, *map;
int id;

static gint zoom_in(gpointer data);

static gint zoom_out(gpointer data)
{
  gtk_timeout_remove(id);
  e_map_zoom_out(E_MAP(map));
  id = gtk_timeout_add(3000, zoom_in, NULL);

  return(0);
}


static gint zoom_in(gpointer data)
{
  gtk_timeout_remove(id);
  e_map_zoom_to_site(E_MAP(map), -60, -60);
  id = gtk_timeout_add(3000, zoom_out, NULL);

  return(0);
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
  gtk_widget_show_all(window);
/*  id = gtk_timeout_add(3000, zoom_in, NULL); */
  gtk_main();
  return(0);
}
