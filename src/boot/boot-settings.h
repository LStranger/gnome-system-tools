/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __BOOT_SETTINGS_H
#define __BOOT_SETTINGS_H

#include <gnome.h>
#include <gnome-xml/tree.h>

#include "boot-image.h"

typedef struct {
	GtkWidget *top;
	BootImage *image;
	GladeXML  *xml;

	/* Basic frame */
	GtkWidget *basic_frame;
	GtkEntry  *name;
	GtkCombo  *type;

	/* Image frame */
	GtkWidget *image_frame;
	GtkWidget *image_widget;
	GtkEntry  *image_entry;
	GtkEntry  *root;
	GtkEntry  *append;

	/* Other frame */
	GtkWidget *other_frame;
	GtkCombo  *device;
	
} BootSettingsGui;

BootSettingsGui *boot_settings_gui_new      (BootImage *image, GtkWidget *parent);
void             boot_settings_gui_setup    (BootSettingsGui *gui, GtkWidget *top);
gboolean         boot_settings_gui_save     (BootSettingsGui *gui, gboolean check);
void             boot_settings_gui_error    (GtkWindow *parent, gchar *error);
void             boot_settings_gui_destroy  (BootSettingsGui *gui);

/* Callbacks */ 

void             on_boot_add_clicked        (GtkButton *button, gpointer data);
void             on_boot_settings_clicked   (GtkButton *button, gpointer data);

#endif /* BOOT_SETTINGS_H */
