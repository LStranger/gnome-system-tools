/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* profile.c: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Tambet Ingo <tambet@ximian.com> and Arturo Espinosa <arturo@ximian.com>.
 */

#include <stdlib.h>
#include <dirent.h>
#include <gnome.h>
#include <gal/e-table/e-table-scrolled.h>
#include <gal/e-table/e-table-memory.h>
#include <gal/e-table/e-table-memory-callbacks.h>

#include "xst.h"
#include "profile.h"
#include "user_group.h"
#include "callbacks.h"
#include "e-table.h"

typedef struct
{
	XstDialog       *dialog;
	
	GtkOptionMenu   *system_menu;
	GtkOptionMenu   *files_menu;
	GtkOptionMenu   *security_menu;
	GnomeFileEntry  *home_prefix;
	GtkCombo        *shell;
	GtkEntry        *group;
	GtkSpinButton   *umin;
	GtkSpinButton   *umax;
	GtkSpinButton   *gmin;
	GtkSpinButton   *gmax;
	GtkSpinButton   *pwd_maxdays;
	GtkSpinButton   *pwd_mindays;
	GtkSpinButton   *pwd_warndays;
	GtkToggleButton *pwd_random;
	GtkCList        *file_list;
	GtkWidget       *file_del;
	GtkWidget       *file_add;
} ProfileTab;

extern XstTool *tool;
ProfileTable *profile_table;
GtkWidget *table;

static ProfileTab *pft;

void on_home_activate (GtkEditable *editable, gpointer user_data);

const gchar *table_spec = "\
<ETableSpecification cursor-mode=\"line\"> \
  <ETableColumn model_col=\"0\" _title=\"Profile\" expansion=\"1.0\" minimum_width=\"60\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableColumn model_col=\"1\" _title=\"Comment\" expansion=\"1.0\" minimum_width=\"40\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableState> \
    <column source=\"0\"/> \
    <column source=\"1\"/> \
    <grouping> \
      <leaf column=\"0\" ascending=\"true\"/> \
    </grouping> \
  </ETableState> \
</ETableSpecification>";

static int
col_count (ETableModel *etm, void *data)
{
        return 0;
}

static void *
duplicate_value (ETableModel *etm, int col, const void *value, void *data)
{
        return g_strdup (value);
}

static void
free_value (ETableModel *etm, int col, void *value, void *data)
{
        g_free (value);
}

static void *
initialize_value (ETableModel *etm, int col, void *data)
{
        return g_strdup ("");
}

static gboolean
value_is_empty (ETableModel *etm, int col, const void *value, void *data)
{
        return !(value && *(char *)value);
}

static char *
value_to_string (ETableModel *etm, int col, const void *value, void *data)
{
	return (gchar *)value;
}

static void
set_value_at (ETableModel *etm, int col, int row, const void *val, void *data)
{
}

static gboolean
is_editable (ETableModel *etm, int col, int row, void *model_data)
{
	return FALSE;
}

static void *
value_at (ETableModel *etm, int col, int row, void *model_data)
{
	Profile *pf = e_table_memory_get_data (E_TABLE_MEMORY (etm), row);

	switch (col) {
	case 0:
		return pf->name;
		break;
	case 1:
		return pf->comment;
		break;
	default:
		return NULL;
	}
}

static void
et_insert (gpointer key, gpointer value, gpointer user_data)
{
	ETable *et;
	ETableModel *model;

	et = e_table_scrolled_get_table (E_TABLE_SCROLLED (table));
	model = et->model;
	
	e_table_memory_insert (E_TABLE_MEMORY (model), -1, value);
}

static void
et_remove (Profile *pf)
{
	ETable *et;
	gint row;

	et = e_table_scrolled_get_table (E_TABLE_SCROLLED (table));
	if ((row = e_table_get_cursor_row (et)) >= 0)
		e_table_memory_remove (E_TABLE_MEMORY (et->model), row);
}

static void
et_cursor_change (ETable *table, gint row, gpointer user_data)
{
	Profile *pf = e_table_memory_get_data (E_TABLE_MEMORY (table->model), row);

	if (pf) {
		profile_table_set_selected (pf->name);
		tables_update_content ();
	}
}

static void
et_cursor_set (Profile *pf)
{
	ETable *et;
	ETableMemory *model;
	Profile *table_pf;
	gint row;

	et = e_table_scrolled_get_table (E_TABLE_SCROLLED (table));
	model = E_TABLE_MEMORY (et->model);

	row = 0;
	while ((table_pf = e_table_memory_get_data (model, row))) {
		if (table_pf == pf) {
			gtk_signal_handler_block_by_func (GTK_OBJECT (et),
							  GTK_SIGNAL_FUNC (et_cursor_change),
							  NULL);
			e_table_set_cursor_row (et, row);
			gtk_signal_handler_unblock_by_func (GTK_OBJECT (et),
							    GTK_SIGNAL_FUNC (et_cursor_change),
							    NULL);
			break;
		}
		row++;
	}
}

static void
create_profile_table (XstDialog *xd)
{
	ETableModel *model;
	GtkWidget *container;
	
	model = e_table_memory_callbacks_new (col_count,
					      value_at,
					      set_value_at,
					      is_editable,
					      duplicate_value,
					      free_value,
					      initialize_value,
					      value_is_empty,
					      value_to_string,
					      NULL);
	
	table = e_table_scrolled_new (model, NULL, table_spec, NULL);
	gtk_signal_connect (GTK_OBJECT (e_table_scrolled_get_table (E_TABLE_SCROLLED (table))),
			    "cursor_change",
			    et_cursor_change,
			    NULL);

	container = xst_dialog_get_widget (xd, "profiles_holder");
	gtk_container_add (GTK_CONTAINER (container), table);
	gtk_widget_show (table);
}

