/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* dropdown-menu.h
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
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
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Carlos Garnacho Parro <garparr@teleline.es>
 *
 * Based on code by: Ettore Perazzoli
 */

#ifndef _DROPDOWN_BUTTON_H_
#define _DROPDOWN_BUTTON_H_

#ifdef HAVE_CONFIG_H
   #include <config.h>
#endif

#include <gtk/gtktogglebutton.h>
#include <gtk/gtkmenu.h>

#include <gnome.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define TYPE_DROPDOWN_BUTTON		(dropdown_button_get_type ())
#define DROPDOWN_BUTTON(obj)		(GTK_CHECK_CAST ((obj), TYPE_DROPDOWN_BUTTON, DropdownButton))
#define DROPDOWN_BUTTON_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), TYPE_DROPDOWN_BUTTON, DropdownButtonClass))
#define IS_DROPDOWN_BUTTON(obj)	  	(GTK_CHECK_TYPE ((obj), TYPE_DROPDOWN_BUTTON))
#define IS_DROPDOWN_BUTTON_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((obj), TYPE_DROPDOWN_BUTTON))

typedef struct _DropdownButton        DropdownButton;
typedef struct _DropdownButtonPrivate DropdownButtonPrivate;
typedef struct _DropdownButtonClass   DropdownButtonClass;

struct _DropdownButton
{
	GtkToggleButton parent;
	DropdownButtonPrivate *priv;
};

struct _DropdownButtonClass {
	GtkToggleButtonClass parent_class;
};

GtkType    dropdown_button_get_type   (void);
void       dropdown_button_construct  (DropdownButton *dropdown_button, const char *label_text, GtkMenu *menu);
GtkWidget *dropdown_button_new        (const char *label_text, GtkMenu *menu);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _E_DROPDOWN_BUTTON_H_ */
