/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * e-search-bar.c
 *
 * Copyright (C) 2000, 2001 Ximian, Inc.
 *
 * Authors:
 *  Chris Lahey <clahey@ximian.com>
 *  Ettore Perazzoli <ettore@ximian.com>
 *  Carlos Garnacho Parro <garnacho@tuxerver.net> (GTK2 adaption)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <gnome.h>

#include "search-bar.h"
#include "search-bar-marshal.h"

enum {
	QUERY_CHANGED,

	LAST_SIGNAL
};

static gint sb_signals [LAST_SIGNAL] = { 0, };

static GtkHBoxClass *parent_class = NULL;

/* The arguments we take */
enum {
	ARG_0,
	ARG_OPTION_CHOICE,
	ARG_TEXT,
};

/* Signals.  */

static void
emit_query_changed (SearchBar *sb)
{
	gtk_signal_emit(GTK_OBJECT (sb), sb_signals [QUERY_CHANGED]);

	gtk_widget_set_sensitive (sb->search_button, FALSE);
}


/* Callbacks.  */

static void
option_activated_cb (GtkWidget *widget,
		     SearchBar *sb)
{
	int id;

	id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "SbChoiceId"));

	sb->option_choice = id;
	
	gtk_widget_set_sensitive (sb->search_button, TRUE);
}

static void
entry_activated_cb (GtkWidget *widget,
		     SearchBar *sb)
{
	emit_query_changed (sb);
}

static void
entry_changed_cb (GtkWidget *widget,
		  SearchBar *sb)
{
	gtk_widget_set_sensitive (sb->search_button, TRUE);
}

static void
clear_button_clicked_cb (GtkWidget *widget,
			  SearchBar *sb)
{
	gchar *text = (gchar *) gtk_entry_get_text (GTK_ENTRY (sb->entry));
	
	if (strcmp (text, "\0") != 0) {
		/* if there is any search in the searchbar, we clear it */
		gtk_entry_set_text (GTK_ENTRY (sb->entry), "");
		emit_query_changed (sb);
	}
	
	gtk_widget_set_sensitive (sb->search_button, FALSE);
	gtk_option_menu_set_history (GTK_OPTION_MENU (sb->option), 0);
}

/* Widgetry creation.  */
static void
set_option(SearchBar *sb, SearchBarItem *items)
{
	GtkWidget *menu;
	GtkRequisition option_requisition;
	int i;

	if (sb->option) {
		gtk_widget_destroy(sb->option_menu);
	} else {
		sb->option = gtk_option_menu_new();
		gtk_widget_show(sb->option);
		gtk_box_pack_start(GTK_BOX(sb), sb->option, FALSE, FALSE, 0);
	}

	sb->option_menu = menu = gtk_menu_new ();
	for (i = 0; items[i].id != -1; i++) {
		GtkWidget *item;

		if (items[i].text)
			item = gtk_menu_item_new_with_label (_(items[i].text));
		else
			item = gtk_menu_item_new();

		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

		g_object_set_data (G_OBJECT (item), "SbChoiceId", GINT_TO_POINTER(items[i].id));

		g_signal_connect (G_OBJECT (item), "activate",
				  G_CALLBACK (option_activated_cb),
				  sb);
	}
	gtk_widget_show_all (menu);

	gtk_option_menu_set_menu (GTK_OPTION_MENU (sb->option), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (sb->option), 0);

	gtk_widget_set_sensitive (sb->option, TRUE);
	gtk_size_group_add_widget (GTK_SIZE_GROUP (sb->size_group), sb->option);
}

static void
add_entry (SearchBar *sb)
{
	sb->entry = gtk_entry_new();
	g_signal_connect (G_OBJECT (sb->entry), "activate",
			  G_CALLBACK (entry_activated_cb), sb);
	g_signal_connect (G_OBJECT (sb->entry), "changed",
			  G_CALLBACK (entry_changed_cb), sb);
	gtk_widget_show(sb->entry);
	gtk_box_pack_start(GTK_BOX(sb), sb->entry, TRUE, TRUE, 0);
	gtk_size_group_add_widget (GTK_SIZE_GROUP (sb->size_group), sb->entry);
}

static void
add_spacer (SearchBar *sb)
{
	GtkWidget *spacer;

	spacer = gtk_drawing_area_new();
	gtk_widget_show(spacer);
	gtk_box_pack_start(GTK_BOX(sb), spacer, FALSE, FALSE, 0);

	gtk_widget_set_usize(spacer, 19, 1);
	gtk_size_group_add_widget (GTK_SIZE_GROUP (sb->size_group), spacer);
}

static void
add_search_button (SearchBar *sb)
{
	sb->search_button = gtk_button_new_with_label (_("Search"));
	g_signal_connect (G_OBJECT (sb->search_button), "clicked",
			  G_CALLBACK (entry_activated_cb), sb);
	gtk_widget_show (sb->search_button);
	gtk_widget_set_sensitive (sb->search_button, FALSE);
	gtk_box_pack_start (GTK_BOX (sb), sb->search_button, FALSE, FALSE, 0);
	gtk_size_group_add_widget (GTK_SIZE_GROUP (sb->size_group), sb->search_button);
}