/* End of ETable */


static guint
my_atoi (gchar *str) 
{
	if (!str || !*str)
		return 0;
	return atoi (str);
}

/* Profiles callbacks */

static void
pro_name_changed (GtkMenuItem *menu_item, gpointer data)
{
	profile_table_set_selected ((gchar *) data);
	tables_update_content ();
}

static void
pro_del_clicked (GtkButton *button, gpointer data)
{
	XstDialog *xd = XST_DIALOG (data);
	
	g_return_if_fail (xst_tool_get_access (tool));
	
	if (profile_table_del_profile (NULL))
		xst_dialog_modify (xd);
}

enum
{
	PROFILE_ERROR,
	PROFILE_NEW,
	PROFILE_COPY,
};

static void
pro_ask_name (XstDialog *xd, gint action)
{
	gchar *buf;
	GtkWidget *w0;
	Profile *new, *pf = NULL;
	
	switch (action)
	{
	case PROFILE_NEW:
		break;
		
	case PROFILE_COPY:
		w0 = xst_dialog_get_widget (xd, "profile_new_menu");
		buf = xst_ui_option_menu_get_selected_string (GTK_OPTION_MENU (w0));
		pf = profile_table_get_profile (buf);
		g_free (buf);
		break;
		
	case PROFILE_ERROR:
	default:
		g_warning ("pro_ask_name: Shouldn't be here");
		return;
	}

	buf = gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (xd, "profile_new_name")));
	new = profile_add (pf, buf, TRUE);
	if (new) {
		buf = gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (xd, "profile_new_comment")));
		new->comment = g_strdup (buf);
		xst_dialog_modify (xd);
	}
	
}

static void
pro_prepare (XstDialog *xd, gint action)
{
	GtkWidget *w;

	w = xst_dialog_get_widget (xd, "profile_new_name");
	gtk_entry_set_text (GTK_ENTRY (w), "");
	gtk_widget_grab_focus (w);

	gtk_entry_set_text (GTK_ENTRY (xst_dialog_get_widget (xd, "profile_new_comment")), "");

	w = xst_dialog_get_widget (xd, "profile_new_copy");
	
	if (action == PROFILE_NEW)
		gtk_widget_hide (w);
	else {
		GtkWidget *menu = xst_dialog_get_widget (xd, "profile_new_menu");
		GSList *list = profile_table_get_list ();
		Profile *pf = profile_table_get_profile (NULL);

		xst_ui_option_menu_clear (GTK_OPTION_MENU (menu));

		while (list) {
			xst_ui_option_menu_add_string (GTK_OPTION_MENU (menu), list->data);
			list = list->next;
		}
		g_slist_free (list);
		xst_ui_option_menu_set_selected_string (GTK_OPTION_MENU (menu), pf->name);
	
		gtk_widget_show (w);
	}
}

static void
pro_new_clicked (GtkButton *button, gpointer data)
{
	GtkWidget *d;
	gint res;
	XstDialog *xd = XST_DIALOG (data);

	g_return_if_fail (xst_tool_get_access (tool));

	d = xst_dialog_get_widget (xd, "profile_new_dialog");
	pro_prepare (xd, PROFILE_NEW);
	res = gnome_dialog_run (GNOME_DIALOG (d));

	if (res)
		return;

	pro_ask_name (xd, PROFILE_NEW);
}

static void
pro_copy_clicked (GtkButton *button, gpointer data)
{
	GtkWidget *d;
	gint res;
	XstDialog *xd = XST_DIALOG (data);
	
	g_return_if_fail (xst_tool_get_access (tool));
	
	d = xst_dialog_get_widget (xd, "profile_new_dialog");
	pro_prepare (xd, PROFILE_COPY);
	res = gnome_dialog_run (GNOME_DIALOG (d));

	if (res)
		return;

	pro_ask_name (xd, PROFILE_COPY);
}

static void
pro_apply_clicked (XstDialog *xd, gpointer user_data)
{
	g_return_if_fail (xst_tool_get_access (tool));

	profile_save (NULL);
	tables_update_content ();

	profile_table_to_xml (xst_xml_doc_get_root (tool->config));
	xst_dialog_modify (tool->main_dialog);
}

static void
file_list_select (GtkCList *clist, gint row, gint col, GdkEventButton *event, gpointer data)
{
	ProfileTab *gui = data;
	gchar *buf;

	if (clist->selection)
		gtk_widget_set_sensitive (GTK_WIDGET (gui->file_del), TRUE);
	else
		gtk_widget_set_sensitive (GTK_WIDGET (gui->file_del), FALSE);

	buf = gtk_clist_get_row_data (clist, row);
	g_print ("%s\n", buf);
}

static gboolean
is_directory (const gchar *full_path)
{
	/* We just check if it ends with dir separator */
	if (full_path[(strlen (full_path) - 1)] == G_DIR_SEPARATOR)
		return TRUE;
	else
		return FALSE;
}

