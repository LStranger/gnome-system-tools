/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __BOOT_SETTINGS_H
#define __BOOT_SETTINGS_H

#include <gnome.h>

#include "boot-image.h"

typedef struct {
	GtkWidget *top;
	BootImage *image;
	GladeXML  *xml;

	/* Basic frame */
	GtkWidget *basic_frame;
	GtkEntry  *name;
	GtkWidget *type;

	/* Image frame */
	GtkWidget *image_frame;
	GtkWidget *image_widget;
	GtkEntry  *image_entry;
	GtkWidget *root;
	GtkWidget *initrd_label;
	GtkWidget *initrd_widget;
	GtkEntry  *initrd_entry;
	GtkEntry  *append;
	GtkButton *append_browse;

	/* Security Frame */
	GtkCheckButton *use_password;
	GtkLabel *pass_label;
	GtkLabel *confirm_label;
	GtkEntry *password;
	GtkEntry *password_confirm;
	
	/* Other frame */
	GtkWidget *other_frame;
	GtkWidget *device;
	
} BootSettingsGui;

typedef struct _BootImageEditor
{
	GtkDialog *dialog;

	BootSettingsGui *gui;
} BootImageEditor;

BootSettingsGui *boot_settings_gui_new        (BootImage *image, GtkWidget *parent);
void             boot_settings_gui_setup      (BootSettingsGui *gui, GtkWidget *top);
gboolean         boot_settings_gui_save       (BootSettingsGui *gui, gboolean check);
void             boot_settings_gui_error      (GtkWindow *parent, gchar *error);
void             boot_settings_gui_destroy    (BootSettingsGui *gui);

void             boot_settings_fill_type_list (BootSettingsGui*);
const gchar*     boot_settings_get_type       (BootSettingsGui*);
void             boot_settings_set_type       (BootSettingsGui*, gchar*);

#endif /* BOOT_SETTINGS_H */
