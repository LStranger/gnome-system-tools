/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Tambet Ingo <tambet@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "xst-ui.h"


GtkWidget *
xst_ui_list_get_list_item_by_name (GtkList *list, const gchar *label)
{
	GList *items, *children;
	GtkWidget *listitem;
	gchar *buf;
	GtkWidget *child;
	
	g_return_val_if_fail (list != NULL, NULL);
	g_return_val_if_fail (GTK_IS_LIST (list), NULL);

	items = gtk_container_children (GTK_CONTAINER (list));

	while (items)
	{
		listitem = items->data;
		items = items->next;

		children = gtk_container_children (GTK_CONTAINER (listitem));

		while (children)
		{
			child    = children->data;
			children = children->next;

			if (GTK_IS_LABEL (child))
			{
				gtk_label_get (GTK_LABEL (child), &buf);
				if (strcmp (buf, label))
					continue;

				/* Found */
				return listitem;
			}
		}
	}

	return NULL;
}

void
xst_ui_combo_remove_by_label (GtkCombo *combo, const gchar *label)
{
	GtkWidget *item;
	gchar *buf;
	
	g_return_if_fail (combo != NULL);
	g_return_if_fail (GTK_IS_COMBO (combo));

	if (!label)
		buf = gtk_entry_get_text (GTK_ENTRY (combo->entry));
	else
		buf = (void *)label;

	item = xst_ui_list_get_list_item_by_name (GTK_LIST (combo->list),
									  buf);
	if (item)
		gtk_widget_destroy (item);
}