static GSList *
get_dirlist (const gchar *full_path)
{
	DIR *dir;
	struct dirent *ep;
	GSList *list = NULL;
	
	if (!is_directory (full_path))
		return list;

	dir = opendir (full_path);
	if (dir) {
		while ((ep = readdir (dir))) {
			if (!strcmp (ep->d_name, "."))
				continue;
			if (!strcmp (ep->d_name, ".."))
				continue;

			list = g_slist_prepend (list, g_strdup_printf ("%s%s", full_path, ep->d_name));
		}
		closedir (dir);
	} else {
		/* TODO: give the reason why not */
		g_warning ("get_dirlist: failed to open directory %s", full_path);
	}

	return list;
}

static gchar *
get_filename (const gchar *full_path, gint depth)
{
	gchar *path, *retval;
	gint len, i;
	
	if (!full_path || (strlen (full_path) == 0))
		return g_strdup ("");

	path = (gchar *)full_path;
	len = strlen (path);
	retval = g_new (gchar, len);
	i = 0;
	while (--len) {
		if (path[len] == G_DIR_SEPARATOR) {
			if (--depth < 1)
				break;
		}
		retval[i++] = path[len];
	}
	retval[i] = '\0';
	g_strreverse (retval);
	return retval;
}

static void
add_file_row (GtkCList *clist, const gchar *full_path)
{
	gchar *text[2];
	gint row;
	
	text[1] = NULL;
	if (is_directory (full_path)) {
		GSList *list = get_dirlist (full_path);

		while (list) {
			gchar *name = list->data;
			list = list->next;
		
			text[0] = get_filename (name, 2);
			row = gtk_clist_append (clist, text);
			gtk_clist_set_row_data (clist, row, (gpointer) name);
		}
		g_slist_free (list);
	} else {
		text[0] = get_filename (full_path, 1);
		row = gtk_clist_append (clist, text);
		gtk_clist_set_row_data (clist, row, (gpointer) g_strdup (full_path));
	}
}

static void
file_list_add_cb (GtkButton *button,  gpointer data)
{
	GtkFileSelection *filesel = data;
	gchar *name = gtk_file_selection_get_filename (filesel);

	add_file_row (pft->file_list, name);
	xst_dialog_modify (pft->dialog);
}

static void
file_list_add (GtkButton *button, gpointer data)
{
	GtkFileSelection *filesel;

	filesel = GTK_FILE_SELECTION (gtk_file_selection_new (_("Choose the file")));
	gtk_file_selection_set_filename (filesel, "/");

	gtk_signal_connect (GTK_OBJECT (filesel->ok_button),
			    "clicked", GTK_SIGNAL_FUNC (file_list_add_cb),
			    (gpointer) filesel);
   			   
	gtk_signal_connect_object (GTK_OBJECT (filesel->ok_button),
				   "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
				   (gpointer) filesel);

	gtk_signal_connect_object (GTK_OBJECT (filesel->cancel_button),
				   "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
				   (gpointer) filesel);
   
	gtk_widget_show (GTK_WIDGET (filesel));
}

static void
file_list_del (GtkButton *button, gpointer data)
{
	gchar *buf;
	gint row;
	ProfileTab *gui = data;
	GtkCList *list = gui->file_list;

	while (list->selection) {
		row = GPOINTER_TO_INT (list->selection->data);
		gtk_clist_get_text (list, row, 0, &buf);
		gtk_clist_remove (list, row);
	}
	xst_dialog_modify (gui->dialog);
}

static void
profile_tab_prefill (void)
{
	xmlNodePtr root, node;
	GtkWidget *li;

	root = xst_xml_doc_get_root (tool->config);
	root = xst_xml_element_find_first (root, "shells");

	if (!root)
		return;

	node = xst_xml_element_find_first (root, "shell");
	while (node) {
		li = gtk_list_item_new_with_label (xst_xml_element_get_content (node));
		gtk_widget_show (li);
		gtk_container_add (GTK_CONTAINER (pft->shell->list), li);
		
		node = xst_xml_element_find_next (node, "shell");
	}
}

static XstDialogSignal signals[] = {
	{ "pro_del",                     "clicked",       pro_del_clicked },
	{ "pro_new",                     "clicked",       pro_new_clicked },
	{ "pro_copy",                    "clicked",       pro_copy_clicked },
	{ NULL }
};

static void
connect_modify_cb (XstDialog *xd, GtkWidget *widget)
{
	gtk_signal_connect (GTK_OBJECT (widget), "changed",
			    GTK_SIGNAL_FUNC (xst_dialog_modify_cb), xd);
}

static void
profile_tab_connect_signals (XstDialog *xd)
{
	gint i;
	GtkWidget *widgets[] = {
		GTK_WIDGET (pft->shell->entry),
		GTK_WIDGET (pft->group),
		GTK_WIDGET (pft->umin),
		GTK_WIDGET (pft->umax),
		GTK_WIDGET (pft->gmin),
		GTK_WIDGET (pft->gmax),
		GTK_WIDGET (pft->pwd_mindays),
		GTK_WIDGET (pft->pwd_maxdays),
		GTK_WIDGET (pft->pwd_warndays),
		NULL
	};		
	
	xst_dialog_connect_signals (xd, signals);
	
	gtk_signal_connect (GTK_OBJECT (gnome_file_entry_gtk_entry (pft->home_prefix)),
			    "activate",
			    GTK_SIGNAL_FUNC (on_home_activate),
			    (gpointer) pft->home_prefix);

	gtk_signal_connect (GTK_OBJECT (xd), "apply", GTK_SIGNAL_FUNC (pro_apply_clicked), xd);

	for (i = 0; widgets[i]; i++)
		connect_modify_cb (xd, widgets[i]);

	connect_modify_cb (xd, gnome_file_entry_gtk_entry (pft->home_prefix));
}

