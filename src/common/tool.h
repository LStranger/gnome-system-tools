#include <glib.h>
#include <tree.h>
#include <parser.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixbuf.h>


typedef struct _ToolContext ToolContext;


typedef enum
{
	TOOL_STATE_FROZEN, TOOL_STATE_UNMODIFIED, TOOL_STATE_MODIFIED
}
ToolState;

typedef enum
{
	TOOL_COMPLEXITY_BASIC, TOOL_COMPLEXITY_INTERMEDIATE, TOOL_COMPLEXITY_ADVANCED
}
ToolComplexity;

typedef enum
{
	TOOL_READ_PROGRESS_MAX, TOOL_READ_PROGRESS_DONE
}
ToolReadState;

typedef void (*UpdateComplexityFunc) (ToolComplexity);
typedef void (*ToolXMLFunc) (xmlNode *);

struct _ToolContext
{
	gchar *task;
	xmlDocPtr config;
	GladeXML *interface;
	GladeXML *common_interface;
	GtkWidget *top_window;
	GtkWidget *child;
	gboolean frozen, modified, access;
	ToolComplexity complexity;
	UpdateComplexityFunc complexity_func;
	ToolXMLFunc to_gui;
	ToolXMLFunc to_xml;
};


GdkPixbuf *tool_load_image(char *image_name);

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

gboolean tool_get_access(void);
void tool_set_access(gboolean state);

ToolComplexity tool_get_complexity(void);
void tool_set_complexity(ToolComplexity complexity);

void tool_set_complexity_func (UpdateComplexityFunc func);
void tool_set_xml_funcs (ToolXMLFunc to_gui, ToolXMLFunc to_xml);


GtkWidget *tool_get_top_window(void);

void tool_splash_show(void);
void tool_splash_hide(void);

ToolContext *tool_init(gchar *tast, int argc, char *argv[]);

void tool_user_close(GtkWidget *widget, gpointer data);
