/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Functions for transferring information between XML tree and UI */

#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include <gnome.h>
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <glade/glade.h>

#include "xst.h"
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

	s = xst_xml_element_get_content (node);
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

	child = xst_xml_element_find_first (node, name);
	g_return_val_if_fail (child != NULL, -1);

	return get_int_from_node (child);
}

static void
set_node_from_int (xmlNodePtr node, gint val)
{
	gchar *s;

	s = g_strdup_printf ("%d", val);
	xst_xml_element_set_content (node, s);
	g_free (s);
}

static void
set_named_child_from_int (xmlNodePtr node, gchar *name, gint val)
{
	xmlNodePtr child;

	child = xst_xml_element_find_first (node, name);
	g_return_if_fail (child != NULL);

	set_node_from_int (child, val);
}

static void
transfer_time_xml_to_gui (XstTool *xst_tool, xmlNodePtr root)
{
	XstTimeTool *tool = XST_TIME_TOOL (xst_tool);
	xmlNodePtr   local_time_node;
	struct tm    tm;

	local_time_node = xst_xml_element_find_first (root, "local_time");
	g_return_if_fail (local_time_node != NULL);

	tm.tm_year = get_int_from_named_child (local_time_node, "year") - 1900;
	tm.tm_mon  = get_int_from_named_child (local_time_node, "month");
	tm.tm_mday = get_int_from_named_child (local_time_node, "monthday");
	tm.tm_hour = get_int_from_named_child (local_time_node, "hour");
	tm.tm_min  = get_int_from_named_child (local_time_node, "minute");
	tm.tm_sec  = get_int_from_named_child (local_time_node, "second");

	xst_time_set_full (tool, &tm);
}

static void
transfer_time_gui_to_xml (XstTool *xst_tool, xmlNodePtr root)
{
	XstTimeTool *tool = XST_TIME_TOOL (xst_tool);
	GtkWidget   *calendar_widget;
	guint        year = 0, month = 0, day = 0;
	xmlNodePtr   local_time_node;

	local_time_node = xst_xml_element_find_first (root, "local_time");
	g_return_if_fail (local_time_node != NULL);

	calendar_widget = xst_dialog_get_widget (xst_tool->main_dialog, "calendar");
	gtk_calendar_get_date (GTK_CALENDAR (calendar_widget), &year, &month, &day);

	set_named_child_from_int (local_time_node, "year",     year);
	set_named_child_from_int (local_time_node, "month",    month + 1);
	set_named_child_from_int (local_time_node, "monthday", day);
	set_named_child_from_int (local_time_node, "hour",     tool->hrs);
	set_named_child_from_int (local_time_node, "minute",   tool->min);
	set_named_child_from_int (local_time_node, "second",   tool->sec);
}

static void
transfer_timezone_xml_to_gui (XstTool *tool, xmlNodePtr root)
{
	XstTimeTool *time_tool;
	xmlNodePtr node;
	char *s;

	time_tool = XST_TIME_TOOL (tool);
	
	node = xst_xml_element_find_first (root, "timezone");
	if (!node) return;
	
	s = xst_xml_element_get_content (node);
	xst_time_tool_set_time_zone_name (time_tool, s);
	
	g_free (s);
}

static void
transfer_timezone_gui_to_xml (XstTool *tool, xmlNodePtr root)
{
	XstTimeTool *time_tool;
	xmlNodePtr node;
	
	time_tool = XST_TIME_TOOL (tool);

	node = xst_xml_element_find_first (root, "timezone");
	if (!node) node = xst_xml_element_add (root, "timezone");
	
	xst_xml_element_set_content (node, time_tool->time_zone_name);
}

static GtkWidget *server_entry_found;

static void
server_list_cb (GtkWidget *item, gpointer data)
{
	char *entry_text;
	
	gtk_label_get (GTK_LABEL (GTK_BIN (item)->child), &entry_text);
	if (strstr (entry_text, data)) server_entry_found = item;
}