static void
profile_tab_init ()
{
	XstDialog *xd;
	
	if (pft)
		return;
	
	pft = g_new (ProfileTab, 1);

	xd = xst_dialog_new (tool, "pro_dialog", _("Profile Editor"));
	pft->dialog = xd;
	
	pft->system_menu = GTK_OPTION_MENU (xst_dialog_get_widget (xd, "pro_system_menu"));
	pft->files_menu = GTK_OPTION_MENU (xst_dialog_get_widget (xd, "pro_files_menu"));
	pft->security_menu = GTK_OPTION_MENU (xst_dialog_get_widget (xd, "pro_security_menu"));

	pft->home_prefix = GNOME_FILE_ENTRY (xst_dialog_get_widget (xd, "pro_home"));
	pft->shell       = GTK_COMBO (xst_dialog_get_widget (xd, "pro_shell"));
	pft->group       = GTK_ENTRY (xst_dialog_get_widget (xd, "pro_group"));
	
	pft->umin = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_umin"));
	pft->umax = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_umax"));
	pft->gmin = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_gmin"));
	pft->gmax = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_gmax"));

	pft->pwd_maxdays  = GTK_SPIN_BUTTON   (xst_dialog_get_widget (xd, "pro_maxdays"));
	pft->pwd_mindays  = GTK_SPIN_BUTTON   (xst_dialog_get_widget (xd, "pro_mindays"));
	pft->pwd_warndays = GTK_SPIN_BUTTON   (xst_dialog_get_widget (xd, "pro_between"));
	pft->pwd_random   = GTK_TOGGLE_BUTTON (xst_dialog_get_widget (xd, "pro_random"));

	pft->file_list = GTK_CLIST (xst_dialog_get_widget (xd, "pro_file_list"));
	gtk_clist_set_auto_sort (pft->file_list, TRUE);
	gtk_signal_connect (GTK_OBJECT (pft->file_list), "select_row",
			    GTK_SIGNAL_FUNC (file_list_select), pft);
	gtk_signal_connect (GTK_OBJECT (pft->file_list), "unselect_row",
			    GTK_SIGNAL_FUNC (file_list_select), pft);
	
	pft->file_del = xst_dialog_get_widget (xd, "pro_file_del");
	gtk_signal_connect (GTK_OBJECT (pft->file_del), "clicked",
			    GTK_SIGNAL_FUNC (file_list_del), pft);

	pft->file_add = xst_dialog_get_widget (xd, "pro_file_add");
	gtk_signal_connect (GTK_OBJECT (pft->file_add), "clicked",
			    GTK_SIGNAL_FUNC (file_list_add), pft);

	create_profile_table (xd);
	
	profile_tab_prefill ();

	profile_tab_connect_signals (xd);
}

static void
signals_block_cb (GtkWidget *widget)
{
	gtk_signal_handler_block_by_func (GTK_OBJECT (widget),
					  GTK_SIGNAL_FUNC (xst_dialog_modify_cb),
					  pft->dialog);
}

static void
signals_unblock_cb (GtkWidget *widget)
{
	gtk_signal_handler_unblock_by_func (GTK_OBJECT (widget),
					    GTK_SIGNAL_FUNC (xst_dialog_modify_cb),
					    pft->dialog);
}

static void
profile_tab_signals_block (gboolean block)
{
	void (*func)(GtkWidget *widget);
	gint i;
	GtkWidget *widgets[] = {
		GTK_WIDGET (pft->shell->entry),
		GTK_WIDGET (pft->group),
		GTK_WIDGET (pft->umin),
		GTK_WIDGET (pft->umax),
		GTK_WIDGET (pft->gmin),
		GTK_WIDGET (pft->gmax),
		GTK_WIDGET (pft->pwd_mindays),
		GTK_WIDGET (pft->pwd_maxdays),
		GTK_WIDGET (pft->pwd_warndays),
		NULL
	};
	
	if (block)
		func = signals_block_cb;
	else
		func = signals_unblock_cb;

	for (i = 0; widgets[i]; i++)
		func (widgets[i]);

	func (gnome_file_entry_gtk_entry (pft->home_prefix));
}

static void
profile_update_ui (Profile *pf)
{
	GSList *tmp;
	GtkOptionMenu *om[] = { pft->system_menu, pft->files_menu, pft->security_menu, NULL };
	gint i;

	profile_tab_signals_block (TRUE);
	
	et_cursor_set (pf);
	
	for (i = 0; om[i]; i++)
		xst_ui_option_menu_set_selected_string (om[i], pf->name);
	
	my_gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (pft->home_prefix)),
					pf->home_prefix);
	my_gtk_entry_set_text (GTK_ENTRY (pft->shell->entry), pf->shell);
	my_gtk_entry_set_text (GTK_ENTRY (pft->group), pf->group);
	
	gtk_spin_button_set_value (pft->umin, (gfloat) pf->umin);
	gtk_spin_button_set_value (pft->umax, (gfloat) pf->umax);
	gtk_spin_button_set_value (pft->gmin, (gfloat) pf->gmin);
	gtk_spin_button_set_value (pft->gmax, (gfloat) pf->gmax);

	gtk_spin_button_set_value (pft->pwd_maxdays,  (gfloat) pf->pwd_maxdays);
	gtk_spin_button_set_value (pft->pwd_mindays,  (gfloat) pf->pwd_mindays);
	gtk_spin_button_set_value (pft->pwd_warndays, (gfloat) pf->pwd_warndays);
	gtk_toggle_button_set_active (pft->pwd_random, pf->pwd_random);

	gtk_clist_clear (pft->file_list);
	gtk_clist_freeze (pft->file_list);
	tmp = pf->files;
	while (tmp) {
		add_file_row (pft->file_list, tmp->data);
		tmp = tmp->next;
	}
	gtk_clist_thaw (pft->file_list);
	
	profile_tab_signals_block (FALSE);
}

