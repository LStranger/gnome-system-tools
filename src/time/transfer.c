/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Functions for transferring information between XML tree and UI */

#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include <gnome.h>

#include "gst.h"
#include "time-tool.h"

#include "transfer.h"
#include "tz.h"
#include "e-map/e-map.h"
#include "tz-map.h"

static gint
get_int_from_node (xmlNodePtr node)
{
	gint   val;
	gchar *s;

	s = gst_xml_element_get_content (node);
	if (!s || !strlen (s))
	{
		g_warning ("XML node did not contain any value!");
		return -1;
	}

	val = atoi (s);
	g_free (s);

	return val;
}

static gint
get_int_from_named_child (xmlNodePtr node, gchar *name)
{
	xmlNodePtr child;

	child = gst_xml_element_find_first (node, name);
	g_return_val_if_fail (child != NULL, -1);

	return get_int_from_node (child);
}

static void
set_node_from_int (xmlNodePtr node, gint val)
{
	gchar *s;

	s = g_strdup_printf ("%d", val);
	gst_xml_element_set_content (node, s);
	g_free (s);
}

static void
set_named_child_from_int (xmlNodePtr node, gchar *name, gint val)
{
	xmlNodePtr child;

	child = gst_xml_element_find_first (node, name);
	g_return_if_fail (child != NULL);

	set_node_from_int (child, val);
}

static void
transfer_time_xml_to_gui (GstTool *gst_tool, xmlNodePtr root)
{
	GstTimeTool *tool = GST_TIME_TOOL (gst_tool);
	xmlNodePtr   local_time_node;
	struct tm    tm;

	local_time_node = gst_xml_element_find_first (root, "local_time");
	g_return_if_fail (local_time_node != NULL);

	tm.tm_year = get_int_from_named_child (local_time_node, "year") - 1900;
	tm.tm_mon  = get_int_from_named_child (local_time_node, "month") - 1;
	tm.tm_mday = get_int_from_named_child (local_time_node, "monthday");
	tm.tm_hour = get_int_from_named_child (local_time_node, "hour");
	tm.tm_min  = get_int_from_named_child (local_time_node, "minute");
	tm.tm_sec  = get_int_from_named_child (local_time_node, "second");

	gst_time_set_full (tool, &tm);
}

static void
transfer_time_gui_to_xml (GstTool *gst_tool, xmlNodePtr root)
{
	GstTimeTool *tool = GST_TIME_TOOL (gst_tool);
	GtkWidget   *calendar_widget;
	guint        year = 0, month = 0, day = 0;
	xmlNodePtr   local_time_node;

	local_time_node = gst_xml_element_find_first (root, "local_time");
	g_return_if_fail (local_time_node != NULL);

	calendar_widget = gst_dialog_get_widget (gst_tool->main_dialog, "calendar");
	gtk_calendar_get_date (GTK_CALENDAR (calendar_widget), &year, &month, &day);

	set_named_child_from_int (local_time_node, "year",     year);
	set_named_child_from_int (local_time_node, "month",    month + 1);
	set_named_child_from_int (local_time_node, "monthday", day);
	set_named_child_from_int (local_time_node, "hour",     tool->hrs);
	set_named_child_from_int (local_time_node, "minute",   tool->min);
	set_named_child_from_int (local_time_node, "second",   tool->sec);
}

static void
transfer_timezone_xml_to_gui (GstTool *tool, xmlNodePtr root)
{
	GstTimeTool *time_tool;
	xmlNodePtr node;
	char *s;

	time_tool = GST_TIME_TOOL (tool);
	
	node = gst_xml_element_find_first (root, "timezone");
	if (!node) return;
	
	s = gst_xml_element_get_content (node);
	gst_time_tool_set_time_zone_name (time_tool, s);
	
	g_free (s);
}

