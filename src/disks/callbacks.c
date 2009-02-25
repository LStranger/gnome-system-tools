/* callbacks.c: this file is part of disks-admin, a gnome-system-tool frontend 
 * for disks administration.
 * 
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
 * Authors: Alvaro Peña Gonzalez <apg@esware.com>
 *          Carlos Garcia Campos <elkalmail@yahoo.es>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>

#include "gst.h"

#include "disks-storage.h"
#include "disks-storage-disk.h"
#include "disks-storage-cdrom.h"
#include "disks-cdrom-disc.h"
#include "disks-cdrom-disc-data.h"
#include "disks-cdrom-disc-audio.h"
#include "disks-mountable.h"
#include "disks-partition.h"
#include "disks-gui.h"
#include "callbacks.h"

extern GstTool *tool;

enum {
	TAB_PROPERTIES,
	TAB_PARTITIONS,
	TAB_CDROM_DISC
};

enum {
	TAB_PROP_DISK,
	TAB_PROP_CDROM
};

void
gst_on_storage_list_selection_change (GtkTreeSelection *selection, gpointer gdata)
{
	   GtkWidget        *notebook;
	   GtkTreeModel     *model;
	   GtkTreeIter       iter;
	   GtkTreeSelection *selec;
	   GstDisksStorage  *storage;
	   GtkWidget        *treeview;
	   GtkWidget        *storage_list;
	   GtkWidget        *main_window;
	   GList            *partitions;
	   gboolean          cd_empty;
	   GstCdromDisc     *disc;
	   GtkWidget        *properties_notebook;
	   GdkCursor *cursor;
	   
	   /* Avoid cycle */
	   g_signal_handlers_block_by_func (G_OBJECT (selection),
								 G_CALLBACK (gst_on_storage_list_selection_change),
								 NULL);
	   
	   notebook = gst_dialog_get_widget (tool->main_dialog, "main_notebook");
	   storage_list = gst_dialog_get_widget (tool->main_dialog, "storage_list");
	   main_window = gst_dialog_get_widget (tool->main_dialog, "disks_admin");
	   
	   if (gtk_tree_selection_get_selected (GTK_TREE_SELECTION (selection), &model, &iter)) {
			 gtk_tree_model_get (model, &iter, STORAGE_LIST_POINTER, &storage, -1);
			 
			 if (GST_IS_DISKS_STORAGE (storage)) {
				    /* Properties Notebook */
				    properties_notebook = gst_dialog_get_widget (tool->main_dialog,
													    "properties_notebook");
				    if (GST_IS_DISKS_STORAGE_DISK (storage)) {
						  gtk_widget_show (properties_notebook);
						  gtk_notebook_set_current_page (GTK_NOTEBOOK (properties_notebook),
												   TAB_PROP_DISK);
						  
						  cursor = gdk_cursor_new (GDK_WATCH);
						  if (!GTK_WIDGET_REALIZED (main_window))
								gtk_widget_realize (GTK_WIDGET (main_window));
						  gdk_window_set_cursor (main_window->window, cursor);
						  gtk_widget_set_sensitive (storage_list, FALSE);
						  
						  gst_disks_storage_setup_properties_widget (storage);
						  
						  gtk_widget_set_sensitive (storage_list, TRUE);
						  gdk_cursor_unref (cursor);
						  gdk_window_set_cursor (main_window->window, NULL);
						  
						  
						  g_object_get (G_OBJECT (storage), "partitions", &partitions, NULL);
						  
						  treeview = gst_dialog_get_widget (tool->main_dialog, "partition_list");
						  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
						  
						  if (partitions) {
								gst_disks_gui_setup_partition_list (treeview, partitions);
								
								selec = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
								gtk_tree_model_get_iter_first (model, &iter);
								gtk_tree_selection_select_iter (selec, &iter);
						  } else {
								gtk_tree_store_clear (GTK_TREE_STORE (model));
								gst_on_partition_list_selection_change (
									   gtk_tree_view_get_selection (
											 GTK_TREE_VIEW (treeview)),
									   NULL);
						  }
						  
						  gtk_widget_hide (gtk_notebook_get_nth_page (
											  GTK_NOTEBOOK (notebook), TAB_CDROM_DISC));
						  gtk_widget_show (gtk_notebook_get_nth_page (
											  GTK_NOTEBOOK (notebook), TAB_PARTITIONS));
						  
				    } else if (GST_IS_DISKS_STORAGE_CDROM (storage)) {
						  gtk_widget_show (properties_notebook);
						  gtk_notebook_set_current_page (GTK_NOTEBOOK (properties_notebook),
												   TAB_PROP_CDROM);
						  
						  cursor = gdk_cursor_new (GDK_WATCH);
						  if (!GTK_WIDGET_REALIZED (main_window))
								gtk_widget_realize (GTK_WIDGET (main_window));
						  gdk_window_set_cursor (main_window->window, cursor);
						  gtk_widget_set_sensitive (storage_list, FALSE);
						  
						  gst_disks_storage_setup_properties_widget (storage);
						  
						  gtk_widget_set_sensitive (storage_list, TRUE);
						  gdk_cursor_unref (cursor);
						  gdk_window_set_cursor (main_window->window, NULL);
						  
						  g_object_get (G_OBJECT (storage), "empty", &cd_empty,
									 "disc", &disc, NULL);
						  
						  if (!cd_empty) {
								gtk_widget_show (gtk_notebook_get_nth_page (
													GTK_NOTEBOOK (notebook),
													TAB_CDROM_DISC));
						  } else {
								gtk_widget_hide (gtk_notebook_get_nth_page (
													GTK_NOTEBOOK (notebook),
													TAB_CDROM_DISC));
						  }
						  
						  gtk_widget_hide (gtk_notebook_get_nth_page (
											  GTK_NOTEBOOK (notebook), TAB_PARTITIONS));
				    } else {
						  gtk_widget_hide (gtk_notebook_get_nth_page (
											  GTK_NOTEBOOK (notebook), TAB_PARTITIONS));
						  gtk_widget_hide (gtk_notebook_get_nth_page (
											  GTK_NOTEBOOK (notebook), TAB_CDROM_DISC));
						  gtk_widget_hide (properties_notebook);
				    }
			 }
	   }
	   
	   /* Avoid cycle */
	   g_signal_handlers_unblock_by_func (G_OBJECT (selection),
								   G_CALLBACK (gst_on_storage_list_selection_change),
								   NULL);
}