static void
profile_save_entry (GtkEntry *entry, gchar **data)
{
	gchar *buf;
	
	buf = gtk_entry_get_text (entry);
	
	if (validate_var (buf)) {
		if (*data)
			g_free (*data);
		*data = g_strdup (buf);
	} else
		my_gtk_entry_set_text (entry, *data);
}

void
profile_table_run (void)
{
	g_return_if_fail (pft->dialog != NULL);
	
	gtk_widget_show (GTK_WIDGET (pft->dialog));
}

void
profile_save (gchar *name)
{
	gchar *buf;
	Profile *pf;
	gint row = 0;
	
	if (name)
		buf = g_strdup (name);
	else
		buf = g_strdup (profile_table->selected);
	
	pf = (Profile *)g_hash_table_lookup (profile_table->hash, buf);
	g_free (buf);

	if (pf)	{
		/* TODO: free pf->files */
		profile_save_entry (GTK_ENTRY (gnome_file_entry_gtk_entry (pft->home_prefix)),
				    &pf->home_prefix);
		profile_save_entry (GTK_ENTRY (GTK_COMBO (pft->shell)->entry), &pf->shell);
		profile_save_entry (GTK_ENTRY (pft->group), &pf->group);
		
		pf->umin = gtk_spin_button_get_value_as_int (pft->umin);
		pf->umax = gtk_spin_button_get_value_as_int (pft->umax);
		pf->gmin = gtk_spin_button_get_value_as_int (pft->gmin);
		pf->gmax = gtk_spin_button_get_value_as_int (pft->gmax);

		pf->pwd_maxdays  = gtk_spin_button_get_value_as_int (pft->pwd_maxdays);
		pf->pwd_mindays  = gtk_spin_button_get_value_as_int (pft->pwd_mindays);
		pf->pwd_warndays = gtk_spin_button_get_value_as_int (pft->pwd_warndays);
		pf->pwd_random = gtk_toggle_button_get_active (pft->pwd_random);

		pf->files = NULL;
		while (gtk_clist_get_text (pft->file_list, row++, 0, &buf))
			pf->files = g_slist_prepend (pf->files, g_strdup (buf));
	}
}	

Profile *
profile_add (Profile *old_pf, const gchar *new_name, gboolean select)
{	
	Profile *pf;
	GtkWidget *d;
	GSList *tmp;
	gchar *buf = NULL;

	pf = profile_table_get_profile (new_name);
	if (pf)
		buf = g_strdup (N_("Profile with given name already exists."));

	if (strlen (new_name) < 1)
		buf = g_strdup (N_("Profile name must not be empty."));

	if (buf) {
		d = gnome_error_dialog_parented (buf, GTK_WINDOW (tool->main_dialog));
		gnome_dialog_run (GNOME_DIALOG (d));
		g_free (buf);
		return NULL;
	}
	
	pf = g_new0 (Profile, 1);

	pf->name = g_strdup (new_name);

	if (old_pf) {
		/* Let's make copy of old profile. */
		
		pf->home_prefix = g_strdup (old_pf->home_prefix);
		pf->shell = g_strdup (old_pf->shell);
		pf->group = g_strdup (old_pf->group);
		
		pf->umin = old_pf->umin;
		pf->umax = old_pf->umax;
		pf->gmin = old_pf->gmin;
		pf->gmax = old_pf->gmax;
		pf->pwd_maxdays = old_pf->pwd_maxdays;
		pf->pwd_mindays = old_pf->pwd_mindays;
		pf->pwd_warndays = old_pf->pwd_warndays;
		pf->pwd_random = old_pf->pwd_random;

		tmp = old_pf->files;
		while (tmp) {
			pf->files = g_slist_prepend (pf->files, tmp->data);
			tmp = tmp->next;
		}
	}

	profile_table_add_profile (pf, select);

	return pf;
}

void
profile_destroy (Profile *pf)
{
	if (!pf)
		return;

	g_free (pf->name);
	g_free (pf->home_prefix);
	g_free (pf->shell);
	g_free (pf->group);

	/* TODO: free strings in pf->files */
	g_slist_free (pf->files);
	
	g_free (pf);
}

/* Profile table functions */

void
profile_table_init (void)
{
	if (!profile_table) {
		profile_table = g_new (ProfileTable, 1);

		profile_table->selected = NULL;
		profile_table->hash = g_hash_table_new (g_str_hash, g_str_equal);
	}

	if (!pft)
		profile_tab_init ();
}

void
profile_table_destroy (void)
{
	if (!profile_table)
		return;

	g_free (profile_table->selected);
	g_hash_table_destroy (profile_table->hash);

	g_free (profile_table);
}

