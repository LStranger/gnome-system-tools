#include <gnome.h>
#include <gdk-pixbuf/gdk-pixbuf.h>


extern int active_zone;
extern GnomeCanvas *canvas;

struct canvas_zone
{
	char *name;
	char *filename;
	GdkPixbuf *pixbuf;
};

extern struct canvas_zone canvas_zones[];

void activate_zone(int zone);
void activate_zone_by_name(char *name);
void tz_select_combo(GtkList *list, GtkWidget *child, gpointer data);
