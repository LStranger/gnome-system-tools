#include <glib.h>
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <glade/glade.h>


typedef struct _ToolContext ToolContext;

typedef enum
{
  TOOL_STATE_FROZEN, TOOL_STATE_UNMODIFIED, TOOL_STATE_MODIFIED
}
ToolState;

typedef enum
{
  TOOL_READ_PROGRESS_MAX,
  TOOL_READ_PROGRESS_DONE
}
ToolReadState;

struct _ToolContext
{
	gchar *task;
	xmlDocPtr config;
	GladeXML *interface;
	GladeXML *common_interface;
  gboolean frozen, modified;

  ToolReadState read_state;
  guint progress_max,
        progress_done;
};


void tool_context_destroy(ToolContext *tc);

gboolean tool_config_load(void);
gboolean tool_config_save(void);

xmlDocPtr tool_config_get_xml(void);
void tool_config_set_xml(xmlDocPtr xml);

GtkWidget *tool_widget_get(gchar *name);
GtkWidget *tool_widget_get_common(gchar *name);

gboolean tool_get_modified(void);
void tool_set_modified(gboolean state);
void tool_modified_cb(void);

gboolean tool_get_frozen(void);
void tool_set_frozen(gboolean state);

void tool_splash_show(void);
void tool_splash_hide(void);

ToolContext *tool_init(gchar *tast, int argc, char *argv[]);