static GSList *
get_files (xmlNodePtr files)
{
	xmlNodePtr node;
	GSList *list = NULL;

	node = xst_xml_element_find_first (files, "file");
	while (node) {
		list = g_slist_prepend (list, xst_xml_element_get_content (node));
		node = xst_xml_element_find_next (node, "file");
	}

	return list;
}

#define XST_USER_DATA_DIR "/var/cache/xst/"

static GSList *
profile_files (GSList **list, xmlNodePtr root, const gchar *prefix)
{
	gchar *buf, *path, *tmp;

	root = root->childs;
	while (root) {
		gchar *name = xst_xml_element_get_attribute (root, "name");

		if (!name) {
			buf = xst_xml_element_get_content (root);
			if (buf) {
				tmp = g_strdup_printf ("%s%s", prefix, buf);
				g_free (buf);
				buf = strstr (tmp, XST_USER_DATA_DIR);
				if (buf)
					buf = tmp + strlen (XST_USER_DATA_DIR);
				else
					buf = tmp;
				
				*list = g_slist_append (*list, buf);
				g_print ("%s\n", buf);
			}
			root = root->next;
			continue;
		}

		path = g_strdup_printf ("%s%s", prefix, name);
		*list = profile_files (list, root, path);
		g_free (path);
		g_free (name);
		root = root->next;
	}
	return *list;
}

/* Structure with some hard-coded defaults, just in case any of the tags is not present. */
/* These were taken form RH 6.2's default values. Any better suggestions? */
/* NULL means not present for string members. */

const static Profile default_profile = {
	(N_("Default")),         /* name */
	(N_("Default profile")), /* comment */
	99999,                   /* pwd_maxdays */
	0,                       /* pwd_mindays */
	7,                       /* pwd_warndays */
	500,                     /* umin */
	60000,                   /* umax */
	500,                     /* gmin */
	60000,                   /* gmax */
	"/home/$user",           /* home_prefix */
	"/bin/bash/",            /* shell */
	"$user",                 /* group */
	FALSE,                   /* pwd_random */
	TRUE,                    /* logindefs */
	NULL                     /* files */
};

static Profile *
profile_get_default (xmlNodePtr root)
{
	Profile *pf;
	xmlNodePtr node, n0;
	gchar *tag;
	gint i;
	gchar *logindefs_tags[] = {
		"passwd_max_day_use", "passwd_min_day_use", "passwd_warning_advance_days",
		"new_user_min_id", "new_user_max_id", "new_group_min_id", "new_group_max_id",
		"files", NULL
	};

	node = xst_xml_element_find_first (root, "logindefs");
	if (!node) {
		g_warning ("profile_get_default: Can't find logindefs tag.");
		return NULL;
	}
	
	pf = g_new0 (Profile, 1);
	
	/* Assign defaults */	
	pf->name = g_strdup (default_profile.name);
	pf->comment = g_strdup (default_profile.comment);
	pf->home_prefix = g_strdup (default_profile.home_prefix);
	pf->shell = g_strdup (default_profile.shell);
	pf->group = g_strdup (default_profile.group);
	pf->umin = default_profile.umin;
	pf->umax = default_profile.umax;
	pf->gmin = default_profile.gmin;
	pf->gmax = default_profile.gmax;
	pf->pwd_maxdays = default_profile.pwd_maxdays;
	pf->pwd_mindays = default_profile.pwd_mindays;
	pf->pwd_warndays = default_profile.pwd_warndays;
	pf->pwd_random = default_profile.pwd_random;
	pf->logindefs = default_profile.logindefs;
	
	for (i = 0, tag = logindefs_tags[0]; tag; i++, tag = logindefs_tags[i]) {
		n0 = xst_xml_element_find_first (node, tag);

		if (n0) {
			switch (i) {
			case  0: pf->pwd_maxdays  = my_atoi (xst_xml_element_get_content (n0)); break;
			case  1: pf->pwd_mindays  = my_atoi (xst_xml_element_get_content (n0)); break;
			case  2: pf->pwd_warndays = my_atoi (xst_xml_element_get_content (n0)); break;
			case  3: pf->umin         = my_atoi (xst_xml_element_get_content (n0)); break;
			case  4: pf->umax         = my_atoi (xst_xml_element_get_content (n0)); break;
			case  5: pf->gmin         = my_atoi (xst_xml_element_get_content (n0)); break;
			case  6: pf->gmax         = my_atoi (xst_xml_element_get_content (n0)); break;
			case  7: pf->files        = get_files (n0); break;
				
			default: g_warning ("profile_get_default: Shouldn't be here."); break;
			}
		}
	}

	return pf;
}

