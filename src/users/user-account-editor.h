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

#ifndef USER_ACCOUNT_EDITOR_H
#define USER_ACCOUNT_EDITOR_H

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-file-entry.h>
#include "user_settings.h"

#define USER_ACCOUNT_EDITOR_TYPE        (user_account_editor_get_type ())
#define USER_ACCOUNT_EDITOR(o)          (GTK_CHECK_CAST ((o), USER_ACCOUNT_EDITOR_TYPE, UserAccountEditor))
#define USER_ACCOUNT_EDITOR_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), USER_ACCOUNT_EDITOR_TYPE, UserAccountEditorClass))
#define USER_IS_ACCOUNT_EDITOR(o)       (GTK_CHECK_TYPE ((o), USER_ACCOUNT_EDITOR_TYPE))
#define USER_IS_ACCOUNT_EDITOR_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), USER_ACCOUNT_EDITOR_TYPE))

struct _UserAccountEditor {
	GnomeDialog parent;
	
	UserAccountGui *gui;
	GtkNotebook *notebook;
};

typedef struct _UserAccountEditor UserAccountEditor;

typedef struct {
	GnomeDialogClass parent_class;
	
	/* signals */
	
} UserAccountEditorClass;

GtkType user_account_editor_get_type (void);

UserAccountEditor *user_account_editor_new (UserAccount *account);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* USER_ACCOUNT_EDITOR_H */