static void
add_clear_button (SearchBar *sb)
{
	sb->clear_button = gtk_button_new_with_label (_("Clear"));
	g_signal_connect (G_OBJECT (sb->clear_button), "clicked",
			  G_CALLBACK (clear_button_clicked_cb), sb);
	gtk_widget_show (sb->clear_button);
	gtk_box_pack_start (GTK_BOX (sb), sb->clear_button, FALSE, FALSE, 0);
	gtk_size_group_add_widget (GTK_SIZE_GROUP (sb->size_group), sb->clear_button);
}

static int
find_id(GtkWidget *menu, int idin, const char *type, GtkWidget **widget)
{
	GList *l = GTK_MENU_SHELL(menu)->children;
	int row = -1, i = 0, id;

	if (widget)
		*widget = NULL;
	while (l) {
		id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT (l->data), type));
/*		printf("comparing id %d to query %d\n", id, idin);*/
		if (id == idin) {
			row = i;
			if (widget)
				*widget = l->data;
			break;
		}
		i++;
		l = l->next;
	}
	return row;
}

/* GObject methods.  */

static void
impl_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	SearchBar *sb = SEARCH_BAR(object);

	switch (prop_id) {
	case ARG_OPTION_CHOICE:
		g_value_set_uint (value, sb->option_choice);
		break;

	case ARG_TEXT:
		g_value_set_string (value, gtk_entry_get_text (GTK_ENTRY (sb->entry)));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
impl_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	SearchBar *sb = SEARCH_BAR(object);
	int row;

	switch (prop_id) {
	case ARG_OPTION_CHOICE:
		sb->option_choice = g_value_get_uint (value);
		row = find_id (sb->option_menu, sb->option_choice, "SbChoiceId", NULL);
		if (row == -1)
			row = 0;
		gtk_option_menu_set_history (GTK_OPTION_MENU (sb->option), row);
		emit_query_changed (sb);
		break;

	case ARG_TEXT:
		gtk_entry_set_text (GTK_ENTRY (sb->entry), g_value_get_string (value));
		emit_query_changed (sb);
		break;

	default:
		break;
	}
}

static void
impl_destroy (GtkObject *object)
{
	if (GTK_OBJECT_CLASS(parent_class)->destroy)
		GTK_OBJECT_CLASS(parent_class)->destroy (object);
}

static void
class_init (SearchBarClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS(klass);

	parent_class = gtk_type_class (gtk_hbox_get_type ());

	g_object_class->set_property = impl_set_property;
	g_object_class->get_property = impl_get_property;
/*	g_object_class->finalize = impl_finalize;*/

	klass->set_option = set_option;

	g_object_class_install_property (g_object_class,
					 ARG_OPTION_CHOICE,
					 g_param_spec_uint ("option_choice",
							    _("Choice"),
							    "",
							    0,
							    G_MAXUINT,
							    0,
							    G_PARAM_READWRITE));

	g_object_class_install_property (g_object_class,
					 ARG_TEXT,
					 g_param_spec_string ("text",
							      _("Text"),
							      "",
							      "",
							      G_PARAM_READWRITE));

	sb_signals [QUERY_CHANGED] =
		g_signal_new ("query_changed",
			      G_OBJECT_CLASS_TYPE (g_object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (SearchBarClass, query_changed),
			      NULL, NULL,
			      g_cclosure_user_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

static void
init (SearchBar *sb)
{
	sb->option        = NULL;
	sb->entry         = NULL;
	sb->search_button = NULL;
	sb->clear_button  = NULL;

	sb->option_choice = 0;
}

/* Object construction.  */

static void
search_bar_construct (SearchBar *search_bar,
                      SearchBarItem *option_items)
{
	g_return_if_fail (search_bar != NULL);
	g_return_if_fail (IS_SEARCH_BAR (search_bar));
	g_return_if_fail (option_items != NULL);
	
	gtk_box_set_spacing (GTK_BOX (search_bar), 6);
	search_bar->size_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);

	search_bar_set_option(search_bar, option_items);

	add_entry (search_bar);

	add_search_button (search_bar);
	add_clear_button (search_bar);

	add_spacer (search_bar);
}

void
search_bar_set_option(SearchBar *search_bar, SearchBarItem *option_items)
{
	g_return_if_fail (search_bar != NULL);
	g_return_if_fail (IS_SEARCH_BAR (search_bar));
	g_return_if_fail (option_items != NULL);

	((SearchBarClass *)(G_OBJECT_GET_CLASS ((GtkObject *)search_bar)))->set_option(search_bar, option_items);
}

GtkWidget *
search_bar_new (SearchBarItem *option_items)
{
	GtkWidget *widget;

	g_return_val_if_fail (option_items != NULL, NULL);
	
	widget = GTK_WIDGET (g_type_create_instance (search_bar_get_type ()));

	search_bar_construct (SEARCH_BAR (widget), option_items);

	return widget;
}

GtkType
search_bar_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (SearchBarClass),
			NULL, /* base_init */
			NULL, /* base finalize */
			(GClassInitFunc) class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (SearchBar),
			0, /* n_preallocs */
			(GInstanceInitFunc) NULL
		};
		
		type = g_type_register_static (gtk_hbox_get_type (), "SearchBar", &info, 0);
	}

	return type;
}

/* Helper functions */
void
search_bar_clear_search (SearchBar *sb)
{
	g_signal_emit_by_name (G_OBJECT (sb->clear_button), "clicked", NULL);
}