gboolean
gst_on_storage_list_button_press (GtkTreeView *treeview, GdkEventButton *event, gpointer gdata)
{
	   GtkTreeSelection *selection;
	   
	   if (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS) {
			 selection = gtk_tree_view_get_selection (treeview);
			 gst_on_storage_list_selection_change (selection, NULL);
	   }

	   return FALSE;
}

void 
gst_on_partition_list_selection_change (GtkTreeSelection *selection, gpointer gdata)
{
	GtkTreeModel      *model;
	GtkTreeIter        iter;
	GstDisksPartition *part = NULL;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, PARTITION_LIST_POINTER, &part, -1);
		if (GST_IS_DISKS_PARTITION (part)) {
			gst_disks_partition_setup_properties_widget (part);
		}
	} else {
		gst_disks_partition_setup_properties_widget (part); /* part == NULL */
	}
}

void
gst_on_point_entry_changed (GtkWidget *entry, gpointer gdata)
{
	GtkWidget            *treeview;
	GtkTreeModel         *model;
	GtkTreeIter           iter;
	GtkTreeSelection     *selection;
	GstDisksPartition    *part;
	GstDisksStorageCdrom *cdrom;
	GstCdromDisc         *disc;
	GstCdromDiscData     *disc_data;
	const gchar *point;

	treeview = (GtkWidget *) gdata;

	point = gtk_entry_get_text (GTK_ENTRY (entry));

	if (point == NULL || strlen (point) <= 0)
		return;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		   if (gtk_tree_model_get_n_columns (model) == PARTITION_LIST_LAST) {
				 gtk_tree_model_get (model, &iter, PARTITION_LIST_POINTER, &part, -1);
				 if (GST_IS_DISKS_PARTITION (part)) {
					    /* TODO: point should be a directory */
					    g_object_set (G_OBJECT (part), "point", point, NULL);
				 }
		   } else if (gtk_tree_model_get_n_columns (model) == STORAGE_LIST_LAST) {
				 gtk_tree_model_get (model, &iter, STORAGE_LIST_POINTER, &cdrom, -1);
				 if (GST_IS_DISKS_STORAGE_CDROM (cdrom)) {
					    g_object_get (G_OBJECT (cdrom), "disc", &disc, NULL);
					    if (GST_IS_CDROM_DISC_DATA (disc)) {
							  /* TODO: point should be a directory */
							  g_object_set (G_OBJECT (disc), "mount-point",
										 point, NULL);
					    } else if (GST_IS_CDROM_DISC_MIXED (disc)) {
							  g_object_get (G_OBJECT (disc), "data",
										 &disc_data, NULL);
							  if (GST_IS_CDROM_DISC_DATA (disc_data))
									/* TODO: point should be a directory */
									g_object_set (G_OBJECT (disc_data), "mount-point",
											    point, NULL);
					    }
					    
				 }
		   }
	}
}

