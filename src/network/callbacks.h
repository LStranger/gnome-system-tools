/* Copyright (C) 2000 Helix Code, Inc.
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
 * Authors: Hans Petter Jansson <hpj@helixcode.com> and Arturo Espinosa <arturo@helixcode.com>.
 */

#include <gnome.h>
#include <gnome-xml/tree.h>


extern xmlDocPtr doc;


extern void 
on_nameresolution_admin_show (GtkWidget *w, gpointer user_data);

extern void
on_nameresolution_notebook_switch_page (GtkWidget *notebook, GtkNotebookPage *page,
																				gint page_num, gpointer user_data);

extern void
on_dns_list_select_child               (GtkList         *list,
                                        GtkWidget       *widget,
                                        gpointer         user_data);

extern void
on_dns_list_unselect_child             (GtkList         *list,
                                        GtkWidget       *widget,
                                        gpointer         user_data);

extern void
on_dns_list_add (GtkContainer *list, gpointer user_data);

extern void
on_dns_ip_activate                     (GtkEditable     *editable, 
																				gpointer         user_data);

extern void
on_dns_ip_add_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

extern void
on_dns_delete_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

extern void
on_wins_use_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

extern void
on_searchdomain_list_select_child      (GtkList         *list,
                                        GtkWidget       *widget,
                                        gpointer         user_data);

extern void
on_searchdomain_list_unselect_child    (GtkList         *list,
                                        GtkWidget       *widget,
                                        gpointer         user_data);

extern void
on_searchdomain_list_add (GtkContainer *list, gpointer user_data);

extern void
on_searchdomain_activate               (GtkEditable     *editable,
                                        gpointer         user_data);

extern void
on_searchdomain_add_clicked            (GtkButton       *button,
                                        gpointer         user_data);

extern void
on_searchdomain_delete_clicked         (GtkButton       *button,
                                        gpointer         user_data);

extern void
on_searchdomain_move_up_clicked        (GtkButton       *button,
																				gpointer         user_data);

extern void
on_searchdomain_move_down_clicked      (GtkButton       *button,
																				gpointer         user_data);

extern void 
on_statichost_list_state_changed (GtkCList *clist, gpointer user_data);


extern void
on_statichost_list_select_row          (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

extern void
on_statichost_list_unselect_row        (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

extern void
on_statichost_new_clicked           (GtkButton       *button,
                                        gpointer         user_data);

extern void
on_statichost_delete_clicked        (GtkButton       *button,
                                        gpointer         user_data);

extern void
on_statichost_settings_clicked      (GtkButton       *button,
                                        gpointer         user_data);

extern gboolean
on_aliases_settings_dialog_delete_event  (GtkWidget       *widget,
                                          GdkEvent        *event,
                                          gpointer         user_data);

extern void
on_aliases_settings_dialog_show (GtkWidget *w, gpointer user_data);

extern void
on_aliases_settings_ip_changed (GtkWidget *w, gpointer user_data);

extern void
on_aliases_settings_list_select_child (GtkList *list, GtkWidget *widget, gpointer user_data);

extern void
on_aliases_settings_list_unselect_child (GtkList *list, GtkWidget *widget, gpointer user_data);

extern void
on_aliases_settings_list_add (GtkContainer *list, gpointer user_data);

extern void
on_aliases_settings_ok_clicked (GtkWidget *w, gpointer user_data);

extern void
on_aliases_settings_cancel_clicked (GtkWidget *w, gpointer user_data);

extern void
on_aliases_settings_delete_clicked (GtkWidget *w, gpointer user_data);

extern void
on_aliases_settings_add_clicked (GtkWidget *w, gpointer user_data);

extern void
on_aliases_settings_new_activate (GtkEditable *editable, gpointer user_data);

extern void
on_help_clicked                        (GtkButton       *button,
                                        gpointer         user_data);

extern void
on_close_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

extern void
on_apply_clicked                       (GtkButton       *button,
                                        gpointer         user_data);

extern gboolean
on_nameresolution_admin_delete_event  (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);
