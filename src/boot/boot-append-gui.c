/* boot_append_gui.c: this file is part of boot-admin, a ximian-setup-tool
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <ctype.h>
#include <glib/gi18n.h>

#include "gst.h"
#include "callbacks.h"
#include "boot-append-gui.h"

extern GstTool *tool;

char *vga_table[][4] =
{
	   "769", "771", "773", "775",
	   "784", "787", "790", "793",
	   "785", "788", "791", "794",
	   "786", "789", "792", "795"
};

void
on_append_vga_manual_toggle (GtkCheckButton *append_vga_manual, gpointer data)
{
	   BootAppendGui *gui;
	   
	   gui = (BootAppendGui *) data;
	   	   
	   if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (append_vga_manual)))
	   {
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_label_colors), TRUE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_menu_colors), TRUE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_label_res), TRUE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_menu_res), TRUE);
	   }
	   else
	   {
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_label_colors), FALSE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_menu_colors), FALSE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_label_res), FALSE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_menu_res), FALSE);
	   }
}
   
void
on_append_vga_toggle (GtkCheckButton *append_vga, gpointer data)
{
	   BootAppendGui *gui;
	   
	   gui = (BootAppendGui *) data;
	   
	   if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (append_vga)))
	   {
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_vga_manual), FALSE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_vga_ask), FALSE);

			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_label_colors), FALSE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_menu_colors), FALSE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_label_res), FALSE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_menu_res), FALSE);
	   }
	   else
	   {
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_vga_manual), TRUE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_vga_ask), TRUE);
			 
			 if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gui->append_vga_manual)))
			 {
				    gtk_widget_set_sensitive (GTK_WIDGET (gui->append_label_colors), TRUE);
				    gtk_widget_set_sensitive (GTK_WIDGET (gui->append_menu_colors), TRUE);
				    gtk_widget_set_sensitive (GTK_WIDGET (gui->append_label_res), TRUE);
				    gtk_widget_set_sensitive (GTK_WIDGET (gui->append_menu_res), TRUE); 
			 }
	   }
}

void
on_append_scsi_toggle (GtkCheckButton *append_scsi, gpointer data)
{
	   BootAppendGui *gui;
	   
	   gui = (BootAppendGui *) data;
	   
	   if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (append_scsi)))
	   {
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_scsi_hda), FALSE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_scsi_hdb), FALSE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_scsi_hdc), FALSE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_scsi_hdd), FALSE);
	   }
	   else
	   {
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_scsi_hda), TRUE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_scsi_hdb), TRUE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_scsi_hdc), TRUE);
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_scsi_hdd), TRUE);
	   }
}

void
on_append_others_toggle (GtkCheckButton *append_others, gpointer data)
{
	   BootAppendGui *gui;
	   
	   gui = (BootAppendGui *) data;
	   
	   if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (append_others)))
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_entry_others), FALSE);
	   else
			 gtk_widget_set_sensitive (GTK_WIDGET (gui->append_entry_others), TRUE);
}

static void
fill_colors_menu (BootAppendGui *gui)
{
	   gtk_combo_box_append_text (gui->append_menu_colors, "256");
	   gtk_combo_box_append_text (gui->append_menu_colors, _("32,768"));
	   gtk_combo_box_append_text (gui->append_menu_colors, _("65,536"));
	   gtk_combo_box_append_text (gui->append_menu_colors, _("16,777,216"));

	   gtk_combo_box_set_active (gui->append_menu_colors, 0);
}

static void
fill_resolution_menu (BootAppendGui *gui)
{
	   gtk_combo_box_append_text (gui->append_menu_res, "640x480");
	   gtk_combo_box_append_text (gui->append_menu_res, "800x600");
	   gtk_combo_box_append_text (gui->append_menu_res, "1024x768");
	   gtk_combo_box_append_text (gui->append_menu_res, "1280x1024");

	   gtk_combo_box_set_active (gui->append_menu_res, 0);
}

BootAppendGui *
boot_append_gui_new (BootSettingsGui *settings, GtkWidget *parent)
{
	   BootAppendGui *gui;
	   
	   if (!settings)
			 return NULL;
	   
	   gui = g_new0 (BootAppendGui, 1);
	   gui->settings = settings;
	   gui->xml = glade_xml_new (tool->glade_path, NULL, NULL);
	   gui->top = parent;
	   
	   /* Vga modes */
	   gui->append_vga         = GTK_CHECK_BUTTON (glade_xml_get_widget (gui->xml, "append_vga"));
	   
	   gui->append_vga_manual  = GTK_RADIO_BUTTON (glade_xml_get_widget (gui->xml, "append_vga_manual"));
	   gui->append_vga_ask     = GTK_RADIO_BUTTON (glade_xml_get_widget (gui->xml, "append_vga_ask"));
	   
	   gui->append_label_colors = GTK_LABEL (glade_xml_get_widget (gui->xml, "append_label_colors"));
	   gui->append_menu_colors  = GTK_COMBO_BOX (glade_xml_get_widget (gui->xml, "append_menu_colors"));
	   gui->append_label_res    = GTK_LABEL (glade_xml_get_widget (gui->xml, "append_label_res"));
	   gui->append_menu_res     = GTK_COMBO_BOX (glade_xml_get_widget (gui->xml, "append_menu_res"));
	   
	   /* Scsi Emulation Devices */
	   gui->append_scsi     = GTK_CHECK_BUTTON (glade_xml_get_widget (gui->xml, "append_scsi"));
	   
	   gui->append_scsi_hda = GTK_CHECK_BUTTON (glade_xml_get_widget (gui->xml, "append_scsi_hda"));
	   gui->append_scsi_hdb = GTK_CHECK_BUTTON (glade_xml_get_widget (gui->xml, "append_scsi_hdb"));
	   gui->append_scsi_hdc = GTK_CHECK_BUTTON (glade_xml_get_widget (gui->xml, "append_scsi_hdc"));
	   gui->append_scsi_hdd = GTK_CHECK_BUTTON (glade_xml_get_widget (gui->xml, "append_scsi_hdd"));
	   
	   /* Others */
	   gui->append_others       = GTK_CHECK_BUTTON (glade_xml_get_widget (gui->xml, "append_others"));
	   
	   gui->append_entry_others = GTK_ENTRY (glade_xml_get_widget (gui->xml, "append_entry_others"));
	   
	   /* Connect signals */
	   g_signal_connect (G_OBJECT (gui->append_vga), "toggled",
					 G_CALLBACK (on_append_vga_toggle), (gpointer) gui);
	   g_signal_connect (G_OBJECT (gui->append_vga_manual), "toggled",
					 G_CALLBACK (on_append_vga_manual_toggle), (gpointer) gui);
	   g_signal_connect (G_OBJECT (gui->append_scsi), "toggled",
					 G_CALLBACK (on_append_scsi_toggle), (gpointer) gui);
	   g_signal_connect (G_OBJECT (gui->append_others), "toggled",
					 G_CALLBACK (on_append_others_toggle), (gpointer) gui);

	   /* Fill combo boxes */
	   fill_colors_menu (gui);
	   fill_resolution_menu (gui);
	   
	   return gui;
}

