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
 * Authors: Hans Petter Jansson <hpj@ximian.com>
 *          Carlos Garnacho Parro <carlosg@gnome.org>
 */

#include <glib/gi18n.h>
#include "gst.h"
#include "gst-platform-dialog.h"

#define GST_PLATFORM_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_PLATFORM_DIALOG, GstPlatformDialogPrivate))

enum {
	PLATFORM_REDHAT,
	PLATFORM_DEBIAN,
	PLATFORM_MANDRAKE,
	PLATFORM_TURBOLINUX,
	PLATFORM_SLACKWARE,
	PLATFORM_SUSE,
	PLATFORM_FREEBSD,
	PLATFORM_GENTOO,
	PLATFORM_PLD,
	PLATFORM_OPENNA,
	PLATFORM_FEDORA,
	PLATFORM_CONECTIVA,
	PLATFORM_BLACK_PANTHER,
	PLATFORM_VINE,
	PLATFORM_SPECIFIX,
	PLATFORM_ARCHLINUX,
	PLATFORM_VIDALINUX,
	PLATFORM_LAST
};

enum {
	PLATFORM_LIST_COL_LOGO,
	PLATFORM_LIST_COL_NAME,
	PLATFORM_LIST_COL_ID,
	PLATFORM_LIST_COL_LAST
};

typedef struct _GstPlatformDialogPrivate GstPlatformDialogPrivate;

struct _GstPlatformDialogPrivate
{
	GdkPixbuf *platforms [PLATFORM_LAST];
	GtkWidget *list;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;
};

enum {
	PROP_0,
	PROP_SESSION
};

static void gst_platform_dialog_class_init (GstPlatformDialogClass *class);
static void gst_platform_dialog_init       (GstPlatformDialog      *dialog);
static void gst_platform_dialog_finalize   (GObject                *object);

static void gst_platform_dialog_set_property (GObject      *object,
					      guint         prop_id,
					      const GValue *value,
					      GParamSpec   *pspec);

static GObject* gst_platform_dialog_constructor (GType                  type,
						 guint                  n_construct_properties,
						 GObjectConstructParam *construct_params);

static void gst_platform_dialog_response   (GtkDialog *dialog,
					    gint       response);

G_DEFINE_TYPE (GstPlatformDialog, gst_platform_dialog, GTK_TYPE_DIALOG);

static void
gst_platform_dialog_class_init (GstPlatformDialogClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (class);

	object_class->set_property = gst_platform_dialog_set_property;
	object_class->constructor  = gst_platform_dialog_constructor;
	object_class->finalize     = gst_platform_dialog_finalize;

	dialog_class->response     = gst_platform_dialog_response;

	g_object_class_install_property (object_class,
					 PROP_SESSION,
					 g_param_spec_object ("session",
							      "session",
							      "session",
							      OOBS_TYPE_SESSION,
							      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_type_class_add_private (object_class,
				  sizeof (GstPlatformDialogPrivate));
}

static void
on_selection_changed (GtkTreeSelection *selection,
		      gpointer          data)
{
	GstPlatformDialog *dialog;
	GstPlatformDialogPrivate *priv;
	gboolean selected;

	dialog = GST_PLATFORM_DIALOG (data);
	priv = dialog->_priv;

	selected = gtk_tree_selection_get_selected (selection, NULL, NULL);
	gtk_widget_set_sensitive (priv->ok_button, selected);
}

static GtkWidget*
gst_platform_dialog_create_treeview (GstPlatformDialog *dialog)
{
	GtkWidget *list;
	GtkListStore *store;
	GtkTreeModel *sort_model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;

	list = gtk_tree_view_new ();
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list), FALSE);

	store = gtk_list_store_new (PLATFORM_LIST_COL_LAST,
				    GDK_TYPE_PIXBUF,
				    G_TYPE_STRING,
				    G_TYPE_POINTER);

	sort_model = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sort_model),
					      PLATFORM_LIST_COL_NAME, GTK_SORT_ASCENDING);

	gtk_tree_view_set_model (GTK_TREE_VIEW (list), sort_model);
	g_object_unref (sort_model);

	column = gtk_tree_view_column_new ();
	
	/* Insert the pixmaps cell */
	renderer = gtk_cell_renderer_pixbuf_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);

	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer,
					    "pixbuf", PLATFORM_LIST_COL_LOGO);

	/* Insert the text cell */
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);

	gtk_tree_view_column_pack_end (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer,
					    "markup", PLATFORM_LIST_COL_NAME);

	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
	
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

	g_signal_connect (G_OBJECT (select), "changed",
			  G_CALLBACK (on_selection_changed), dialog);
	return list;
}

