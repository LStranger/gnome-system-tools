#define XST_TYPE_TIME_TOOL            (xst_time_tool_get_type ())
#define XST_TIME_TOOL(obj)            (GTK_CHECK_CAST ((obj), XST_TYPE_TIME_TOOL, XstTimeTool))
#define XST_TIME_TOOL_CLASS(class)    (GTK_CHECK_CLASS_CAST ((class), XST_TYPE_TIME_TOOL, XstTimeToolClass))
#define XST_IS_TIME_TOOL(obj)         (GTK_CHECK_TYPE ((obj), XST_TYPE_TIME_TOOL))
#define XST_IS_TIME_TOOL_CLASS(class) (GTK_CHECK_CLASS_TYPE ((class), XST_TYPE_TIME_TOOL))

typedef struct _XstTimeTool      XstTimeTool;
typedef struct _XstTimeToolClass XstTimeToolClass;

struct _XstTimeTool {
	XstTool tool;

	gboolean running;
	gboolean ticking;
	
	guint timeout;

	GtkWidget *seconds;
	GtkWidget *minutes;
	GtkWidget *hours;

	gint sec;
	gint min;
	gint hrs;
};

struct _XstTimeToolClass {
	XstToolClass parent_class;
};

GtkType xst_time_tool_get_type (void);
void    xst_time_update (XstTimeTool *tool);

void xst_time_clock_stop  (XstTimeTool *tool);
void xst_time_clock_start (XstTimeTool *tool);