static gint
hextodec (gchar *hex)
{
	   gint i;
	   
	   for (i=2;i<5;i++)
	   {
			 switch (hex[i])
			 {
			 case 'a': case 'A':
				    hex[i] = 58;
				    break;
			 case 'b': case'B':
                        hex[i] = 59;
				    break;
			 case 'c': case'C':
                        hex[i] = 60;
				    break;
			 case 'd': case'D':
                        hex[i] = 61;
				    break;
			 case 'e': case'E':
                        hex[i] = 62;
				    break;
			 case 'f': case'F':
                        hex[i] = 63;
			 }
	   }	 
				    
	   return ((hex[4] - '0') + ((hex[3] - '0') * 16) + ((hex[2] - '0') * 16 * 16));
}

void
boot_append_vga_error (GtkWidget *parent, gchar *vga)
{
	   GtkWidget *dialog;

	   dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
								 GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
								 GTK_MESSAGE_ERROR,
								 GTK_BUTTONS_CLOSE,
								 _("Invalid VGA value"));
	   gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
										_("\"%s\" is not a valid VGA value."), vga);
	   gtk_dialog_run (GTK_DIALOG (dialog));
	   gtk_widget_destroy (dialog);
}

void
append_gui_vga_setup (BootAppendGui *gui, gchar *vga)
{
	   gint vga_num;

	   
	   if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gui->append_vga)))
			 gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->append_vga), TRUE);
	   
	   if (strcmp (vga, "ask") == 0)
	   {
			 gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->append_vga_ask), TRUE);
			 return;
	   }
	   else
			 gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->append_vga_manual), TRUE);
	   
	   if ((vga[0] == '0') && (vga[1] == 'x'))
			 vga_num = hextodec (vga);
	   else
			 vga_num = atoi (vga);

	   switch (vga_num)
	   {
	   case 769: case 784: case 785: case 786:
			 gtk_combo_box_set_active (gui->append_menu_res, 0);
			 break;
	   case 771: case 787: case 788: case 789:
			 gtk_combo_box_set_active (gui->append_menu_res, 1);
			 break;
	   case 773: case 790: case 791: case 792:
			 gtk_combo_box_set_active (gui->append_menu_res, 2);
			 break;
	   case 775: case 793: case 794: case 795:
			 gtk_combo_box_set_active (gui->append_menu_res, 3);
			 break;
	   default:
			 gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->append_vga), FALSE);
	   }
	   
	   switch (vga_num)
	   {
	   case 769: case 771: case 773: case 775:
			 gtk_combo_box_set_active (gui->append_menu_colors, 0);
			 break;
	   case 784: case 787: case 790: case 793:
			 gtk_combo_box_set_active (gui->append_menu_colors, 1);
			 break;
	   case 785: case 788: case 791: case 794:
			 gtk_combo_box_set_active (gui->append_menu_colors, 2);
			 break;
	   case 786: case 789: case 792: case 795:
			 gtk_combo_box_set_active (gui->append_menu_colors, 3);
			 break;
	   default:
			 boot_append_vga_error (gui->top, vga);
			 gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->append_vga), FALSE);
	   }
}

