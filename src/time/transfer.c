/* Functions for transferring information between XML tree and UI */

#include <gnome.h>
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <glade/glade.h>

#include "global.h"

#include "transfer.h"
#include "tz.h"
#include "e-map/e-map.h"
#include "tz-map.h"

extern XstTool *tool;

TransStringSpin transfer_string_spin_table[] =
{
	{ "hour", "hour" },
	{ "minute", "minute" },
	{ "second", "second" },
	{ 0, 0 }
};


TransStringCalendar transfer_string_calendar_table[] =
{
	{ "year", "month", "monthday", /* <-> */ "calendar" },
	{ 0, 0, 0, 0 }
};


TransTree trans_tree =
{
	0,
	0,
	0,
	0,
	transfer_string_calendar_table,
	transfer_string_spin_table
};


static void
transfer_string_spin_xml_to_gui (TransTree *trans_tree, xmlNodePtr root)
{
	int i;
	xmlNodePtr node;
	char *s;
	GtkWidget *spin;
	TransStringSpin *transfer_string_spin_table;
	
	transfer_string_spin_table = trans_tree->transfer_string_spin_table;
	if (!transfer_string_spin_table) return;
	for (i = 0; transfer_string_spin_table [i].xml_path; i++)
	{
		node = xst_xml_element_find_first (root, transfer_string_spin_table [i].xml_path);
		
		if (node && (s = xst_xml_element_get_content (node)))
		{
			spin = xst_dialog_get_widget (tool->main_dialog, transfer_string_spin_table [i].spin);
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), (gfloat) atoi (s));
			
			g_free (s);
		}
	}
}


static void
transfer_string_spin_gui_to_xml (TransTree *trans_tree, xmlNodePtr root)
{
	int i;
	xmlNodePtr node;
	char *s;
	GtkWidget *spin;
	TransStringSpin *transfer_string_spin_table;
	
	transfer_string_spin_table = trans_tree->transfer_string_spin_table;
	if (!transfer_string_spin_table) return;
	
	for (i = 0; transfer_string_spin_table [i].xml_path; i++)
	{
		spin = xst_dialog_get_widget (tool->main_dialog, transfer_string_spin_table [i].spin);
		s = g_strdup_printf ("%d", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spin)));
		node = xst_xml_element_find_first (root, transfer_string_spin_table [i].xml_path);
		xst_xml_element_set_content (node, s);
		g_free (s);
	}
}


static void
transfer_string_calendar_xml_to_gui (TransTree *trans_tree, xmlNodePtr root)
{
	int i;
	xmlNodePtr node;
	char *s;
	GtkWidget *calendar;
	TransStringCalendar *transfer_string_calendar_table;
	guint year = 0, month = 0, day = 0;
	
	transfer_string_calendar_table = trans_tree->transfer_string_calendar_table;
	if (!transfer_string_calendar_table) return;
	
	for (i = 0; transfer_string_calendar_table [i].calendar; i++)
	{
		calendar = xst_dialog_get_widget (tool->main_dialog, transfer_string_calendar_table [i].calendar);
		
		node = xst_xml_element_find_first (root, transfer_string_calendar_table [i].xml_year_path);
		if (node && (s = xst_xml_element_get_content (node)))
		{
			year = atoi (s);
			g_free (s);
		}
		
		node = xst_xml_element_find_first (root, transfer_string_calendar_table [i].xml_month_path);
		if (node && (s = xst_xml_element_get_content (node)))
		{
			month = atoi (s);
			g_free (s);
		}
		
		node = xst_xml_element_find_first (root, transfer_string_calendar_table[i].xml_day_path);
		if (node && (s = xst_xml_element_get_content (node)))
		{
			day = atoi (s);
			g_free (s);
		}
		
		gtk_calendar_select_month (GTK_CALENDAR (calendar), month - 1, year);
		gtk_calendar_select_day (GTK_CALENDAR (calendar), day);
	}
}