void
profile_table_from_xml (xmlNodePtr root)
{
	xmlNodePtr node, n0, pf_node;
	Profile *pf;
	gchar *profile_tags[] = {
		"home_prefix", "shell", "group", "pwd_maxdays",
		"pwd_mindays", "pwd_warndays", "umin","umax",
		"gmin", "gmax", "pwd_random", "comment", "name",
		"files", NULL
	};
	gchar *tag;
	gint i;

	if ((pf = profile_get_default (root)))
		profile_table_add_profile (pf, TRUE);
	
	node = xst_xml_element_find_first (root, "profiles");
	if (!node)
		return;

	pf_node = xst_xml_element_find_first (node, "profile");	
	while (pf_node)	{
		pf = g_new (Profile, 1);
		pf->logindefs = FALSE;
		for (i = 0, tag = profile_tags[0]; tag; i++, tag = profile_tags[i]) {
			n0 = xst_xml_element_find_first (pf_node, tag);
			
			if (n0) {
				switch (i) {
				case  0: pf->home_prefix  = xst_xml_element_get_content (n0); break;
				case  1: pf->shell        = xst_xml_element_get_content (n0); break;
				case  2: pf->group        = xst_xml_element_get_content (n0); break;
				case  3: pf->pwd_maxdays  = my_atoi (xst_xml_element_get_content (n0)); break;
				case  4: pf->pwd_mindays  = my_atoi (xst_xml_element_get_content (n0)); break;
				case  5: pf->pwd_warndays = my_atoi (xst_xml_element_get_content (n0)); break;
				case  6: pf->umin         = my_atoi (xst_xml_element_get_content (n0)); break;
				case  7: pf->umax         = my_atoi (xst_xml_element_get_content (n0)); break;
				case  8: pf->gmin         = my_atoi (xst_xml_element_get_content (n0)); break;
				case  9: pf->gmax         = my_atoi (xst_xml_element_get_content (n0)); break;
				case 10: pf->pwd_random   = xst_xml_element_get_bool_attr (n0, "set"); break;
				case 11: pf->comment      = xst_xml_element_get_content (n0); break;
				case 12: pf->name         = xst_xml_element_get_content (n0); break;
				case 13: pf->files        = profile_files (NULL, n0, ""); break;
//				case 13: pf->files        = get_files (n0); break;
					
				default: g_warning ("profile_get_from_xml: we shouldn't be here."); break;
				}
			}
		}
		profile_table_add_profile (pf, FALSE);
		pf_node = xst_xml_element_find_next (pf_node, "profile");
	}
}

static void
set_files (xmlNodePtr root, GSList *list)
{
	xmlNodePtr node, n0;
	GSList *tmp = list;

	node = xst_xml_element_find_first (root, "files");
	if (node)
		xst_xml_element_destroy (node);

	node = xst_xml_element_add (root, "files");	
	while (tmp) {
		n0 = xst_xml_element_add (node, "file");
		xst_xml_element_set_content (n0, (gchar *)tmp->data);

		tmp = tmp->next;
	}
}

static void
save_logindefs_xml (Profile *pf, xmlNodePtr root)
{
	gint i, val;
	xmlNodePtr node;
	gchar *buf;
	gchar *nodes[] = { "new_user_min_id", "new_user_max_id", "new_group_min_id",
			   "new_group_max_id", "passwd_max_day_use", "passwd_min_day_use",
			   "passwd_warning_advance_days", NULL };

	/* FIXME: */
	root = xst_xml_doc_get_root (tool->config);
	root = xst_xml_element_find_first (root, "logindefs");

	for (i = 0; nodes[i]; i++)
	{
		switch (i)
		{
		case 0: val = pf->umin; break;
		case 1: val = pf->umax; break;
		case 2: val = pf->gmin; break;
		case 3: val = pf->gmax; break;
		case 4: val = pf->pwd_maxdays; break;
		case 5: val = pf->pwd_mindays; break;
		case 6: val = pf->pwd_warndays; break;
		default:
			g_warning ("save_logindefs_xml: Shouldn't be here");
			continue;
		}

		buf = g_strdup_printf ("%d", val);

		node = xst_xml_element_find_first (root, nodes[i]);
		if (!node)
			node = xst_xml_element_add (root, nodes[i]);

		xst_xml_element_set_content (node, buf);
		g_free (buf);
	}
}

static void
save_xml (gpointer key, gpointer value, gpointer user_data)
{
	xmlNodePtr root, node;
	Profile *pf;
	gint i, val;
	gchar *buf;
	gchar *nodes[] = {
		"pwd_maxdays", "pwd_mindays", "pwd_warndays",
		"umin", "umax", "gmin", "gmax", NULL};

	root = user_data;
	pf = value;

	if (pf->logindefs) { /* Logindefs is "fake" profile. */
		save_logindefs_xml (pf, root);
		return;
	}

	node = xst_xml_element_add (root, "profile");

	xst_xml_element_add_with_content (node, "name",        pf->name);
	xst_xml_element_add_with_content (node, "comment",     pf->comment);
	xst_xml_element_add_with_content (node, "home_prefix", pf->home_prefix);
	xst_xml_element_add_with_content (node, "shell",       pf->shell);
	xst_xml_element_add_with_content (node, "group",       pf->group);
	xst_xml_element_set_bool_attr (xst_xml_element_add (node, "pwd_random"),
				       "set", pf->pwd_random);
	set_files (node, pf->files);
	
	for (i = 0; nodes[i]; i++)
	{
		switch (i)
		{
		case 0: val = pf->pwd_maxdays;  break;
		case 1: val = pf->pwd_mindays;  break;
		case 2: val = pf->pwd_warndays; break;
		case 3: val = pf->umin;         break;
		case 4: val = pf->umax;         break;
		case 5: val = pf->gmin;         break;
		case 6: val = pf->gmax;         break;
		default:
			g_warning ("save_xml: Shouldn't be here");
			continue;
		}
		
		buf = g_strdup_printf ("%d", val);
		xst_xml_element_add_with_content (node, nodes[i], buf);
		g_free (buf);
	}
}

void
profile_table_to_xml (xmlNodePtr root)
{
	xmlNodePtr node;
	
	node = xst_xml_element_find_first (root, "profiles");

	if (!node)
		node = xst_xml_element_add (root, "profiles");
	else
		xst_xml_element_destroy_children (node);

	g_hash_table_foreach (profile_table->hash, save_xml, node);
}