static void
append_gui_scsi_setup (BootAppendGui *gui, gchar *scsi)
{
	   if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gui->append_scsi)))
			 gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->append_scsi), TRUE);
	   
	   if (strcmp (scsi,"hda") == 0)
			 gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->append_scsi_hda), TRUE);
	   else if (strcmp (scsi,"hdb") == 0)
			 gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->append_scsi_hdb), TRUE);
	   else if (strcmp (scsi,"hdc") == 0)
			 gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->append_scsi_hdc), TRUE);
	   else if (strcmp (scsi,"hdd") == 0)
			 gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->append_scsi_hdd), TRUE);
}

static gchar *
str_strip_spaces (const gchar *append)
{
	   gint i,j;
	   gchar *str;
	   
	   str = (gchar *) malloc ((strlen (append) + 1) * sizeof (gchar));
	   str[0] = append[0];
	   for (i=1,j=1;i<strlen (append);i++)
	   {
			 if (append[i] != ' ')
			 {
				    str[j] = append[i];
				    j ++;
			 }
			 else
			 {
				    if (append[i-1] != ' ')
				    {
						  str[j] = append[i];
						  j ++;
				    }
			 }
	   }
	   str[j] = '\0';
	   g_strstrip (str);
	   str[strlen (str)] = '\0';
	   
	   return str;
}

