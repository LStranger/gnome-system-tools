/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2001 Ximian, Inc.
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
 *
 */

#include <config.h>

#include <string.h>
#include <ctype.h>

#include "boot-druid.h"
#include "boot-settings.h"

extern GstTool *tool;

static void boot_druid_class_init (BootDruidClass *class);
static void boot_druid_finalize   (GObject *obj);

static GtkWindowClass *parent_class;

GtkType
boot_druid_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		static const GTypeInfo type_info = {
			sizeof (BootDruidClass),
			NULL, /* base_init */
			NULL, /* base finalize */
			(GClassInitFunc) boot_druid_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (BootDruid),
			0, /* n_preallocs */
			(GInstanceInitFunc) NULL
		};
		
		type = g_type_register_static (gtk_window_get_type (), "BootDruid", &type_info, 0);
	}

	return type;
}

static void
boot_druid_class_init (BootDruidClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	parent_class = gtk_type_class (gtk_window_get_type ());

	/* override methods */
	object_class->finalize = boot_druid_finalize;
}

static void
boot_druid_finalize (GObject *obj)
{
	BootDruid *druid = (BootDruid *) obj;

	boot_settings_gui_destroy (druid->gui);

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
druid_exit (BootDruid *druid)
{
	gtk_widget_destroy (GTK_WIDGET (druid));
}

static void
druid_cancel (GtkWidget *w, gpointer data)
{
	druid_exit ((BootDruid *) data);
}

static void
druid_show_error (BootDruid *druid, gchar *error)
{
	GtkWidget *d;

	d = gtk_message_dialog_new (GTK_WINDOW (druid),
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, error);

	g_free (error);
	gtk_dialog_run (GTK_DIALOG (d));
	gtk_widget_destroy (d);
}

/* Identity Page */
static void
identity_check (BootDruid *druid)
{
	const gchar *label = gtk_entry_get_text (druid->gui->name);
	gboolean enabled = (strlen (label) > 0)? TRUE: FALSE;
	
	gnome_druid_set_buttons_sensitive (druid->druid, TRUE, enabled, TRUE, FALSE);
}

static void
identity_changed (GtkWidget *widget, gpointer data)
{
	BootDruid *druid = data;

	identity_check (druid);
}

static void
identity_prepare (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	BootDruid *config = data;
	
	gtk_widget_grab_focus (GTK_WIDGET (config->gui->name));
	g_signal_stop_emission_by_name (page, "prepare");
	identity_check (config);
}

static gboolean
identity_next (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	GnomeDruidPage   *next_page;
	GstBootImageType  type;
	const gchar      *buf;
	gchar            *error;
	BootDruid        *config = data;

	boot_settings_gui_save (config->gui, FALSE);
	
	error = boot_image_valid_label (config->gui->image);
	if (error != NULL) {
		druid_show_error (config, error);
		identity_prepare (page, druid, data);
		return TRUE;
	}

	buf = gtk_entry_get_text (GTK_ENTRY (config->gui->type->entry));
	type = label_to_type (buf);

	if (type == TYPE_LINUX)
	{
		gst_ui_entry_set_text (config->gui->root->entry, "");
		next_page = GNOME_DRUID_PAGE (glade_xml_get_widget (config->gui->xml,
								    "druidImagePage"));
	}
	else
	{
		gst_ui_entry_set_text (config->gui->device->entry, "");
		next_page = GNOME_DRUID_PAGE (glade_xml_get_widget (config->gui->xml,
								    "druidOtherPage"));
	}
	
	gnome_druid_set_page (druid, next_page);
	return TRUE;
}

/* Other Page */
static void
other_check (BootDruid *druid)
{
	const gchar *buf = gtk_entry_get_text (GTK_ENTRY (druid->gui->device->entry));
	gboolean enabled = (strlen (buf) > 0)? TRUE: FALSE;

	/* TODO: Improve check */ 
	gnome_druid_set_buttons_sensitive (druid->druid, TRUE, enabled, TRUE, FALSE);
}

static void
other_changed (GtkWidget *widget, gpointer data)
{
	BootDruid *druid = data;
	
	other_check (druid);
}

static void
other_prepare (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	BootDruid *config = data;

	gtk_widget_grab_focus (config->gui->device->entry);
	g_signal_stop_emission_by_name (page, "prepare");
	other_check (config);
}

static gboolean
other_next (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	GnomeDruidPage *next_page;
	gchar          *error;
	BootDruid      *config = data;

	boot_settings_gui_save (config->gui, FALSE);
	
	error = boot_image_valid_device (config->gui->image);
	if (error != NULL) {
		druid_show_error (config, error);
/*		other_prepare (page, druid, data); */
		return TRUE;
	}

	next_page = GNOME_DRUID_PAGE (glade_xml_get_widget (config->gui->xml, "druidFinishPage"));	
	gnome_druid_set_page (druid, next_page);

	return TRUE;
}

/* Image Page */
static void
image_check (BootDruid *druid)
{
	const gchar *buf = gtk_entry_get_text (druid->gui->image_entry);
	const gchar *buf2 = gtk_entry_get_text (GTK_ENTRY (druid->gui->root->entry));
	gboolean enabled = ((strlen (buf) > 0) && (strlen (buf2) > 0))?TRUE : FALSE;
	
	/* TODO: Improve check */ 
	gnome_druid_set_buttons_sensitive (druid->druid, TRUE, enabled, TRUE, FALSE);
}

static void
image_changed (GtkWidget *widget, gpointer data)
{
	BootDruid *druid = data;

	image_check (druid);
}

static void
image_prepare (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	BootDruid *config = data;

	gtk_widget_grab_focus (GTK_WIDGET (config->gui->image_entry));
	g_signal_stop_emission_by_name (page, "prepare");
	image_check (config);
}

static gboolean
image_next (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	gchar     *error;
	BootDruid *config = data;
	
	boot_settings_gui_save (config->gui, FALSE);
	
	error = boot_image_valid_device (config->gui->image);
	if (error != NULL) {
		druid_show_error (config, error);
/*		image_prepare (page, druid, data);*/
		return TRUE;
	}
	
	error = boot_image_valid_root (config->gui->image);
	if (error != NULL) {
		druid_show_error (config, error);
		gtk_widget_grab_focus (GTK_WIDGET (config->gui->root));
/*		identity_prepare (page, druid, data);*/
		return TRUE;
	}
	
	return FALSE;
}

static gboolean
image_back (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	GnomeDruidPage *next_page;
	BootDruid      *config = data;

	next_page = GNOME_DRUID_PAGE (glade_xml_get_widget (config->gui->xml, "druidIdentityPage"));	
	gnome_druid_set_page (druid, next_page);

	return TRUE;
}

/* Common stuff */
static void
druid_entry_activate (GtkWidget *w, gpointer data)
{
	GtkWidget *widget = NULL;
	BootDruid *druid = data;
	
	if (GTK_WIDGET_MAPPED (druid->druid->next))
		widget = druid->druid->next;
	if (GTK_WIDGET_MAPPED (druid->druid->finish))
		widget = druid->druid->finish;

	if (widget)
		gtk_widget_grab_focus (widget);
}

static void
druid_finish (GnomeDruidPage *druid_page, GnomeDruid *druid, gpointer data)
{
	BootDruid *config = data;

	boot_image_save (config->gui->image);
	boot_settings_gui_save (config->gui, TRUE);
	druid_exit (config);
}

static gboolean
druid_finish_back (GnomeDruidPage *druid_page, GnomeDruid *druid, gpointer data)
{	
	GnomeDruidPage   *next_page;
	GstBootImageType  type;
	const gchar      *buf;
	BootDruid        *config = data;

	buf = gtk_entry_get_text (GTK_ENTRY (config->gui->type->entry));
	type = label_to_type (buf);

	if (type == TYPE_LINUX)
		next_page = GNOME_DRUID_PAGE (glade_xml_get_widget (config->gui->xml,
								    "druidImagePage"));
	else
		next_page = GNOME_DRUID_PAGE (glade_xml_get_widget (config->gui->xml,
								    "druidOtherPage"));

	gnome_druid_set_page (druid, next_page);
	return TRUE;
}

static struct {
	gchar         *name;
	GCallback  next_func;
	GCallback  prepare_func;
	GCallback  back_func;
	GCallback  finish_func;
} pages[] = {
	{ "druidStartPage",
	  G_CALLBACK (NULL),
	  G_CALLBACK (NULL),
	  G_CALLBACK (NULL),
	  G_CALLBACK (NULL) },
	{ "druidIdentityPage",
	  G_CALLBACK (identity_next),
	  G_CALLBACK (identity_prepare),
	  G_CALLBACK (NULL),
	  G_CALLBACK (NULL) },
	{ "druidOtherPage",
	  G_CALLBACK (other_next),
	  G_CALLBACK (other_prepare),
	  G_CALLBACK (NULL),
	  G_CALLBACK (NULL) },
	{ "druidImagePage",
	  G_CALLBACK (image_next),
	  G_CALLBACK (image_prepare),
	  G_CALLBACK (image_back),
	  G_CALLBACK (NULL) },
	{ "druidFinishPage",
	  G_CALLBACK (NULL),
	  G_CALLBACK (NULL),
	  G_CALLBACK (druid_finish_back),
	  G_CALLBACK (druid_finish) },
	NULL
};

static gboolean
construct (BootDruid *druid)
{
	GtkWidget *widget, *vbox;
	BootImage *image;
	int        i;

	image = boot_image_new ();
	if (!image)
		return FALSE;

	druid->gui = boot_settings_gui_new (image, GTK_WIDGET (druid));

        /* get our toplevel widget and reparent it */
	widget = glade_xml_get_widget (druid->gui->xml, "druid_druid");
	gtk_widget_reparent (widget, GTK_WIDGET (druid));
	druid->druid = GNOME_DRUID (widget);
	
	/* set window title */
	gtk_window_set_title (GTK_WINDOW (druid), _("Boot Image Wizard"));
	gtk_window_set_modal (GTK_WINDOW (druid), TRUE);

	/* attach to druid page signals */
	for (i = 0; pages[i].name != NULL; i++) {
		GtkWidget *page;

		page = glade_xml_get_widget (druid->gui->xml, pages[i].name);

		if (pages[i].next_func)
			g_signal_connect (G_OBJECT (page), "next",
					  pages[i].next_func, druid);
		if (pages[i].prepare_func)
			g_signal_connect (G_OBJECT (page), "prepare",
					  pages[i].prepare_func, druid);
		if (pages[i].back_func)
			g_signal_connect (G_OBJECT (page), "back",
					  pages[i].back_func, druid);
		if (pages[i].finish_func)
			g_signal_connect (G_OBJECT (page), "finish",
					  pages[i].finish_func, druid);
	}
	g_signal_connect (G_OBJECT (druid->druid), "cancel",
			  G_CALLBACK (druid_cancel), druid);

	/* Reparent "interesting" widgets. */
	
	vbox = glade_xml_get_widget (druid->gui->xml, "druid_identity_vbox");
	widget = druid->gui->basic_frame;
	gtk_widget_reparent (widget, vbox);
	gtk_box_set_child_packing (GTK_BOX (vbox), widget, TRUE, TRUE, 0, GTK_PACK_START);

	vbox = glade_xml_get_widget (druid->gui->xml, "druid_image_vbox");
	widget = druid->gui->image_frame;
	gtk_widget_reparent (widget, vbox);
	gtk_box_set_child_packing (GTK_BOX (vbox), widget, TRUE, TRUE, 0, GTK_PACK_START);

	vbox = glade_xml_get_widget (druid->gui->xml, "druid_other_vbox");
	widget = druid->gui->other_frame;
	gtk_widget_reparent (widget, vbox);
	gtk_box_set_child_packing (GTK_BOX (vbox), widget, TRUE, TRUE, 0, GTK_PACK_START);
		
	boot_settings_gui_setup (druid->gui, NULL);
	gtk_combo_set_popdown_strings (druid->gui->type, settings_type_list ());
	
	/* Connect druid specific signals. */
	
	g_signal_connect (G_OBJECT (druid->gui->name), "changed",
			  G_CALLBACK (identity_changed), druid);
	g_signal_connect (G_OBJECT (druid->gui->type->entry), "activate",
			  G_CALLBACK (druid_entry_activate), druid);
	
	g_signal_connect (G_OBJECT (druid->gui->image_entry), "changed",
			  G_CALLBACK (image_changed), druid);
	g_signal_connect (G_OBJECT (druid->gui->root->entry), "changed",
			  G_CALLBACK (image_changed), druid);
	g_signal_connect (G_OBJECT (druid->gui->append), "activate",
			  G_CALLBACK (druid_entry_activate), druid);

	g_signal_connect (G_OBJECT (druid->gui->device->entry), "changed",
			  G_CALLBACK (other_changed), druid);
	g_signal_connect (G_OBJECT (druid->gui->device->entry), "activate",
			  G_CALLBACK (druid_entry_activate), druid);

	return TRUE;
}

BootDruid *
boot_druid_new (void)
{
	BootDruid *new;

	new = (BootDruid *) g_type_create_instance (boot_druid_get_type ());

	if (construct (new))
		return new;
	else {
		gtk_widget_destroy (GTK_WIDGET (new));
		return NULL;
	}
}