static void
gst_platform_dialog_init (GstPlatformDialog *dialog)
{
	GstPlatformDialogPrivate *priv;
	GtkWidget *box, *title, *label, *scrolled_window;
	gchar *str;

	priv = GST_PLATFORM_DIALOG_GET_PRIVATE (dialog);
	dialog->_priv = priv;

	priv->platforms [PLATFORM_REDHAT] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/redhat.png", NULL);
	priv->platforms [PLATFORM_DEBIAN] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/debian.png", NULL);
	priv->platforms [PLATFORM_MANDRAKE] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/mandrake.png", NULL);
	priv->platforms [PLATFORM_TURBOLINUX] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/turbolinux.png", NULL);
	priv->platforms [PLATFORM_SLACKWARE] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/slackware.png", NULL);
	priv->platforms [PLATFORM_SUSE] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/suse.png", NULL);
	priv->platforms [PLATFORM_FREEBSD] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/freebsd.png", NULL);
	priv->platforms [PLATFORM_GENTOO] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/gentoo.png", NULL);
	priv->platforms [PLATFORM_PLD] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/pld.png", NULL);
	priv->platforms [PLATFORM_OPENNA] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/openna.png", NULL);
	priv->platforms [PLATFORM_FEDORA] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/fedora.png", NULL);
	priv->platforms [PLATFORM_CONECTIVA] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/conectiva.png", NULL);
	priv->platforms [PLATFORM_BLACK_PANTHER] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/black_panther.png", NULL);
	priv->platforms [PLATFORM_VINE] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/vine.png", NULL);
	priv->platforms [PLATFORM_SPECIFIX] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/specifix.png", NULL);
	priv->platforms [PLATFORM_ARCHLINUX] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/archlinux.png", NULL);
	priv->platforms [PLATFORM_VIDALINUX] = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/vidalinux.png", NULL);

	box = gtk_vbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (box), 6);

	/* title label */
	title = gtk_label_new (NULL);
	gtk_label_set_line_wrap (GTK_LABEL (title), TRUE);
	gtk_misc_set_alignment (GTK_MISC (title), 0., 0.);
	str = g_strdup_printf ("<span weight='bold' size='larger'>%s</span>",
			       _("The platform you are running is not supported by this tool"));
	gtk_label_set_markup (GTK_LABEL (title), str);
	gtk_box_pack_start (GTK_BOX (box), title, FALSE, FALSE, 0);
	g_free (str);

	/* label */
	label = gtk_label_new (_("If you know for sure that it works like one of "
				 "the platforms listed below, you can select that "
				 "and continue. Note, however, that this might "
				 "damage the system configuration or downright "
				 "cripple your computer."));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0., 0.);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

	/* platforms list */
	priv->list = gst_platform_dialog_create_treeview (dialog);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);

	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->list);
	gtk_box_pack_start (GTK_BOX (box), scrolled_window, TRUE, TRUE, 0);
	gtk_widget_show_all (box);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), box);

	priv->cancel_button = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	priv->ok_button = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Unsupported platform"));
	gtk_window_set_default_size (GTK_WINDOW (dialog), 300, 400);
}

static void
gst_platform_dialog_finalize (GObject *object)
{
	GstPlatformDialog *dialog = GST_PLATFORM_DIALOG (object);
	GstPlatformDialogPrivate *priv = dialog->_priv;
	gint i = 0;

	if (dialog->session)
		g_object_unref (dialog->session);

	for (i = 0; i < PLATFORM_LAST; i++)
		g_object_unref (priv->platforms [i]);

	(* G_OBJECT_CLASS (gst_platform_dialog_parent_class)->finalize) (object);
}

