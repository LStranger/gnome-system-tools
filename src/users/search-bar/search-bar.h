/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* search-bar.h
 * Copyright (C) 2000  Helix Code, Inc.
 * Author: Chris Lahey <clahey@helixcode.com>
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
#ifndef __SEARCH_BAR_H__
#define __SEARCH_BAR_H__


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#include <gtk/gtk.h>

#define SEARCH_BAR_TYPE			(search_bar_get_type ())
#define SEARCH_BAR(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), SEARCH_BAR_TYPE, SearchBar))
#define SEARCH_BAR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), SEARCH_BAR_TYPE, SearchBarClass))
#define IS_SEARCH_BAR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), SEARCH_BAR_TYPE))
#define IS_SEARCH_BAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), SEARCH_BAR_TYPE))

typedef struct {
	char *text;
	int id;
} SearchBarItem;

typedef struct _SearchBar       SearchBar;
typedef struct _SearchBarClass  SearchBarClass;

struct _SearchBar
{
	GtkHBox parent;
	
	/* item specific fields */
	GtkWidget *option;
	GtkWidget *entry;
	GtkWidget *clear_button;

	/* PRIVATE */
	GtkSizeGroup *size_group;

	int        option_choice;
};

struct _SearchBarClass
{
	GtkHBoxClass parent_class;

	void (*set_menu)       (SearchBar *, SearchBarItem *);
	void (*set_option)     (SearchBar *, SearchBarItem *);

	void (*query_changed)  (SearchBar *search);
	void (*menu_activated) (SearchBar *search, int item);
};

GType    search_bar_get_type   (void);

void       search_bar_clear_search (SearchBar *search_bar);
void       search_bar_set_option   (SearchBar *search_bar, SearchBarItem *option_items);

GtkWidget *search_bar_new          (SearchBarItem *option_items);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __E_SEARCH_BAR_H__ */
