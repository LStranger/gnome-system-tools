#define GST_TYPE_TIME_TOOL            (gst_time_tool_get_type ())
#define GST_TIME_TOOL(obj)            (GTK_CHECK_CAST ((obj), GST_TYPE_TIME_TOOL, GstTimeTool))
#define GST_TIME_TOOL_CLASS(class)    (GTK_CHECK_CLASS_CAST ((class), GST_TYPE_TIME_TOOL, GstTimeToolClass))
#define GST_IS_TIME_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GST_TYPE_TIME_TOOL))
#define GST_IS_TIME_TOOL_CLASS(class) (GTK_CHECK_CLASS_TYPE ((class), GST_TYPE_TIME_TOOL))

typedef struct _GstTimeTool      GstTimeTool;
typedef struct _GstTimeToolClass GstTimeToolClass;

struct _GstTimeTool {
	GstTool tool;

	gboolean running;
	gboolean ticking;
	
	guint timeout;

	GtkWidget *seconds;
	GtkWidget *minutes;
	GtkWidget *hours;

	GtkWidget *map_hover_label;

	gint sec;
	gint min;
	gint hrs;

	gchar *time_zone_name;
};

struct _GstTimeToolClass {
	GstToolClass parent_class;
};

GtkType gst_time_tool_get_type           (void);
void    gst_time_update                  (GstTimeTool *tool);

void    gst_time_clock_stop              (GstTimeTool *tool);
void    gst_time_clock_start             (GstTimeTool *tool);

void    gst_time_tool_set_time_zone_name (GstTimeTool *time_tool, gchar *name);
gchar*  gst_time_tool_get_time_zone_name (GstTimeTool*);
void    gst_time_set_full                (GstTimeTool *time_tool, struct tm *tm);
void    gst_time_set_from_localtime      (GstTimeTool *time_tool, gint correction);
