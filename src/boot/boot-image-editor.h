/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Tambet Ingo <tambet@ximian.com>
 *
 *  Copyright 2001 Ximian, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef BOOT_IMAGE_EDITOR_H
#define BOOT_IMAGE_EDITOR_H

#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-file-entry.h>
#include "boot-settings.h"

G_BEGIN_DECLS

#define BOOT_IMAGE_EDITOR_TYPE        (boot_image_editor_get_type ())
#define BOOT_IMAGE_EDITOR(o)          (GTK_CHECK_CAST ((o), BOOT_IMAGE_EDITOR_TYPE, BootImageEditor))
#define BOOT_IMAGE_EDITOR_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BOOT_IMAGE_EDITOR_TYPE, BootImageEditorClass))
#define USER_IS_ACCOUNT_EDITOR(o)       (GTK_CHECK_TYPE ((o), BOOT_IMAGE_EDITOR_TYPE))
#define USER_IS_ACCOUNT_EDITOR_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BOOT_IMAGE_EDITOR_TYPE))

struct _BootImageEditor {
	GtkDialog parent;

	BootSettingsGui *gui;
};

typedef struct _BootImageEditor BootImageEditor;

typedef struct {
	GtkDialogClass parent_class;
	
	/* signals */
	
} BootImageEditorClass;

GtkType boot_image_editor_get_type (void);

BootImageEditor *boot_image_editor_new (BootImage *image);

G_END_DECLS

#endif /* BOOT_IMAGE_EDITOR_H */