static gboolean
update_progress_bar (gpointer gdata)
{
	   GtkWidget *progress = NULL;
	   gdouble fraction;

	   if (gdata == NULL)
			 return FALSE;

	   progress = (GtkWidget *) gdata;
	   
	   fraction = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (progress));

	   if (fraction != 0.7 && fraction < 0.9) {
			 gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress),
									  fraction + 0.1);
	   }

	   return TRUE;
}

void
gst_on_format_button_clicked (GtkWidget *button, gpointer gdata)
{
	   GtkWidget              *treeview;
	   GtkTreeModel           *model, *combo_model;
	   GtkTreeIter             iter, iter2;
	   GtkTreeSelection       *selection;
	   GtkWidget              *dialog, *msgdialog;
	   GtkWidget              *progress, *cancel_button, *format_button;
	   GtkWidget              *filesys_combo, *mpoint_combo, *entry;
	   GstDisksPartition      *part;
	   GstPartitionTypeFsInfo *table;
	   GstPartitionTypeFs      type;
	   static gint timeout_id = 0;
	   gchar *fs_type, *device, *bar_text;
	   const gchar *point = NULL;
	   gchar *current_point, *name;
	   gboolean repeat = FALSE;

	   treeview = (GtkWidget *) gdata;

	   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	   if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			 gtk_tree_model_get (model, &iter, PARTITION_LIST_POINTER, &part, -1);

			 gst_disks_gui_setup_format_dialog (part);

			 filesys_combo = gst_dialog_get_widget (tool->main_dialog,
											"fmt_filesys_combo");
			 mpoint_combo = gst_dialog_get_widget (tool->main_dialog,
										    "fmt_mount_point_combo");
			 cancel_button = gst_dialog_get_widget (tool->main_dialog,
											"fmt_cancel_button");
			 format_button = gst_dialog_get_widget (tool->main_dialog,
											"fmt_format_button");
			 dialog = gst_dialog_get_widget (tool->main_dialog, "format_dialog");
			 
			 do {
				    repeat = FALSE;

				    gtk_widget_set_sensitive (filesys_combo, TRUE);
				    gtk_widget_set_sensitive (mpoint_combo, TRUE);
				    gtk_widget_set_sensitive (cancel_button, TRUE);
				    gtk_widget_set_sensitive (format_button, TRUE);
				    
				    switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
				    case GTK_RESPONSE_ACCEPT:
						  g_object_get (G_OBJECT (part), "device", &device,
									 "point", &current_point,
									 "name", &name, NULL);
						  
						  if (current_point == NULL)
								current_point = g_strdup ("none");
						  
						  gtk_widget_set_sensitive (filesys_combo, FALSE);
						  gtk_widget_set_sensitive (mpoint_combo, FALSE);
						  gtk_widget_set_sensitive (cancel_button, FALSE);
						  gtk_widget_set_sensitive (format_button, FALSE);

						  combo_model = gtk_combo_box_get_model (GTK_COMBO_BOX (filesys_combo));
						  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (filesys_combo), &iter2);
						  gtk_tree_model_get (combo_model, &iter2, 0, &fs_type, -1);

						  entry = gtk_bin_get_child (GTK_BIN (mpoint_combo));
						  if (GTK_IS_ENTRY (entry)) {
								point = gtk_entry_get_text (GTK_ENTRY (entry));
						  }

						  if ((g_ascii_strcasecmp (current_point, point) != 0) &&
							 (gst_disks_gui_get_mount_point_is_busy (point))) {
								msgdialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
															 GTK_DIALOG_MODAL,
															 GTK_MESSAGE_ERROR,
															 GTK_BUTTONS_CLOSE,
															 _("Access Path \"%s\" already in use"),
															 point);
								gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msgdialog),
																  _("Access Path \"%s\" is already being "
																    "used by other partition."), point);
								gtk_dialog_run (GTK_DIALOG (msgdialog));
								gtk_widget_destroy (msgdialog);
								
								repeat = TRUE;
								
								break;
						  }

						  msgdialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
													   GTK_DIALOG_MODAL,
													   GTK_MESSAGE_WARNING,
													   GTK_BUTTONS_YES_NO,
													   _("Are you sure you want to "
														"format \"%s (%s)\"?"),
													   name, device);
						  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msgdialog),
														    _("Formatting a partition implies "
															 "that all data in that partition "
															 "will be lost."));

						  if (gtk_dialog_run (GTK_DIALOG (msgdialog)) != GTK_RESPONSE_YES) {
								gtk_widget_destroy (msgdialog);
								repeat = FALSE;

								break;
						  }

						  gtk_widget_destroy (msgdialog);
						  
						  table = gst_disks_partition_get_type_fs_info_table ();
						  type = gst_disks_partition_get_typefs_from_hr_name (fs_type);
						  
						  progress = gst_dialog_get_widget (tool->main_dialog, "fmt_progress");
						  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), 0.0);
						  bar_text = g_strdup_printf ("Formatting %s . . .", device);
						  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), bar_text);
						  g_free (bar_text);
						  
						  if (timeout_id > 0)
								g_source_remove (timeout_id);
				    
						  timeout_id = g_timeout_add (25, update_progress_bar,
												progress);

						  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), 0.7);
						  gst_disks_partition_format (part, type, point);
						  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), 0.8);
						  
						  gst_disks_partition_setup_properties_widget (part);
						  
						  if (timeout_id > 0)
								g_source_remove (timeout_id);
						  
						  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), 1.0);
						  
						  break;
				    case GTK_RESPONSE_CANCEL:
						  break;
				    default:
						  break;
				    }
			 } while (repeat == TRUE);
			 
			 gtk_widget_hide (dialog);
			 
			 if (current_point && (g_ascii_strcasecmp (current_point, "none") == 0))
				    g_free (current_point);
	   }
}

