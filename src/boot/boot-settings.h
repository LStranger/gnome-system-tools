#ifndef __BOOT_SETTINGS_H
#define __BOOT_SETTINGS_H

#include <gnome.h>
#include <gnome-xml/tree.h>

typedef struct {
	GtkWidget *dialog;
	GtkEntry *basic_name;
	GtkCombo *type;

	GtkWidget *settings;
	GtkEntry *adv_name;
	GtkWidget *device_label;
	GtkCombo *device;
	GtkWidget *image_label;
	GtkWidget *image;
	GtkEntry *image_entry;

	GtkWidget *optional;
	GtkEntry *root;

	XstDialogComplexity complexity;
} BootSettingsDialog;

BootSettingsDialog *boot_settings_prepare (xmlNodePtr node);
void boot_settings_affect (BootSettingsDialog *state);

extern void on_boot_settings_clicked (GtkButton *button, gpointer user_data);

#endif /* BOOT_SETTINGS_H */
