#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "global.h"

#include "callbacks.h"
#include "transfer.h"
#include "user_group.h"

#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-preview.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gal/e-table/e-table-header.h>
#include <gal/e-table/e-table-header-item.h>
#include <gal/e-table/e-table-item.h>
#include <gal/e-table/e-cell-text.h>
#include <gal/e-table/e-cell-tree.h>
#include <gal/e-table/e-cell-checkbox.h>
#include <gal/e-table/e-table.h>
#include <gal/e-table/e-table-simple.h>

#include "e-table.h"


#define USER_COLS 1
#define GROUP_COLS 1

#define USER_SPEC "<ETableSpecification> \
  <ETableColumn model_col=\"0\" _title=\"Users\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
    <ETableState> \
      <column source=\"0\"/> \
      <grouping><leaf column=\"0\" ascending=\"true\"/></grouping> \
    </ETableState> \
  </ETableSpecification>"

#define GROUP_SPEC "<ETableSpecification> \
  <ETableColumn model_col=\"0\" _title=\"Groups\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
    <ETableState> \
      <column source=\"0\"/> \
      <grouping><leaf column=\"0\" ascending=\"true\"/></grouping> \
    </ETableState> \
  </ETableSpecification>"


GtkWidget *user_table;
GtkWidget *group_table;

/* ETreeSimple callbacks
 * These are the callbacks that define the behaviour of our custom model.
 */

/* This function returns the number of columns in our ETableModel. */
static int
col_count (ETableModel *etm, void *data)
{
	if (E_TABLE (user_table)->model == etm)
		return USER_COLS;
	else
		return GROUP_COLS;
}

static int
row_count (ETableModel *etm, void *data)
{
	return g_list_length ((GList *)data);
}

/* This function returns the value at a particular point in our ETableModel. */
static void *
value_at (ETableModel *etm, int col, int row, void *data)
{
	GList *tmp_list = data;

	if (E_TABLE (user_table)->model == etm)
	{
		user *u;

		u = g_list_nth_data (tmp_list, row);
		return u->login;
	}
	else
	{
		group *g;

		g = g_list_nth_data (tmp_list, row);
		return g->name;
	}
}

static void
set_value_at (ETableModel *etm, int col, int row, const void *val, void *data)
{
	GList *tmp_list = data;

	if (E_TABLE (user_table)->model == etm)
	{
		user *u;

		u = g_list_nth_data (tmp_list, row);
		g_free (u->login);
		u->login = g_strdup (val);
	}
	else
	{
		group *g;

		g = g_list_nth_data (tmp_list, row);
		g_free (g->name);
		g->name = g_strdup (val);
	}
}

static gboolean
is_cell_editable (ETableModel *etm, int col, int row, void *data)
{
	return FALSE;
}

/* This function duplicates the value passed to it. */
static void *
duplicate_value (ETableModel *etm, int col, const void *value, void *data)
{
	return g_strdup (value);
}

/* This function frees the value passed to it. */
static void
free_value (ETableModel *etm, int col, void *value, void *data)
{
	g_free (value);
}

/* This function creates an empty value. */
static void *
initialize_value (ETableModel *etm, int col, void *data)
{
	return g_strdup ("");
}

/* This function reports if a value is empty. */
static gboolean
value_is_empty (ETableModel *etm, int col, const void *value, void *data)
{
	return !(value && *(char *)value);
}

/* This function reports if a value is empty. */
static char *
value_to_string (ETableModel *etm, int col, const void *value, void *data)
{
	return g_strdup(value);
}

static void
select_row (ETable *et, int row)
{
	ETableModel *etm;
	gchar *txt, *label;

	etm = E_TABLE_MODEL (et->model);
	txt = e_table_model_value_at (etm, 0, row);

	if (et == E_TABLE (user_table))
	{
		current_user = g_list_nth_data (user_list, row);
		user_actions_set_sensitive (TRUE);
		label = g_strconcat (_("Settings for user "), current_user->login, NULL);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("user_settings_frame")), label);
		g_free (label);
	}
	else
	{
		current_group = g_list_nth_data (group_list, row);
		group_actions_set_sensitive (TRUE);
		label = g_strconcat ("Settings for group ", current_group->name, NULL);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("group_settings_frame")), label);
		g_free (label);
	}
}

/* Public functions */

void
e_table_user_create (void)
{
	GtkWidget *w0;
	ETableModel *e_table_model = NULL;

	/* Users table */

	e_table_model = e_table_simple_new (col_count,
                                           row_count,
                                           value_at,
                                           set_value_at,
                                           is_cell_editable,
                                           duplicate_value,
                                           free_value,
                                           initialize_value,
                                           value_is_empty,
                                           value_to_string,
                                           user_list);

	user_table = e_table_new (E_TABLE_MODEL(e_table_model), NULL, USER_SPEC, NULL);

	if (!user_table)
		g_warning ("e-table: Can't make user table");

	gtk_signal_connect (GTK_OBJECT (user_table), "cursor_change", select_row, NULL);

	w0 = tool_widget_get ("users_holder");
	gtk_container_add (GTK_CONTAINER (w0), user_table);
	gtk_widget_show (user_table);

	/* Groups table */
	e_table_model = e_table_simple_new (col_count,
                                           row_count,
                                           value_at,
                                           set_value_at,
                                           is_cell_editable,
                                           duplicate_value,
                                           free_value,
                                           initialize_value,
                                           value_is_empty,
                                           value_to_string,
                                           group_list);

	group_table = e_table_new (E_TABLE_MODEL(e_table_model), NULL, GROUP_SPEC, NULL);

	if (!group_table)
		g_warning ("e-table: Can't make group table");

	gtk_signal_connect (GTK_OBJECT (group_table), "cursor_change", select_row, NULL);

	w0 = tool_widget_get ("groups_holder");
	gtk_container_add (GTK_CONTAINER (w0), group_table);
	gtk_widget_show (group_table);
}

void
e_table_del (gchar del)
{
	gint row, nrow;
	ETableModel *etm;
	ETable *table;

	if (del == USER)
	{
		table = E_TABLE (user_table);
		user_list = g_list_remove (user_list, current_user);
	}
	else
	{
		table = E_TABLE (group_table);
		group_list = g_list_remove (group_list, current_group);
	}
	
	row = e_table_get_cursor_row (table);
	if (row == -1)
		return;

	nrow = e_table_get_prev_row (table, row);
	if (nrow < 0)
		nrow = e_table_get_next_row (table, row);

	etm = E_TABLE_MODEL (table->model);
	e_table_model_row_deleted (etm, row);

	e_table_set_cursor_row (table, nrow);
}

void
e_table_changed (gchar change, gboolean new)
{
	ETableModel *etm;

	if (change == USER)
		etm = E_TABLE_MODEL (E_TABLE (user_table)->model);
	else
		etm = E_TABLE_MODEL (E_TABLE (group_table)->model);

	if (new)
		e_table_model_append_row (etm, NULL, 0);

	e_table_model_changed (etm);

}