void
gst_on_mount_button_clicked (GtkWidget *button, gpointer gdata)
{
	GtkWidget            *treeview;
	GtkTreeModel         *model;
	GtkTreeIter           iter;
	GtkTreeSelection     *selection;
	GstDisksPartition    *part;
	GstDisksStorageCdrom *cdrom;
	GstCdromDisc *disc;

	treeview = (GtkWidget *) gdata;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		if (gtk_tree_model_get_n_columns (model) == PARTITION_LIST_LAST) {
			gtk_tree_model_get (model, &iter, PARTITION_LIST_POINTER, &part, -1);

			if (GST_IS_DISKS_PARTITION (part)) {
				gst_disks_mountable_mount (GST_DISKS_MOUNTABLE (part));
				
				gst_disks_partition_setup_properties_widget (part);
			}
		} else if (gtk_tree_model_get_n_columns (model) == STORAGE_LIST_LAST) {
			gtk_tree_model_get (model, &iter, STORAGE_LIST_POINTER, &cdrom, -1);
			if (GST_IS_DISKS_STORAGE_CDROM (cdrom)) {
				g_object_get (G_OBJECT (cdrom), "disc", &disc, NULL);
				if (GST_IS_CDROM_DISC_DATA (disc))
					gst_disks_mountable_mount (GST_DISKS_MOUNTABLE (disc));
			
				gst_disks_storage_setup_properties_widget (GST_DISKS_STORAGE (cdrom));
			}
			
		}
	}
}

void
gst_on_browse_button_clicked (GtkWidget *button, gpointer gdata)
{
	GtkWidget            *treeview;
	GtkTreeModel         *model;
	GtkTreeIter           iter;
	GtkTreeSelection     *selection;
	GstDisksPartition    *part;
	GstDisksStorageCdrom *cdrom;
	GstCdromDisc         *disc;

	treeview = (GtkWidget *) gdata;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		if (gtk_tree_model_get_n_columns (model) == PARTITION_LIST_LAST) {
			gtk_tree_model_get (model, &iter, PARTITION_LIST_POINTER, &part, -1);
			if (GST_IS_DISKS_PARTITION (part)) {
				gst_disks_partition_browse (part);
			}
		} else if (gtk_tree_model_get_n_columns (model) == STORAGE_LIST_LAST) {
			gtk_tree_model_get (model, &iter, STORAGE_LIST_POINTER, &cdrom, -1);
			if (GST_IS_DISKS_STORAGE_CDROM (cdrom)) {
				g_object_get (G_OBJECT (cdrom), "disc", &disc, NULL);
				if (GST_IS_CDROM_DISC_DATA (disc)) {
					gst_disks_cdrom_disc_data_browse (GST_CDROM_DISC_DATA (disc));
				} else if (GST_IS_CDROM_DISC_MIXED (disc)) {
					gst_disks_cdrom_disc_mixed_browse (GST_CDROM_DISC_MIXED (disc));
				}
			}
		}
	}
}

