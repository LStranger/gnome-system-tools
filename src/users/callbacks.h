#include <gnome.h>
#include <gnome-xml/tree.h>


extern xmlDocPtr doc;

void
on_help_clicked                        (GtkButton       *button,
                                        gpointer         user_data);

void
on_ok_clicked                          (GtkButton       *button,
                                        gpointer         user_data);

void
on_cancel_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_apply_clicked                       (GtkButton       *button,
                                        gpointer         user_data);

void
on_user_settings_clicked (GtkButton *button, gpointer user_data);

void
on_user_settings_cancel_clicked (GtkButton *button, gpointer user_data);

void
on_user_settings_ok_clicked (GtkButton *button, gpointer user_data);



void
on_user_chpasswd_clicked (GtkButton *button, gpointer user_data);

void
on_new_user_clicked (GtkButton *button, gpointer user_data);

void
on_user_delete_clicked (GtkButton *button, gpointer user_data);

void
on_user_list_selection_changed (GtkWidget *list, gpointer user_data);


void
on_group_settings_clicked (GtkButton *button, gpointer user_data);

void
on_group_list_selection_changed (GtkWidget *list, gpointer user_data);



void
on_user_passwd_cancel_clicked (GtkButton *button, gpointer user_data);

void
on_user_passwd_ok_clicked (GtkButton *button, gpointer user_data);

void
on_group_settings_cancel_clicked (GtkButton *button, gpointer user_data);

void
on_group_settings_ok_clicked (GtkButton *button, gpointer user_data);


