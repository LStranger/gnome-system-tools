/* boot_append_editor.h: this file is part of boot-admin, a ximian-setup-tool
 * frontend for boot administration.
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
 * Authors: Carlos Garcia Campos <elkalmail@yahoo.es>.
 */
 
#ifndef BOOT_APPEND_EDITOR_H
#define BOOT_APPEND_EDITOR_H

#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-file-entry.h>
#include "boot-append-gui.h"

G_BEGIN_DECLS

#define BOOT_APPEND_EDITOR_TYPE        (boot_append_editor_get_type ())
#define BOOT_APPEND_EDITOR(o)          (GTK_CHECK_CAST ((o), BOOT_APPEND_EDITOR_TYPE, BootAppendEditor))
#define BOOT_APPEND_EDITOR_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BOOT_APPEND_EDITOR_TYPE, BootAppendEditorClass))

typedef struct _BootAppendEditor
{
	   GtkDialog parent;
	   
	   BootAppendGui *gui;
} BootAppendEditor;

typedef struct 
{
	   GtkDialogClass parent_class;
	   
} BootAppendEditorClass;

GtkType boot_append_editor_get_type (void);

BootAppendEditor *boot_append_editor_new (BootSettingsGui *settings);

G_END_DECLS

#endif /* BOOT_APPEND_EDITOR_H */