static GdkPixbuf*
name_to_logo (GstPlatformDialogPrivate *priv,
	      const gchar              *name)
{
	g_return_val_if_fail (name != NULL, NULL);

	if (strcmp (name, "Debian GNU/Linux") == 0)
		return priv->platforms[PLATFORM_DEBIAN];
	else if (strcmp (name, "Red Hat Linux") == 0)
		return priv->platforms[PLATFORM_REDHAT];
	else if (strcmp (name, "OpenNA Linux") == 0)
		return priv->platforms[PLATFORM_OPENNA];
	else if (strcmp (name, "Linux Mandrake") == 0)
		return priv->platforms[PLATFORM_MANDRAKE];
	else if (strcmp (name, "Black Panther OS") == 0)
		return priv->platforms[PLATFORM_BLACK_PANTHER];
	else if (strcmp (name, "Conectiva Linux") == 0)
		return priv->platforms[PLATFORM_CONECTIVA];
	else if (strcmp (name, "SuSE Linux") == 0)
		return priv->platforms[PLATFORM_SUSE];
	else if (strcmp (name, "Turbolinux") == 0)
		return priv->platforms[PLATFORM_TURBOLINUX];
	else if (strcmp (name, "Slackware") == 0)
		return priv->platforms[PLATFORM_SLACKWARE];
	else if (strcmp (name, "FreeBSD") == 0)
		return priv->platforms[PLATFORM_FREEBSD];
	else if (strcmp (name, "Gentoo Linux") == 0)
		return priv->platforms[PLATFORM_GENTOO];
	else if (strcmp (name, "PLD") == 0)
		return priv->platforms[PLATFORM_PLD];
	else if (strcmp (name, "Vine Linux") == 0)
		return priv->platforms[PLATFORM_VINE];
	else if (strcmp (name, "Fedora Core") == 0)
		return priv->platforms[PLATFORM_FEDORA];
	else if (strcmp (name, "Specifix Linux") == 0)
		return priv->platforms[PLATFORM_SPECIFIX];
	else if (strcmp (name, "Arch Linux") == 0)
		return priv->platforms[PLATFORM_ARCHLINUX];
		
	return NULL;
}

static void
gst_platform_dialog_populate_list (GstPlatformDialog *dialog)
{
	GstPlatformDialogPrivate *priv;
	GtkTreeModel *sort_model;
	GtkListStore *store;
	GtkTreeIter iter;
	GList *platforms, *list;
	OobsPlatform *platform;
	GString *str;

	g_return_if_fail (OOBS_IS_SESSION (dialog->session));

	if (oobs_session_get_supported_platforms (dialog->session, &platforms) != OOBS_RESULT_OK)
		return;

	list = platforms;
	priv = dialog->_priv;
	sort_model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->list));
	store = GTK_LIST_STORE (gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (sort_model)));

	while (platforms) {
		platform = platforms->data;
		platforms = platforms->next;

		str = g_string_new (platform->name);

		if (platform->version)
			g_string_append_printf (str, " %s", platform->version);

		if (platform->codename)
			g_string_append_printf (str, " (<i>%s</i>)", platform->codename);

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
		                    PLATFORM_LIST_COL_LOGO, name_to_logo (priv, platform->name),
		                    PLATFORM_LIST_COL_NAME, str->str,
		                    PLATFORM_LIST_COL_ID, platform->id,
				    -1);

		g_string_free (str, TRUE);
	}

	g_list_free (list);
}

static GObject*
gst_platform_dialog_constructor (GType                  type,
				 guint                  n_construct_properties,
				 GObjectConstructParam *construct_params)
{
	GObject *object;

	object = (* G_OBJECT_CLASS (gst_platform_dialog_parent_class)->constructor) (type,
										     n_construct_properties,
										     construct_params);
	gst_platform_dialog_populate_list (GST_PLATFORM_DIALOG (object));

	return object;
}

static void
gst_platform_dialog_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
	GstPlatformDialog *dialog = GST_PLATFORM_DIALOG (object);

	switch (prop_id) {
	case PROP_SESSION:
		dialog->session = g_value_dup_object (value);
		break;
	}
}

static void
gst_platform_dialog_response (GtkDialog *dialog,
			      gint       response)
{
	if (response == GTK_RESPONSE_OK) {
		GstPlatformDialog *platform_dialog;
		GstPlatformDialogPrivate *priv;
		GtkTreeSelection *selection;
		GtkTreeModel *sort_model, *model;
		GtkTreeIter iter, child_iter;
		gchar *platform;

		platform_dialog = GST_PLATFORM_DIALOG (dialog);
		priv = platform_dialog->_priv;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->list));

		if (gtk_tree_selection_get_selected (selection, &sort_model, &iter)) {
			model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (sort_model));
			gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (sort_model),
									&child_iter, &iter);
			gtk_tree_model_get (model, &child_iter,
					    PLATFORM_LIST_COL_ID, &platform,
					    -1);

			oobs_session_set_platform (platform_dialog->session, platform);
			g_free (platform);
		}
	} else {
		exit (0);
	}
}

GtkWidget*
gst_platform_dialog_new (OobsSession *session)
{
	g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

	return g_object_new (GST_TYPE_PLATFORM_DIALOG,
			     "has-separator", FALSE,
			     "session", session,
			     NULL);
}
