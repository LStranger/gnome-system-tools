/* boot_append_gui.h: this file is part of boot-admin, a ximian-setup-tool
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

#ifndef __BOOT_APPEND_H
#define __BOOT_APPEND_H

#include <gnome.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-file-entry.h>


#include "boot-settings.h"

typedef struct
{
	   GtkWidget *top;
	   /*BootImage *image;*/
	   BootSettingsGui *settings;
	   GladeXML  *xml;
	   
	   /* vga modes */
	   GtkCheckButton *append_vga;
	   
	   GtkRadioButton *append_vga_manual;
	   GtkRadioButton *append_vga_ask;

	   GtkOptionMenu *append_menu_colors;
	   GtkOptionMenu *append_menu_res;
	   
	   GtkLabel      *append_label_colors;
	   GtkLabel      *append_label_res;
	   
	   GtkCheckButton *append_scsi;
	   
	   GtkCheckButton *append_scsi_hda;
	   GtkCheckButton *append_scsi_hdb;
	   GtkCheckButton *append_scsi_hdc;
	   GtkCheckButton *append_scsi_hdd;
	   
	   GtkCheckButton *append_others;
	   
	   GtkEntry       *append_entry_others;

} BootAppendGui;

BootAppendGui   *boot_append_gui_new      (BootSettingsGui *settings, GtkWidget *parent);
gboolean 	      boot_append_gui_save	  (BootAppendGui *gui, char **append_string);
void 		 boot_append_gui_error     (GtkWindow *parent, gchar *error);
void            boot_append_gui_destroy   (BootAppendGui *gui);

/* callbacks */
void            on_boot_append_browse_clicked    (GtkButton *button, gpointer data);

#endif /* BOOT_APPEND_H */
