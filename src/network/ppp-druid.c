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
 * Authors: Arturo Espinosa <arturo@ximian.com>
 */

#include <config.h>

#include <string.h>
#include <ctype.h>

#include <gnome.h>
#include <glade/glade.h>

#include "gst.h"
#include "ppp-druid.h"
#include "callbacks.h"

static GtkWidget *
my_get_widget (GladeXML *glade, const gchar *name)
{
	GtkWidget *w;
	gchar *s;

	g_return_val_if_fail (glade != NULL, NULL);

	s = g_strdup_printf ("ppp_druid_%s", name);
	w = glade_xml_get_widget (glade, s);
	if (!w)
		g_warning ("my_get_widget: Unexistent widget %s", s);

	g_free (s);

	return w;
}

static void
ppp_druid_connect_signals (PppDruid *ppp, GstDialogSignal *signals)
{
	gint i;

	g_return_if_fail (ppp != NULL);

	for (i = 0; signals[i].widget; i++)
		g_signal_connect_after (G_OBJECT (my_get_widget (ppp->glade, signals[i].widget)),
							 signals[i].signal_name, signals[i].func, ppp);
}

void
ppp_druid_exit (PppDruid *ppp)
{
	g_return_if_fail (ppp != NULL);

	g_object_unref (G_OBJECT (ppp->glade));
	gtk_widget_destroy (ppp->win);
	gtk_main_quit ();
}

static gchar *
ppp_druid_get_serial_port (PppDruid *ppp)
{
	gchar *port;
	xmlNode *node;

	node = gst_xml_doc_get_root (ppp->tool->config);
	port = connection_get_serial_port_from_node (node, "Defaults");

	if (!port)
		port = g_strdup ("/dev/modem");

	return port;
}

void
ppp_druid_save (PppDruid *ppp)
{
	xmlNode *root;
	GstConnection *cxn;
	
	g_return_if_fail (ppp != NULL);

	root = gst_xml_doc_get_root (ppp->tool->config);
	cxn = connection_new_from_type (GST_CONNECTION_PPP, root);
	ppp->cxn = cxn;

	cxn->serial_port = ppp_druid_get_serial_port (ppp);
	cxn->phone_number = g_strdup (gtk_entry_get_text (GTK_ENTRY (ppp->phone)));
	cxn->login = g_strdup (gtk_entry_get_text (GTK_ENTRY (ppp->login)));
	cxn->password = g_strdup (gtk_entry_get_text (GTK_ENTRY (ppp->passwd)));
	cxn->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (ppp->profile)));
	
	cxn->wvsection = connection_wvsection_name_generate (cxn->dev, root);
	cxn->persist = FALSE;
	cxn->set_default_gw = TRUE;
	cxn->update_dns = TRUE;
	cxn->user = TRUE;

	gtk_signal_emit_by_name (GTK_OBJECT (ppp->tool->main_dialog), "apply", ppp->tool);
	ppp_druid_exit (ppp);
}

GtkWidget *
ppp_druid_get_button_next (PppDruid *ppp)
{
	g_return_val_if_fail (ppp != NULL, NULL);
	g_return_val_if_fail (ppp->druid != NULL, NULL);
	
	if (GTK_WIDGET_MAPPED (ppp->druid->next))
		return ppp->druid->next;
	if (GTK_WIDGET_MAPPED (ppp->druid->finish))
		return ppp->druid->finish;

	g_warning ("ppp_druid_get_button_next: we shouldn't be here.");
	return NULL;
}

/* If error == NULL, there's no error, and we clear the error message. */
static void
ppp_druid_set_error (PppDruid *ppp, gchar *error)
{
	gchar *widget_name;
	GtkWidget *w;
	
	g_return_if_fail (ppp != NULL);
	g_return_if_fail (ppp->current_page > 0);

	ppp->error_state = (error)? TRUE : FALSE;
	gtk_widget_set_sensitive (ppp_druid_get_button_next (ppp), (error)? FALSE: TRUE);

	widget_name = g_strdup_printf ("page%dwarning", ppp->current_page);
	w = my_get_widget (ppp->glade, widget_name);
	g_free (widget_name);

	if (w) {
		if (error) {
			gtk_widget_show (w);
			
			widget_name = g_strdup_printf ("page%dlabel", ppp->current_page);
			w = my_get_widget (ppp->glade, widget_name);
			g_free (widget_name);
			gtk_label_set_text (GTK_LABEL (w), error);
		} else
			gtk_widget_hide (w);
	}
}

