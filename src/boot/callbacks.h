/* callbacks.h: this file is part of boot-admin, a ximian-setup-tool frontend 
 * for boot administration.
 * 
 * Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Tambet Ingo <tambet@ximian.com>.
 */

#ifndef __CALLBACKS_H
#define __CALLBACKS_H

#include <gnome.h>
#include <gnome-xml/tree.h>

extern void on_boot_delete_clicked (GtkButton *button, gpointer user_data);
extern void on_boot_settings_clicked (GtkButton *button, gpointer user_data);
extern void on_boot_add_clicked (GtkButton *button, gpointer user_data);
extern void on_boot_prompt_toggled (GtkToggleButton *toggle, gpointer user_data);

void actions_set_sensitive (gboolean state);

#endif /* CALLBACKS_H */

