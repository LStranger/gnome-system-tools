/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __BOOT_SETTINGS_H
#define __BOOT_SETTINGS_H

#include <gnome.h>
#include <gnome-xml/tree.h>

#include "boot-image.h"



typedef struct {
	GtkWidget *top;
	BootImage *image;
	GladeXML *xml;

	GtkEntry *name;
	GtkCombo *type;

	GtkWidget *device_label;
	GtkCombo  *device;
	GtkWidget *image_label;
	GtkWidget *image_widget;
	GtkEntry  *image_entry;
	GtkWidget *optional;
	GtkEntry *root;
	GtkEntry *append;
} BootSettingsGui;

BootSettingsGui *boot_settings_gui_new      (BootImage *image, GtkWidget *parent);
void             boot_settings_gui_setup    (BootSettingsGui *gui, GtkWidget *top);
gboolean         boot_settings_gui_save     (BootSettingsGui *gui);
void             boot_settings_gui_error    (GtkWindow *parent, gchar *error);
void             boot_settings_gui_destroy  (BootSettingsGui *gui);

/* Callbacks */ 
void             on_boot_settings_clicked   (GtkButton *button, gpointer data);

#endif /* BOOT_SETTINGS_H */