static void
transfer_string_calendar_gui_to_xml (TransTree *trans_tree, xmlNodePtr root)
{
	int i;
	xmlNodePtr node;
	char *s;
	GtkWidget *calendar;
	TransStringCalendar *transfer_string_calendar_table;
	guint year = 0, month = 0, day = 0;
	
	transfer_string_calendar_table = trans_tree->transfer_string_calendar_table;
	if (!transfer_string_calendar_table) return;
	
	for (i = 0; transfer_string_calendar_table [i].calendar; i++)
	{
		calendar = xst_dialog_get_widget (tool->main_dialog, transfer_string_calendar_table [i].calendar);
		gtk_calendar_get_date (GTK_CALENDAR (calendar), &year, &month, &day);
		
		node = xst_xml_element_find_first (root, transfer_string_calendar_table [i].xml_year_path);
		s = g_strdup_printf ("%d", year);
		xst_xml_element_set_content (node, s);
		g_free (s);
		
		node = xst_xml_element_find_first (root, transfer_string_calendar_table [i].xml_month_path);
		s = g_strdup_printf ("%d", month + 1);
		xst_xml_element_set_content (node, s);
		g_free (s);
		
		node = xst_xml_element_find_first (root, transfer_string_calendar_table [i].xml_day_path);
		s = g_strdup_printf ("%d", day);
		xst_xml_element_set_content (node, s);
		g_free (s);
	}
}


static void
transfer_timezone_xml_to_gui (xmlNodePtr root)
{
	xmlNodePtr node;
	char *s;
	
	node = xst_xml_element_find_first (root, "timezone");
	if (!node) return;
	
	s = xst_xml_element_get_content (node);
	e_tz_map_set_tz_from_name (tzmap, s);
	
	g_free (s);
}


static void
transfer_timezone_gui_to_xml (xmlNodePtr root)
{
	xmlNodePtr node;
	
	node = xst_xml_element_find_first (root, "timezone");
	if (!node) node = xst_xml_element_add (root, "timezone");
	
	xst_xml_element_set_content (node, e_tz_map_get_selected_tz_name (tzmap));
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
transfer_servers_xml_to_gui (xmlNodePtr root)
{
	GtkWidget *ntp_list, *item;
	GList *list_add = NULL;
	xmlNodePtr node;
	char *s;
	
	ntp_list = xst_dialog_get_widget (tool->main_dialog, "ntp_list");
	
	node = xst_xml_element_find_first (root, "synchronization");
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
transfer_servers_gui_to_xml (xmlNodePtr root)
{
	GtkWidget *ntp_list;
	xmlNodePtr node;
	
	ntp_list = xst_dialog_get_widget (tool->main_dialog, "ntp_list");
	
	node = xst_xml_element_find_first (root, "synchronization");
	if (!node) node = xst_xml_element_add (root, "synchronization");
	
	xst_xml_element_destroy_children (node);
	
	gtk_container_foreach (GTK_CONTAINER (ntp_list), server_list_get_cb, node);
}


static void
transfer_sync_toggle_xml_to_gui (xmlNodePtr root)
{
	GtkWidget *toggle;
	xmlNodePtr node;
	
	toggle = xst_dialog_get_widget (tool->main_dialog, "ntp_use");
	
	node = xst_xml_element_find_first (root, "synchronization");
	if (!node) return;
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				      xst_xml_element_get_bool_attr (node, "active"));
}


static void
transfer_sync_toggle_gui_to_xml (xmlNodePtr root)
{
	GtkWidget *toggle;
	xmlNodePtr node;
	
	toggle = xst_dialog_get_widget (tool->main_dialog, "ntp_use");
	
	node = xst_xml_element_find_first (root, "synchronization");
	if (!node) return;
	
	xst_xml_element_set_bool_attr (node, "active",
				   gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle)));
}


void
transfer_xml_to_gui (XstTool *tool, gpointer data)
{
	xmlNode *root;

	root = xst_xml_doc_get_root (tool->config);

	transfer_string_calendar_xml_to_gui (&trans_tree, root);
	transfer_string_spin_xml_to_gui (&trans_tree, root);
	transfer_timezone_xml_to_gui (root);
	transfer_servers_xml_to_gui (root);
	transfer_sync_toggle_xml_to_gui (root);
}


void
transfer_gui_to_xml (XstTool *tool, gpointer data)
{
	xmlNode *root;

	root = xst_xml_doc_get_root (tool->config);

	transfer_string_calendar_gui_to_xml (&trans_tree, root);
	transfer_string_spin_gui_to_xml (&trans_tree, root);
	transfer_timezone_gui_to_xml (root);
	transfer_servers_gui_to_xml (root);
	transfer_sync_toggle_gui_to_xml (root);
}
