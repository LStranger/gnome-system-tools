#include "config.h"

#include <stdio.h>
#include <gnome.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gnome-canvas-pixbuf.h>

#include "global.h"

#include "transfer.h"
#include "timezone.h"


GnomeCanvas *canvas;
int active_zone = -1;
GnomeCanvasItem *active_zone_item = NULL;


struct canvas_zone canvas_zones[] =
{
  { "-1100", "gmt_-11.png",    NULL },
  { "-1000", "gmt_-10.png",    NULL },
  { "-0900", "gmt_-09.png",    NULL },
  { "-0800", "gmt_-08.png",    NULL },
  { "-0700", "gmt_-07.png",    NULL },
  { "-0600", "gmt_-06.png",    NULL },
  { "-0500", "gmt_-05.png",    NULL },
  { "-0400", "gmt_-04.png",    NULL },
  { "-0345", "gmt_-03_45.png", NULL },
  { "-0300", "gmt_-03.png",    NULL },
  { "-0200", "gmt_-02.png",    NULL },
  { "-0100", "gmt_-01.png",    NULL },
  { "0000",  "gmt.png",        NULL },
  { "+0100", "gmt_+01.png",    NULL },
  { "+0200", "gmt_+02.png",    NULL },
  { "+0300", "gmt_+03.png",    NULL },
  { "+0330", "gmt_+03_30.png", NULL },
  { "+0400", "gmt_+04.png",    NULL },
  { "+0430", "gmt_+04_30.png", NULL },
  { "+0500", "gmt_+05.png",    NULL },
  { "+0530", "gmt_+05_30.png", NULL },
  { "+0600", "gmt_+06.png",    NULL },
  { "+0630", "gmt_+06_30.png", NULL },
  { "+0700", "gmt_+07.png",    NULL },
  { "+0800", "gmt_+08.png",    NULL },
  { "+0900", "gmt_+09.png",    NULL },
  { "+0930", "gmt_+09_30.png", NULL },
  { "+1000", "gmt_+10.png",    NULL },
  { "+1100", "gmt_+11.png",    NULL },
  { "+1200", "gmt_+12.png",    NULL },
  { NULL, NULL, NULL }
};


void activate_zone(int zone)
{
  GtkList *list;
  
  active_zone = zone;
  if (active_zone_item) gtk_object_destroy(GTK_OBJECT(active_zone_item));
    
  active_zone_item =
    gnome_canvas_item_new (gnome_canvas_root (canvas),
                           gnome_canvas_pixbuf_get_type(),
                           "pixbuf", canvas_zones[zone].pixbuf,
                           "x_in_pixels", TRUE,
                           "y_in_pixels", TRUE,
                           "height_set", TRUE,
                           "width_set", TRUE,
                           "height", (gdouble) 100.0,
                           "width", (gdouble) 200.0,
                           "x", (gdouble) 0.0,
                           "y", (gdouble) 0.0,
                           NULL);

  list = GTK_LIST(GTK_COMBO(tool_widget_get("timezone_combo"))->list);
  gtk_signal_handler_block_by_func(GTK_OBJECT(list), tz_select_combo, NULL);
  gtk_list_select_item(list, zone);
  gtk_signal_handler_unblock_by_func(GTK_OBJECT(list), tz_select_combo, NULL);
}


void activate_zone_by_name(char *name)
{
  int i;
  
  for (i = 0; canvas_zones[i].name; i++)
    if (!strcmp(name, canvas_zones[i].name)) break;
  
  if (canvas_zones[i].name) activate_zone(i);
}


void tz_select_combo(GtkList *list, GtkWidget *child, gpointer data)
{
  activate_zone(gtk_list_child_position(list, child));
}