void
gst_on_play_button_clicked (GtkWidget *button, gpointer gdata)
{
	   GtkWidget             *treeview;
	   GtkTreeModel          *model;
	   GtkTreeIter            iter;
	   GtkTreeSelection      *selection;
	   GstDisksStorageCdrom  *cdrom;
	   GstCdromDisc          *disc;
	   const gchar *device;

	   treeview = (GtkWidget *) gdata;

	   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	   if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			 gtk_tree_model_get (model, &iter, STORAGE_LIST_POINTER, &cdrom, -1);
			 if (GST_IS_DISKS_STORAGE_CDROM (cdrom)) {
				    g_object_get (G_OBJECT (cdrom), "device", &device, 
							   "disc", &disc, NULL);
				    if (GST_IS_CDROM_DISC_AUDIO (disc)) {
						  gst_disks_cdrom_disc_audio_play (GST_CDROM_DISC_AUDIO (disc), device);
				    } else if (GST_IS_CDROM_DISC_MIXED (disc)) {
						  gst_disks_cdrom_disc_mixed_play (GST_CDROM_DISC_MIXED (disc), device);
				    } else if (GST_IS_CDROM_DISC_DVD (disc)) {
						  gst_disks_cdrom_disc_dvd_play (GST_CDROM_DISC_DVD (disc));
				    }
			 }
	   }
}

void
gst_disks_storage_get_device_speed_cb (GstDirectiveEntry *entry)
{
	GtkWidget *speed_label;
	xmlDoc *xml;
	xmlNodePtr root;
	gchar *device, *media;
	gchar *buf = NULL;
	gchar *speed;
	GstDisksStorage *storage;

	storage = (GstDisksStorage *) entry->data;

	if (GST_IS_DISKS_STORAGE_CDROM (storage)) {
		media = g_strdup ("cdrom");
	} else if (GST_IS_DISKS_STORAGE_DISK (storage)) {
		media = g_strdup ("disk");
	} else
		return;

	speed = g_strdup_printf ("%s_speed_label", media);
	speed_label = gst_dialog_get_widget (tool->main_dialog, speed);
	g_free (speed);

	g_object_set (G_OBJECT (storage), "speed", _("Getting ..."), NULL);
	gtk_label_set_text (GTK_LABEL (speed_label), _("Getting ..."));

	g_object_get (G_OBJECT (storage), "device", &device, NULL);
	xml = gst_tool_run_get_directive (entry->tool, entry->report_sign,
					  entry->directive, device, NULL);

	if (!xml) {
		g_object_set (G_OBJECT (storage), "speed", _("Not Available"), NULL);
		gtk_label_set_text (GTK_LABEL (speed_label), _("Not Available"));
		return;
	}

	root = gst_xml_doc_get_root (xml);
	if (root) {
		buf = gst_xml_get_child_content (root, "speed");
	}
	gst_xml_doc_destroy (xml);

	if (buf) {
		g_object_set (G_OBJECT (storage), "speed", buf, NULL);
		gtk_label_set_text (GTK_LABEL (speed_label), buf);
		g_free (buf);
	} else {
		g_object_set (G_OBJECT (storage), "speed", _("Not Available"), NULL);
		gtk_label_set_text (GTK_LABEL (speed_label), _("Not Available"));
	}

	g_free (media);
}

void
gst_on_change_mp_button_clicked (GtkWidget *button, gpointer gdata)
{
	GtkWidget *filesel;
	GtkWidget *point_entry;
	gchar     *filename;

	filesel = gtk_file_chooser_dialog_new (_("Select New Mount Point Path"),
								    NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
								    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
								    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	if (g_ascii_strcasecmp (gtk_widget_get_name (button), "cd_change_mp_button") == 0) {
		   point_entry = gst_dialog_get_widget (tool->main_dialog, "cd_point_entry");
	} else {
		   point_entry = gst_dialog_get_widget (tool->main_dialog, "part_point_entry");
	}

	filename = g_strdup (gtk_entry_get_text (GTK_ENTRY (point_entry)));
	
	if (g_file_test (filename, G_FILE_TEST_IS_DIR)) {
		   gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (filesel),
										filename);
	}

	if (filename) {
		   g_free (filename);
		   filename = NULL;
	}
								  
	if (gtk_dialog_run (GTK_DIALOG (filesel)) == GTK_RESPONSE_ACCEPT) {
		   filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filesel));
		   gtk_entry_set_text (GTK_ENTRY (point_entry), filename);
		   g_free (filename);
	}

	gtk_widget_destroy (filesel);
}