static void
transfer_servers_xml_to_gui (XstTool *tool, xmlNodePtr root)
{
	GtkWidget *ntp_list, *item;
	GList *list_add = NULL;
	xmlNodePtr node;
	char *s;
	
	ntp_list = xst_dialog_get_widget (tool->main_dialog, "ntp_list");
	
	node = xst_xml_element_find_first (root, "sync");
	if (!node) return;
	
	for (node = xst_xml_element_find_first(node, "server");
	     node;
	     node = xst_xml_element_find_next(node, "server"))
	{
		s = xst_xml_element_get_content (node);
		
		server_entry_found = NULL;
		gtk_container_foreach (GTK_CONTAINER (ntp_list), server_list_cb, s);
		
		if (server_entry_found) item = server_entry_found;
		else
		{
			item = gtk_list_item_new_with_label (s);
			gtk_widget_show (item);
			list_add = g_list_append (list_add, item);
		}
		
		gtk_list_item_select (GTK_LIST_ITEM(item));
		g_free (s);
	}
	
	gtk_list_append_items (GTK_LIST (ntp_list), list_add);
}

static void
server_list_get_cb (GtkWidget *item, gpointer data)
{
	xmlNodePtr node = data;
	char *s;
	
	if (GTK_WIDGET_STATE (item) == GTK_STATE_SELECTED)
	{
		gtk_label_get (GTK_LABEL (GTK_BIN (item)->child), &s);
		s = g_strdup (s);
		if (strchr (s, ' ')) *(strchr (s, ' ')) = '\0';  /* Kill comments */
		node = xst_xml_element_add (node, "server");
		xst_xml_element_set_content (node, s);
		g_free (s);
	}
}

static void
transfer_servers_gui_to_xml (XstTool *tool, xmlNodePtr root)
{
	GtkWidget *ntp_list;
	xmlNodePtr node;
	
	ntp_list = xst_dialog_get_widget (tool->main_dialog, "ntp_list");
	
	node = xst_xml_element_find_first (root, "sync");
	if (!node) node = xst_xml_element_add (root, "sync");
	
	xst_xml_element_destroy_children (node);
	
	gtk_container_foreach (GTK_CONTAINER (ntp_list), server_list_get_cb, node);
}

static void
transfer_sync_toggle_xml_to_gui (XstTool *tool, xmlNodePtr root)
{
	GtkWidget *toggle;
	xmlNodePtr node;
	
	toggle = xst_dialog_get_widget (tool->main_dialog, "ntp_use");
	
	node = xst_xml_element_find_first (root, "sync");
	if (!node) return;
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				      xst_xml_element_get_bool_attr (node, "active"));
}

static void
transfer_sync_toggle_gui_to_xml (XstTool *tool, xmlNodePtr root)
{
	GtkWidget *toggle;
	xmlNodePtr node;
	
	toggle = xst_dialog_get_widget (tool->main_dialog, "ntp_use");
	
	node = xst_xml_element_find_first (root, "sync");
	if (!node) return;
	
	xst_xml_element_set_bool_attr (node, "active",
				   gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle)));
}

static void
transfer_misc_xml_to_tool (XstTool *tool, xmlNodePtr root)
{
	xmlNodePtr node;
	gboolean res;
	gchar *str;

	res = FALSE;
	node = xst_xml_element_find_first (root, "ntpinstalled");

	if (node) {
		gtk_object_set_data (GTK_OBJECT (tool), "tool_configured", (gpointer) TRUE);
		str = xst_xml_element_get_content (node);
		res = (*str == '1')? TRUE: FALSE;
		g_free (str);
	}

	gtk_object_set_data (GTK_OBJECT (tool), "ntpinstalled", (gpointer) res);
}

void
transfer_xml_to_gui (XstTool *tool, gpointer data)
{
	xmlNode *root;

	root = xst_xml_doc_get_root (tool->config);

	transfer_time_xml_to_gui (tool, root);
	transfer_timezone_xml_to_gui (tool, root);
	transfer_servers_xml_to_gui (tool, root);
	transfer_sync_toggle_xml_to_gui (tool, root);
	transfer_misc_xml_to_tool (tool, root);
}


void
transfer_gui_to_xml (XstTool *tool, gpointer data)
{
	xmlNode *root;

	root = xst_xml_doc_get_root (tool->config);

	transfer_time_gui_to_xml (tool, root);
	transfer_timezone_gui_to_xml (tool, root);
	transfer_servers_gui_to_xml (tool, root);
	transfer_sync_toggle_gui_to_xml (tool, root);
}