static void
transfer_timezone_gui_to_xml (GstTool *tool, xmlNodePtr root)
{
	GstTimeTool *time_tool;
	xmlNodePtr node;
	
	time_tool = GST_TIME_TOOL (tool);

	node = gst_xml_element_find_first (root, "timezone");
	if (!node) node = gst_xml_element_add (root, "timezone");
	
	gst_xml_element_set_content (node, time_tool->time_zone_name);
}

static gboolean server_entry_found;

static void
server_list_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
		gpointer data)
{
	char *text;
	GValue value = {0, };    /* Initialized the variable --AleX */

	gtk_tree_model_get_value (model, iter, 1, &value);
	
	if (strstr (g_value_get_string (&value), data)) {
		gtk_list_store_set (GTK_LIST_STORE (model), iter, 0, TRUE, -1);
		server_entry_found = TRUE;
	}
}

static void
transfer_servers_xml_to_gui (GstTool *tool, xmlNodePtr root)
{
	GtkWidget *ntp_list, *item;
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	xmlNodePtr node;
	char *s;
	
	ntp_list = gst_dialog_get_widget (tool->main_dialog, "ntp_list2");
	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (ntp_list)));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ntp_list));
	
	node = gst_xml_element_find_first (root, "sync");
	if (!node) return;
	
	for (node = gst_xml_element_find_first(node, "server");
	     node;
	     node = gst_xml_element_find_next(node, "server"))
	{
		s = gst_xml_element_get_content (node);
		
		server_entry_found = FALSE;
		gtk_tree_model_foreach (GTK_TREE_MODEL (store), (GtkTreeModelForeachFunc) server_list_cb, s);
		
		if (!server_entry_found) {
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, TRUE, 1, s, -1);
		}
		
		g_free (s);
	}
}

static void
transfer_sync_toggle_xml_to_gui (GstTool *tool, xmlNodePtr root)
{
	GtkWidget *toggle;
	xmlNodePtr node;
	
	toggle = gst_dialog_get_widget (tool->main_dialog, "ntp_use");
	
	node = gst_xml_element_find_first (root, "sync");
	if (!node) return;
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				      gst_xml_element_get_bool_attr (node, "active"));
}

static void
transfer_sync_toggle_gui_to_xml (GstTool *tool, xmlNodePtr root)
{
	GtkWidget *toggle;
	xmlNodePtr node;
	
	toggle = gst_dialog_get_widget (tool->main_dialog, "ntp_use");
	
	node = gst_xml_element_find_first (root, "sync");
	if (!node) return;
	
	gst_xml_element_set_bool_attr (node, "active",
				   gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle)));
}

static void
transfer_misc_xml_to_tool (GstTool *tool, xmlNodePtr root)
{
	xmlNodePtr node;
	gboolean res;
	gchar *str;

	res = FALSE;
	node = gst_xml_element_find_first (root, "ntpinstalled");

	if (node) {
		g_object_set_data (G_OBJECT (tool), "tool_configured", (gpointer) TRUE);
		str = gst_xml_element_get_content (node);
		res = (*str == '1')? TRUE: FALSE;
		g_free (str);
	}

	g_object_set_data (G_OBJECT (tool), "ntpinstalled", (gpointer) res);
}

void
transfer_xml_to_gui (GstTool *tool, gpointer data)
{
	xmlNode *root;

	root = gst_xml_doc_get_root (tool->config);

	transfer_time_xml_to_gui (tool, root);
	transfer_timezone_xml_to_gui (tool, root);
	transfer_servers_xml_to_gui (tool, root);
	transfer_sync_toggle_xml_to_gui (tool, root);
	transfer_misc_xml_to_tool (tool, root);
}


void
transfer_gui_to_xml (GstTool *tool, gpointer data)
{
	xmlNode *root;

	root = gst_xml_doc_get_root (tool->config);

	transfer_time_gui_to_xml (tool, root);
	transfer_timezone_gui_to_xml (tool, root);
	transfer_sync_toggle_gui_to_xml (tool, root);
}
