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

#include <string.h>

#include <glib/gi18n.h>

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
	g_signal_emit(G_OBJECT(sb), sb_signals [QUERY_CHANGED], 0);
}


/* Callbacks.  */

static void
option_activated_cb (GtkWidget *widget,
		     SearchBar *sb)
{
	sb->option_choice = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	emit_query_changed (sb);
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
	emit_query_changed (sb);

	if (strlen (gtk_entry_get_text (GTK_ENTRY (widget))) > 0)
		gtk_widget_set_sensitive (sb->clear_button, TRUE);
	else
		gtk_widget_set_sensitive (sb->clear_button, FALSE);
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
	
	gtk_combo_box_set_active (GTK_COMBO_BOX (sb->option), 0);
}

/* Widgetry creation.  */
static void
set_option(SearchBar *sb, SearchBarItem *items)
{
	GtkWidget       *menu;
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	GtkCellRenderer *renderer;
	int i;

	if (!sb->option) {
		model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT));
		renderer = gtk_cell_renderer_text_new ();
		
		sb->option = gtk_combo_box_new_with_model (model);
		g_object_unref (model);

		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (sb->option),
					    renderer, TRUE);
		gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (sb->option),
					       renderer,
					       "text", 0);
		gtk_box_pack_start(GTK_BOX(sb), sb->option, FALSE, FALSE, 0);
	}

	for (i = 0; items[i].id != -1; i++) {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model),
				    &iter,
				    0, _(items[i].text),
				    1, items[i].id,
				    -1);
	}

	gtk_widget_show(sb->option);
	gtk_combo_box_set_active (GTK_COMBO_BOX (sb->option), 0);

	g_signal_connect (G_OBJECT (sb->option), "changed",
			  G_CALLBACK (option_activated_cb), sb);

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

/*
static void
add_search_button (SearchBar *sb)
{
	GtkWidget *search_button_label, *search_button_image,
	*search_button_hbox, *search_button_hbox_align;

	/* default i18nized stock button would certainly conflict with other
	buttons so we create our own*/
/*	sb->search_button = gtk_button_new ();

	search_button_hbox_align = gtk_alignment_new (0.50, 0.50, 1.0, 1.0);
	gtk_container_add (GTK_CONTAINER (GTK_BUTTON (sb->search_button)),
	GTK_WIDGET (GTK_ALIGNMENT (search_button_hbox_align)));

	search_button_hbox = gtk_hbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (GTK_ALIGNMENT	(
	search_button_hbox_align)), GTK_WIDGET (GTK_HBOX (
	search_button_hbox)));

	search_button_image = gtk_image_new_from_stock (GTK_STOCK_FIND,
						        GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start (GTK_BOX (GTK_HBOX (search_button_hbox)),
			    GTK_WIDGET (GTK_IMAGE (search_button_image)),
			    FALSE, FALSE, 0);

	search_button_label = gtk_label_new_with_mnemonic (_("_Search"));
	gtk_box_pack_start (GTK_BOX (GTK_HBOX (search_button_hbox)),
			    GTK_WIDGET (GTK_LABEL (search_button_label)),
			    FALSE, FALSE, 0);

	g_signal_connect (G_OBJECT (sb->search_button), "clicked",
			  G_CALLBACK (entry_activated_cb), sb);
	gtk_widget_show_all (sb->search_button);
	gtk_widget_set_sensitive (sb->search_button, FALSE);
	gtk_box_pack_start (GTK_BOX (sb), sb->search_button, FALSE, FALSE, 0);
	gtk_size_group_add_widget (GTK_SIZE_GROUP (sb->size_group), sb->search_button);
}
*/

static void
add_clear_button (SearchBar *sb)
{
	GtkWidget *clear_button_label, *clear_button_image, *clear_button_hbox,
	*clear_button_hbox_align;

	/* default i18nized stock button would certainly conflict with other
	buttons so we create our own*/
	sb->clear_button = gtk_button_new ();

	clear_button_hbox_align = gtk_alignment_new (0.50, 0.50, 1.0, 1.0);
	gtk_container_add (GTK_CONTAINER (GTK_BUTTON (sb->clear_button)),
	GTK_WIDGET (GTK_ALIGNMENT (clear_button_hbox_align)));

	clear_button_hbox = gtk_hbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (GTK_ALIGNMENT	(
	clear_button_hbox_align)), GTK_WIDGET (GTK_HBOX (
	clear_button_hbox)));

	clear_button_image = gtk_image_new_from_stock (GTK_STOCK_CLEAR,
						       GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start (GTK_BOX (GTK_HBOX (clear_button_hbox)),
			    GTK_WIDGET (GTK_IMAGE (clear_button_image)),
			    FALSE, FALSE, 0);

	clear_button_label = gtk_label_new_with_mnemonic (_("C_lear"));
	gtk_box_pack_start (GTK_BOX (GTK_HBOX (clear_button_hbox)),
			    GTK_WIDGET (GTK_LABEL (clear_button_label)),
			    FALSE, FALSE, 0);

	g_signal_connect (G_OBJECT (sb->clear_button), "clicked",
			  G_CALLBACK (clear_button_clicked_cb), sb);
	gtk_widget_show_all (sb->clear_button);
	gtk_box_pack_start (GTK_BOX (sb), sb->clear_button, FALSE, FALSE, 0);
	gtk_size_group_add_widget (GTK_SIZE_GROUP (sb->size_group), sb->clear_button);
}

static int
find_id (GtkWidget *menu)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gint          id = 0;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (menu));

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (menu), &iter))
		gtk_tree_model_get (model, &iter, 1, &iter, -1);

	return id;
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
		row = find_id (sb->option);

		if (row == -1)
			row = 0;
		gtk_combo_box_set_active (GTK_COMBO_BOX (sb->option), row);
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

	parent_class = g_type_class_peek (gtk_hbox_get_type ());

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

	add_clear_button (search_bar);

	gtk_widget_set_sensitive (search_bar->clear_button, FALSE);
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

GType
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
