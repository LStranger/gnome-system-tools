/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * e-search-bar.c
 *
 * Copyright (C) 2000, 2001 Ximian, Inc.
 *
 * Authors:
 *  Chris Lahey <clahey@ximian.com>
 *  Ettore Perazzoli <ettore@ximian.com>
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
#include "dropdown-button.h"

enum {
	QUERY_CHANGED,
	MENU_ACTIVATED,

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
}

static void
emit_menu_activated (SearchBar *sb, int item)
{
	gtk_signal_emit(GTK_OBJECT (sb), sb_signals [MENU_ACTIVATED], item);
}

/* Callbacks.  */

static void
menubar_activated_cb (GtkWidget *widget, SearchBar *sb)
{
	int id;

	id = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (widget), "EsbMenuId"));

	emit_menu_activated(sb, id);
}

static void
option_activated_cb (GtkWidget *widget,
		     SearchBar *sb)
{
	int id;

	id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "EsbChoiceId"));

	sb->option_choice = id;
	emit_query_changed (sb);
}

static void
entry_activated_cb (GtkWidget *widget,
		     SearchBar *esb)
{
	emit_query_changed (esb);
}

/* Widgetry creation.  */

static void add_dropdown(SearchBar *esb, SearchBarItem *items)
{
	GtkWidget *menu = esb->dropdown_menu;
	GtkWidget *item;

	if (items->text)
		item = gtk_menu_item_new_with_label (_(items->text));
	else
		item = gtk_menu_item_new();

	gtk_widget_show(item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_object_set_data (G_OBJECT (item), "EsbMenuId", GINT_TO_POINTER(items->id));
	g_signal_connect (G_OBJECT (item), "activate",
			  G_CALLBACK (menubar_activated_cb),
			  esb);
}

static void
set_dropdown (SearchBar *sb,
	      SearchBarItem *items)
{
	GtkWidget *menu;
	GtkWidget *dropdown;
	int i;

	menu = sb->dropdown_menu = gtk_menu_new ();
	for (i = 0; items[i].id != -1; i++)
		add_dropdown(sb, items+i);

	gtk_widget_show_all (menu);

	dropdown = dropdown_button_new (_("Sear_ch"), GTK_MENU (menu));
	GTK_WIDGET_UNSET_FLAGS (dropdown, GTK_CAN_FOCUS);
	gtk_widget_show (dropdown);

	if (sb->dropdown_holder == NULL) {

		/* So, GtkOptionMenu is stupid; it adds a 1-pixel-wide empty border
	           around the button for no reason.  So we add a 1-pixel-wide border
	           around the button as well, by using an event box.  */

		sb->dropdown_holder = gtk_event_box_new ();
		gtk_container_set_border_width (GTK_CONTAINER (sb->dropdown_holder), 1);
		sb->dropdown = dropdown;
		gtk_container_add (GTK_CONTAINER (sb->dropdown_holder), sb->dropdown);
		gtk_widget_show (sb->dropdown_holder);

		gtk_box_pack_start(GTK_BOX(sb), sb->dropdown_holder, FALSE, FALSE, 0);
	} else {
		gtk_widget_destroy(sb->dropdown);
		sb->dropdown = dropdown;
		gtk_container_add (GTK_CONTAINER (sb->dropdown_holder), sb->dropdown);
	}
}

static void
set_option(SearchBar *esb, SearchBarItem *items)
{
	GtkWidget *menu;
	GtkRequisition dropdown_requisition;
	GtkRequisition option_requisition;
	int i;

	if (esb->option) {
		gtk_widget_destroy(esb->option_menu);
	} else {
		esb->option = gtk_option_menu_new();
		gtk_widget_show(esb->option);
		gtk_box_pack_start(GTK_BOX(esb), esb->option, FALSE, FALSE, 0);
	}

	esb->option_menu = menu = gtk_menu_new ();
	for (i = 0; items[i].id != -1; i++) {
		GtkWidget *item;

		if (items[i].text)
			item = gtk_menu_item_new_with_label (_(items[i].text));
		else
			item = gtk_menu_item_new();

		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

		g_object_set_data (G_OBJECT (item), "EsbChoiceId", GINT_TO_POINTER(items[i].id));

		g_signal_connect (G_OBJECT (item), "activate",
				  G_CALLBACK (option_activated_cb),
				  esb);
	}
	gtk_widget_show_all (menu);

	gtk_option_menu_set_menu (GTK_OPTION_MENU (esb->option), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (esb->option), 0);

	gtk_widget_set_sensitive (esb->option, TRUE);

	/* Set the minimum height of this widget to that of the dropdown
           button, for a better look.  */
	g_assert (esb->dropdown != NULL);

	gtk_widget_size_request (esb->dropdown, &dropdown_requisition);
	gtk_widget_size_request (esb->option, &option_requisition);

	gtk_container_set_border_width (GTK_CONTAINER (esb->dropdown), GTK_CONTAINER (esb->option)->border_width);
}

static void
add_entry (SearchBar *esb)
{
	esb->entry = gtk_entry_new();
	g_signal_connect (G_OBJECT (esb->entry), "activate",
			  G_CALLBACK (entry_activated_cb), esb);
	gtk_widget_show(esb->entry);
	gtk_box_pack_start(GTK_BOX(esb), esb->entry, TRUE, TRUE, 0);
}

static void
add_spacer (SearchBar *esb)
{
	GtkWidget *spacer;

	spacer = gtk_drawing_area_new();
	gtk_widget_show(spacer);
	gtk_box_pack_start(GTK_BOX(esb), spacer, FALSE, FALSE, 0);

	gtk_widget_set_usize(spacer, 19, 1);
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
		printf("comparing id %d to query %d\n", id, idin);
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

/* GtkObject methods.  */

static void
impl_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	SearchBar *sb = SEARCH_BAR(object);

	switch (arg_id) {
	case ARG_OPTION_CHOICE:
//		G_VALUE_HOLDS_ENUM (*arg) = sb->option_choice;
		break;

	case ARG_TEXT:
//		G_VALUE_HOLDS_STRING (*arg) = gtk_entry_get_text (GTK_ENTRY (sb->entry));
		break;

	default:
//		arg->type = G_TYPE_INVALID;
		break;
	}
}

static void
impl_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	SearchBar *esb = SEARCH_BAR(object);
	int row;

	switch (arg_id) {
	case ARG_OPTION_CHOICE:
//		esb->option_choice = G_VALUE_HOLDS_ENUM(*arg);
		row = find_id(esb->option_menu, esb->option_choice, "EsbChoiceId", NULL);
		if (row == -1)
			row = 0;
		gtk_option_menu_set_history (GTK_OPTION_MENU (esb->option), row);
		emit_query_changed (esb);
		break;

	case ARG_TEXT:
//		e_utf8_gtk_editable_set_text(GTK_EDITABLE(esb->entry), G_VALUE_HOLDS_STRING (*arg));
		emit_query_changed (esb);
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
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS(klass);

	parent_class = gtk_type_class (gtk_hbox_get_type ());

	object_class->set_arg = impl_set_arg;
	object_class->get_arg = impl_get_arg;
	object_class->destroy = impl_destroy;

	klass->set_menu = set_dropdown;
	klass->set_option = set_option;

/*	gtk_object_add_arg_type ("SearchBar::option_choice", G_VALUE_HOLDS_ENUM,
				 GTK_ARG_READWRITE, ARG_OPTION_CHOICE);
	gtk_object_add_arg_type ("SearchBar::text", G_VALUE_HOLDS_STRING,
				 GTK_ARG_READWRITE, ARG_TEXT);

	esb_signals [QUERY_CHANGED] =
		gtk_signal_new ("query_changed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (ESearchBarClass, query_changed),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);
*/
/*	esb_signals [MENU_ACTIVATED] =
		gtk_signal_new ("menu_activated",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (ESearchBarClass, menu_activated),
				gtk_marshal_NONE__INT,
				GTK_TYPE_NONE, 1, GTK_TYPE_INT);
*/
//	gtk_object_class_add_signals (object_class, esb_signals, LAST_SIGNAL);
}

static void
init (SearchBar *sb)
{
	sb->dropdown      = NULL;
	sb->option        = NULL;
	sb->entry         = NULL;

	sb->option_choice = 0;
}

/* Object construction.  */

void
search_bar_construct (SearchBar *search_bar,
                      SearchBarItem *menu_items,
                      SearchBarItem *option_items)
{
	g_return_if_fail (search_bar != NULL);
	g_return_if_fail (IS_SEARCH_BAR (search_bar));
	g_return_if_fail (menu_items != NULL);
	g_return_if_fail (option_items != NULL);
	
	gtk_box_set_spacing (GTK_BOX (search_bar), 1);

	search_bar_set_menu(search_bar, menu_items);

	search_bar_set_option(search_bar, option_items);

	add_entry (search_bar);

	add_spacer (search_bar);
}

void
search_bar_set_menu(SearchBar *search_bar, SearchBarItem *menu_items)
{
	g_return_if_fail (search_bar != NULL);
	g_return_if_fail (IS_SEARCH_BAR (search_bar));
	g_return_if_fail (menu_items != NULL);

	((SearchBarClass *)(G_OBJECT_GET_CLASS ((GtkObject *)search_bar)))->set_menu(search_bar, menu_items);
}

void
search_bar_add_menu(SearchBar *search_bar, SearchBarItem *menu_item)
{
	g_return_if_fail (search_bar != NULL);
	g_return_if_fail (IS_SEARCH_BAR (search_bar));
	g_return_if_fail (menu_item != NULL);

	add_dropdown(search_bar, menu_item);
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
search_bar_new (SearchBarItem *menu_items,
		  SearchBarItem *option_items)
{
	GtkWidget *widget;

	g_return_val_if_fail (menu_items != NULL, NULL);
	g_return_val_if_fail (option_items != NULL, NULL);
	
	widget = GTK_WIDGET (g_type_create_instance (search_bar_get_type ()));

	search_bar_construct (SEARCH_BAR (widget), menu_items, option_items);

	return widget;
}

void
search_bar_set_menu_sensitive(SearchBar *esb, int id, gboolean state)
{
	int row;
	GtkWidget *widget;

	row = find_id(esb->dropdown_menu, id, "EsbMenuId", &widget);
	if (row != -1)
		gtk_widget_set_sensitive(widget, state);
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