static gchar *
ppp_druid_check_phone (PppDruid *ppp)
{
	const gchar *phone;
	gchar *valid = "0123456789,";
	int i, len;

	g_return_val_if_fail (ppp != NULL, NULL);

	phone = gtk_entry_get_text (GTK_ENTRY (ppp->phone));

	len = strlen (phone);
	for (i = 0; i < len; i++)
		if (!strchr (valid, phone[i]))
			break;

	if (len == 0 || i < len)
		return _("The phone number must be composed of\nonly numbers or commas (,).");

	return NULL;
}

static gchar *
ppp_druid_check_login_pass (PppDruid *ppp)
{
	const gchar *login, *passwd, *passwd2;

	g_return_val_if_fail (ppp != NULL, NULL);

	login   = gtk_entry_get_text (GTK_ENTRY (ppp->login));
	passwd  = gtk_entry_get_text (GTK_ENTRY (ppp->passwd));
	passwd2 = gtk_entry_get_text (GTK_ENTRY (ppp->passwd2));

	if (strcmp (passwd, passwd2))
		return _("The password and its confirmation\nmust be the same.");

	if (*passwd && !*login)
		return _("If you are specifying a password,\nyou should specify a user name.");

	return NULL;
}

static gchar *
ppp_druid_check_profile (PppDruid *ppp)
{
	const gchar *profile;
	gint i, len;

	g_return_val_if_fail (ppp != NULL, NULL);

	profile = gtk_entry_get_text (GTK_ENTRY (ppp->profile));

	len = strlen (profile);
	for (i = 0; i < len; i++)
		if (!isalnum (profile[i]) && profile[i] != ' ')
			break;

	/* FIXME: we should also check that the profile name is unique. */
	
	if (len == 0 || i < len)
		return _("Please set a name for this account,\nusing only space, letters and numbers.");

	return NULL;
}

static gchar *
ppp_druid_check_last (PppDruid *ppp)
{
	GtkWidget *w;
	gint i;
	gchar *text;
	const gchar *phone, *login, *profile;
	gchar *passwd;
	gchar *format =
		_("You are about to create an account named with the following information:\n\n"
		  "Account Name: %s\n\n"
		  "Phone number: %s\n\n"
		  "User name: %s\n\n"
		  "Password: %s");

	w = my_get_widget (ppp->glade, "page_last");

	phone = gtk_entry_get_text (GTK_ENTRY (ppp->phone));
	login = gtk_entry_get_text (GTK_ENTRY (ppp->login));
	passwd = g_strdup (gtk_entry_get_text (GTK_ENTRY (ppp->passwd)));
	profile = gtk_entry_get_text (GTK_ENTRY (ppp->profile));

	for (i = 0; i < strlen (passwd); i++)
		passwd[i] = '*';

	text = g_strdup_printf (format, profile, phone, login, passwd);

	gnome_druid_page_edge_set_text (GNOME_DRUID_PAGE_EDGE (w), text);

	g_free (text);
	g_free (passwd);

	return NULL;
}

void
ppp_druid_check_page (PppDruid *ppp)
{
	PppDruidCheckFunc func;
	PppDruidCheckFunc checks[] = {
		NULL,
		ppp_druid_check_phone,
		ppp_druid_check_login_pass,
		ppp_druid_check_profile,
		ppp_druid_check_last
	};
	
	g_return_if_fail (ppp != NULL);
	g_return_if_fail (ppp->current_page < PPP_DRUID_MAX_PAGES);

	func = checks[ppp->current_page];

	if (func)
		ppp_druid_set_error (ppp, (func) (ppp));
}