void
profile_table_add_profile (Profile *pf, gboolean select)
{
	GtkWidget *menu_item;
	gint i;
	struct tmp {
		GtkOptionMenu *om;
		gboolean signal;
	} option_menus[] = {
		{ pft->system_menu,   TRUE },
		{ pft->files_menu,    TRUE },
		{ pft->security_menu, TRUE },
		{ NULL }
	};

	/* Add profile to: */

	/* ... Hash table */
	g_hash_table_insert (profile_table->hash, pf->name, pf);

	/* ... ETable */
	et_insert (NULL, pf, NULL);

	/* ... GtkOptionMenus */
	i = 0;
	while (option_menus[i].om) {
		menu_item = xst_ui_option_menu_add_string (option_menus[i].om, pf->name);

		if (option_menus[i].signal) {
			gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
					    GTK_SIGNAL_FUNC (pro_name_changed), pf->name);
		}
		i++;
	}

	if (select || g_hash_table_size (profile_table->hash) == 1)
		profile_table_set_selected (pf->name);
}

gboolean
profile_table_del_profile (gchar *name)
{
	gchar *buf;
	Profile *pf;
	gboolean retval;

	retval = FALSE;
	
	if (name)
		buf = g_strdup (name);
	else
		buf = g_strdup (profile_table->selected);
	
	pf = (Profile *)g_hash_table_lookup (profile_table->hash, buf);

	if (pf) {
		if (pf->logindefs) {
			GnomeDialog *d;

			d = GNOME_DIALOG (gnome_error_dialog_parented (N_("Can't delete Default"),
								       GTK_WINDOW (tool->main_dialog)));
			gnome_dialog_run (d);
		} else {
			xst_ui_option_menu_remove_string (pft->system_menu, buf);
			xst_ui_option_menu_remove_string (pft->files_menu, buf);
			xst_ui_option_menu_remove_string (pft->security_menu, buf);
			et_remove (pf);
			
			g_hash_table_remove (profile_table->hash, buf);
			profile_table->selected = NULL;
			profile_destroy (pf);
			profile_table_set_selected (NULL);
			
			retval = TRUE;
		}
	}

	g_free (buf);

	return retval;
}

static void
get_profile (gpointer key, gpointer data, gpointer user_data)
{
	profile_table->selected = key;
}

Profile *
profile_table_get_profile (const gchar *name)
{
	gchar *buf;
	Profile *pf;

	if (name)
		buf = g_strdup (name);
	else {
		if (!profile_table->selected)
			g_hash_table_foreach (profile_table->hash, get_profile, NULL);

		buf = g_strdup (profile_table->selected);
	}

	if (!buf) {
		g_warning ("profile_table_get_profile: Can't get any profile.");
		return NULL;
	}
	
	pf = (Profile *)g_hash_table_lookup (profile_table->hash, buf);
	g_free (buf);

	return pf;
}

void
profile_table_set_selected (const gchar *name)
{
	Profile *pf = profile_table_get_profile (name);

	if (!pf) {
		if (g_hash_table_size (profile_table->hash) == 0)
			profile_table->selected = NULL;
		return;
	}
	
	profile_table->selected = pf->name;
	profile_update_ui (pf);
}

static void
profile_list (gpointer key, gpointer value, gpointer data)
{
	GSList *list = data;

	list = g_slist_append (list, key);
}

GSList *
profile_table_get_list (void)
{
	GSList *list = NULL;

	list = g_slist_prepend (NULL, GINT_TO_POINTER (1));
	
	g_hash_table_foreach (profile_table->hash, profile_list, list);
	list = g_slist_remove (list, GINT_TO_POINTER (1));
	
	return list;
}

/* Not much at the time :) */
static gchar *known_vars[] = { "$user", NULL }; 

gboolean
validate_var (gchar *var)
{
	const guint max_tokens = 8;
	gchar **buf;
	gint i, j;
	gboolean found, ret;

	ret = TRUE;
	
	if (!var)
		return FALSE;

	buf = g_strsplit (var, "/", max_tokens);
	i = 0;
	while (buf[i]) {
		if (*buf[i] == '$') {
			j = 0;
			found = FALSE;
			while (known_vars[j]) {
				if (!strcmp (known_vars[j], buf[i])) {
					found = TRUE;
					break;
				}
				j++;
			}

			if (!found) {
				gchar *msg;
				GtkWidget *d;
				
				/* FIXME: list ALL known vars. */
				msg = g_strdup_printf (N_("Unknown variable '%s'.\n"
							  "Currently we support only '%s'."),
						       buf[i], *known_vars);

				d = gnome_error_dialog (msg);
				gnome_dialog_run (GNOME_DIALOG (d));
				g_free (msg);
				ret = found;
				break;
			}
		}
		i++;
	}
	
	g_strfreev (buf);
	return ret;
}

/* Callbacks. */

void
on_home_activate (GtkEditable *editable, gpointer user_data)
{
	GnomeFileEntry *fentry;
	gchar *buf, *path;

	fentry = user_data;

	if (!fentry->fsw)
		return; /* User pressed <enter> */

	path = gnome_file_entry_get_full_path (fentry, TRUE);
	buf = g_strconcat (path, "$user", NULL);
	g_free (path);

	gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (fentry)), buf);
	g_free (buf);
}