void
boot_append_gui_setup (BootAppendGui *gui, BootSettingsGui *settings)
{
	   
	   gchar **appends;
	   gchar **item;
	   gchar *others;
	   gint i = 0;
	   
	   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->append_vga), FALSE);
	   on_append_vga_toggle (gui->append_vga, (gpointer) gui);
	   
	   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->append_scsi), FALSE);
	   on_append_scsi_toggle (gui->append_vga, (gpointer) gui);
	   
	   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->append_others), FALSE);
	   on_append_others_toggle (gui->append_others, (gpointer) gui);
	   
	   others = (gchar *) g_malloc (strlen (gtk_entry_get_text (GTK_ENTRY (settings->append))) * sizeof (gchar) + 1);
	   others[0] = '\0';
	   
	   if (strlen (gtk_entry_get_text (GTK_ENTRY (settings->append))) > 0)
			 appends = g_strsplit (str_strip_spaces (gtk_entry_get_text (GTK_ENTRY (settings->append))), " ", 0);
	   else
			 return;
	   
	   if (appends)
	   {
			 while (appends[i])
			 {
				    if (g_strrstr (appends[i], "=") != NULL)
				    {
						  item = g_strsplit (appends[i], "=", 2);
						  if ((item) && (strcmp (item[0],"vga") == 0))
								append_gui_vga_setup (gui, g_strdup (item[1]));
						  else if ((item) && (strcmp (item[1],"ide-scsi") == 0))
								append_gui_scsi_setup (gui, g_strdup (item[0]));
						  else
						  {
								g_strstrip (others);
								if (strlen (others) > 0)
									   strcat (others, " ");
								strcat (others, appends[i]);
						  }
						  if (item) g_strfreev (item);
				    }
				    i ++;
			 }
			 g_strfreev (appends);
	   }
	   
	   if (strlen (others) > 0)
	   {
			 gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->append_others), TRUE);
			 gtk_entry_set_text (gui->append_entry_others, g_strdup (others));
			 g_free (others);
	   }
	   else
			 gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->append_others), FALSE);
}

gboolean
boot_append_gui_save (BootAppendGui *gui, gchar **append_string)
{
	   
	   gchar *append;
	   
	   append = (gchar *) g_malloc ((70 + strlen (gtk_entry_get_text (gui->append_entry_others))) * sizeof (gchar));
	   append[0] = '\0';
	   
	   if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gui->append_vga)))
	   {
			 g_strstrip (append);
			 strcat (append, "vga=");
			 if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gui->append_vga_manual)))
			 {
				    strcat (append, vga_table[gtk_combo_box_get_active (gui->append_menu_colors)]
						  [gtk_combo_box_get_active (gui->append_menu_res)]);
			 }	 
			 else
				    strcat (append, "ask");
	   }
	   
	   if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gui->append_scsi)))
	   {
			 if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gui->append_scsi_hda)))
			 {
				    g_strstrip (append);
				    strcat (append, " hda=ide-scsi");
			 }
			 if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gui->append_scsi_hdb)))
			 {
				    g_strstrip (append);
				    strcat (append, " hdb=ide-scsi");
			 }
			 if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gui->append_scsi_hdc)))
			 {
				    g_strstrip (append);
				    strcat (append, " hdc=ide-scsi");
			 }
			 if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gui->append_scsi_hdd)))
			 {
				    g_strstrip (append);
				    strcat (append, " hdd=ide-scsi");
			 }
	   }
	   
	   if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gui->append_others)))
	   {
			 g_strstrip (append);
			 strcat (append, " ");
			 strcat (append, gtk_entry_get_text (gui->append_entry_others));
	   }
	   
	   if (append)
	   {
			 (*append_string) = g_strdup (append);
			 g_free (append);
	   }
	   
	   if (!append_string)
			 return FALSE;
	   
	   return TRUE;
}

void
boot_append_gui_destroy (BootAppendGui *gui)
{
	   if (gui) 
	   {
			 g_object_unref (G_OBJECT (gui->xml));
			 g_free (gui);
	   }
}