PppDruid *
ppp_druid_new (GstTool *tool)
{
	PppDruid *ppp;
	GstDialogSignal signals[] = {
		{ "window",	"delete_event",	G_CALLBACK (ppp_druid_on_window_delete_event) },
		{ "druid",	"cancel",	G_CALLBACK (ppp_druid_on_druid_cancel) },
		{ "page0",	"next",		G_CALLBACK (ppp_druid_on_page_next) },
		{ "page1",	"next",		G_CALLBACK (ppp_druid_on_page_next) },
		{ "page2",	"next",		G_CALLBACK (ppp_druid_on_page_next) },
		{ "page3",	"next",		G_CALLBACK (ppp_druid_on_page_next) },
		{ "page1",	"back",		G_CALLBACK (ppp_druid_on_page_back) },
		{ "page2",	"back",		G_CALLBACK (ppp_druid_on_page_back) },
		{ "page3",	"back",		G_CALLBACK (ppp_druid_on_page_back) },
		{ "page_last",	"back",		G_CALLBACK (ppp_druid_on_page_back) },
		{ "page0",	"prepare",	G_CALLBACK (ppp_druid_on_page_prepare) },
		{ "page1",	"prepare",	G_CALLBACK (ppp_druid_on_page_prepare) },
		{ "page2",	"prepare",	G_CALLBACK (ppp_druid_on_page_prepare) },
		{ "page3",	"prepare",	G_CALLBACK (ppp_druid_on_page_prepare) },
		{ "page_last",	"prepare",	G_CALLBACK (ppp_druid_on_page_prepare) },
		{ "page_last",	"finish",	G_CALLBACK (ppp_druid_on_page_last_finish) },
		{ "phone",	"changed",	G_CALLBACK (ppp_druid_on_entry_changed) },
		{ "phone",	"activate",	G_CALLBACK (ppp_druid_on_entry_activate) },
		{ "login",	"changed",	G_CALLBACK (ppp_druid_on_entry_changed) },
		{ "login",	"activate",	G_CALLBACK (ppp_druid_on_login_activate) },
		{ "passwd",	"changed",	G_CALLBACK (ppp_druid_on_entry_changed) },
		{ "passwd",	"activate",	G_CALLBACK (ppp_druid_on_passwd_activate) },
		{ "passwd2",	"changed",	G_CALLBACK (ppp_druid_on_entry_changed) },
		{ "passwd2",	"activate",	G_CALLBACK (ppp_druid_on_entry_activate) },
		{ "profile",	"changed",	G_CALLBACK (ppp_druid_on_entry_changed) },
		{ "profile",	"activate",	G_CALLBACK (ppp_druid_on_entry_activate) },
		{ NULL }
	};


	ppp = g_new0 (PppDruid, 1);
	
	ppp->glade = tool->main_dialog->gui;
	ppp->tool  = tool;

	ppp->error_state  = FALSE;
	ppp->current_page = 0;
	ppp->druid        = GNOME_DRUID (my_get_widget (ppp->glade, "druid"));
	ppp->win          = my_get_widget (ppp->glade, "window");
	g_object_set_data (G_OBJECT (ppp->win), "ppp", ppp);
	
	ppp->phone   = my_get_widget (ppp->glade, "phone");
	ppp->login   = my_get_widget (ppp->glade, "login");
	ppp->passwd  = my_get_widget (ppp->glade, "passwd");
	ppp->passwd2 = my_get_widget (ppp->glade, "passwd2");
	ppp->profile = my_get_widget (ppp->glade, "profile");

	ppp_druid_connect_signals (ppp, signals);

	return ppp;
}	

void
ppp_druid_show (PppDruid *ppp)
{
	g_return_if_fail (ppp != NULL);
	
	gtk_widget_show_all (ppp->win);
}

void
ppp_druid_gui_to_xml(GstTool *tool, gpointer data)
{
	PppDruid *ppp = data;
	xmlNode *root = gst_xml_doc_get_root (tool->config);

	connection_save_to_node (ppp->cxn, root);
}
