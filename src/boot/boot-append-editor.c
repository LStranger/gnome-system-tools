/* boot_append_editor.c: this file is part of boot-admin, a ximian-setup-tool
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

#include "boot-append-editor.h"

extern GstTool *tool;

static void boot_append_editor_class_init (BootAppendEditorClass *class);
static void boot_append_editor_finalize   (GObject *obj);

static GtkDialogClass *parent_class;

GtkType
boot_append_editor_get_type (void)
{
	   static GtkType type = 0;
	   if (!type) 
	   {
			 static const GTypeInfo type_info = 
				    {
						  sizeof (BootAppendEditorClass),
						  NULL, /* base_init */
						  NULL, /* base finalize */
						  (GClassInitFunc) boot_append_editor_class_init,
						  NULL, /* class_finalize */
						  NULL, /* class_data */
						  sizeof (BootAppendEditor),
						  0, /* n_preallocs */
						  (GInstanceInitFunc) NULL
				    };
			 
			 type = g_type_register_static (GTK_TYPE_DIALOG, "BootAppendEditor", &type_info, 0);
	   }
	   
	   return type;
}

static void
boot_append_editor_class_init (BootAppendEditorClass *class)
{
	   GObjectClass *object_class = G_OBJECT_CLASS (class);
	   
	   parent_class = gtk_type_class (gtk_dialog_get_type ());
	   
	   object_class->finalize = boot_append_editor_finalize;
}

static void
boot_append_editor_finalize (GObject *obj)
{
	   BootAppendEditor *editor = (BootAppendEditor *) obj;
	   
	   boot_append_gui_destroy (editor->gui);
	   
	   G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
editor_append_response (GtkDialog *dialog, gint response, gpointer data)
{
	   BootAppendEditor *editor;
	   char *append;
	   
	   editor = (BootAppendEditor *) data;
	   
	   switch (response) 
	   {
	   case GTK_RESPONSE_ACCEPT:
			 if (boot_append_gui_save (editor->gui, &append))
			 {
				    if (append)
				    {   
						  gst_ui_entry_set_text (GTK_ENTRY (editor->gui->settings->append), g_strdup (append));
						  g_free (append);
				    }
			 }
			 break;
	   default:
			 break;
	   }
	   
	   gtk_widget_destroy (GTK_WIDGET (editor));
}

static gboolean
construct (BootAppendEditor *editor, BootSettingsGui *settings)
{
	   GtkWidget *w;
	   
	   editor->gui = boot_append_gui_new (settings, GTK_WIDGET (editor));
	   
	   if (!editor->gui)
			 return FALSE;
	   
	   w = glade_xml_get_widget (editor->gui->xml, "boot_append_editor");
	   gtk_widget_reparent (w, GTK_DIALOG (editor)->vbox);
	   
	   /* give our dialog an OK button and title */
	   gtk_window_set_title (GTK_WINDOW (editor), _("Boot Append Editor"));
	   gtk_window_set_policy (GTK_WINDOW (editor), FALSE, TRUE, TRUE);
	   gtk_window_set_modal (GTK_WINDOW (editor), TRUE);
	   gtk_dialog_add_buttons (GTK_DIALOG (editor),
						  GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
						  GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
						  NULL);
	   
	   g_signal_connect (G_OBJECT (editor), "response",
					 G_CALLBACK (editor_append_response), editor);
	   
	   boot_append_gui_setup (editor->gui, settings);
	   
	   return TRUE;
}

BootAppendEditor *
boot_append_editor_new (BootSettingsGui *settings)
{
	   BootAppendEditor *new;
	   
	   if (!settings)
			 return NULL;
	   
	   new = (BootAppendEditor *) g_type_create_instance (boot_append_editor_get_type ());
	   
	   if (construct (new, settings))
			 return new;
	   else
	   {
			 gtk_widget_destroy (GTK_WIDGET (new));
			 return NULL;
	   }
}

