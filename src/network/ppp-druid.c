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

#include "xst.h"
#include "ppp-druid.h"

#define PPP_DRUID_MAX_PAGES 5

typedef gchar *(*PppDruidCheckFunc) (PppDruid *);

static GtkWidget *my_get_widget (GladeXML *glade, const gchar *name)
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

static void ppp_druid_connect_signals (PppDruid *ppp, XstDialogSignal *signals)
{
	gint i;

	g_return_if_fail (ppp != NULL);

	for (i = 0; signals[i].widget; i++)
		gtk_signal_connect_after (GTK_OBJECT (my_get_widget (ppp->glade, signals[i].widget)),
							 signals[i].signal_name, signals[i].func, ppp);
}

static void ppp_druid_exit (PppDruid *ppp)
{
	g_return_if_fail (ppp != NULL);

	gtk_object_unref (GTK_OBJECT (ppp->glade));
	gtk_widget_destroy (ppp->win);
	gtk_main_quit ();
}

static gchar *ppp_druid_get_serial_port (PppDruid *ppp)
{
	gchar *port;
	xmlNode *node;

	node = xst_xml_doc_get_root (ppp->tool->config);
	port = connection_get_serial_port_from_node (node, "Defaults");

	if (!port)
		port = g_strdup ("/dev/modem");

	return port;
}

static void ppp_druid_save (PppDruid *ppp)
{
	xmlNode *root;
	XstConnection *cxn;
	
	g_return_if_fail (ppp != NULL);

	root = xst_xml_doc_get_root (ppp->tool->config);
	cxn = connection_new_from_type (XST_CONNECTION_PPP, root);
	ppp->cxn = cxn;

	cxn->serial_port = ppp_druid_get_serial_port (ppp);
	cxn->phone_number = g_strdup (gtk_entry_get_text (GTK_ENTRY (ppp->phone)));
	cxn->login = g_strdup (gtk_entry_get_text (GTK_ENTRY (ppp->login)));
	cxn->password = g_strdup (gtk_entry_get_text (GTK_ENTRY (ppp->passwd)));
	cxn->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (ppp->profile)));
	
	cxn->wvsection = connection_wvsection_name_generate (cxn->dev, root);
	cxn->persist = FALSE;
	cxn->set_default_gw = TRUE;
	cxn->peerdns = TRUE;
	cxn->user = TRUE;

	gtk_signal_emit_by_name (GTK_OBJECT (ppp->tool->main_dialog), "apply", ppp->tool);
	ppp_druid_exit (ppp);
}

static GtkWidget *ppp_druid_get_button_next (PppDruid *ppp)
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
static void ppp_druid_set_error (PppDruid *ppp, gchar *error)
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

static gchar *ppp_druid_check_phone (PppDruid *ppp)
{
	gchar *phone;
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

static gchar *ppp_druid_check_login_pass (PppDruid *ppp)
{
	gchar *login, *passwd, *passwd2;

	g_return_val_if_fail (ppp != NULL, NULL);

	login = gtk_entry_get_text (GTK_ENTRY (ppp->login));
	passwd = gtk_entry_get_text (GTK_ENTRY (ppp->passwd));
	passwd2 = gtk_entry_get_text (GTK_ENTRY (ppp->passwd2));

	if (strcmp (passwd, passwd2))
		return _("The password and its confirmation\nmust be the same.");

	if (*passwd && !*login)
		return _("If you are specifying a password,\nyou should specify a user name.");

	return NULL;
}

static gchar *ppp_druid_check_profile (PppDruid *ppp)
{
	gchar *profile;
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

static gchar *ppp_druid_check_last (PppDruid *ppp)
{
	GtkWidget *w;
	gint i;
	gchar *text;
	gchar *phone, *login, *passwd, *profile;
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

	gnome_druid_page_finish_set_text (GNOME_DRUID_PAGE_FINISH (w), text);

	g_free (text);
	g_free (passwd);

	return NULL;
}

static void ppp_druid_check_page (PppDruid *ppp)
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

static void ppp_druid_on_window_delete_event (GtkWidget *w, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	ppp_druid_exit (ppp);
}

static void ppp_druid_on_druid_cancel (GtkWidget *w, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	ppp_druid_exit (ppp);
}

static gboolean ppp_druid_on_page_next (GtkWidget *w, gpointer arg1, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	ppp->current_page++;

	return FALSE;
}

static gboolean ppp_druid_on_page_back (GtkWidget *w, gpointer arg1, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	ppp->current_page--;

	return FALSE;
}

static void ppp_druid_on_page_prepare (GtkWidget *w, gpointer arg1, gpointer data)
{
	gchar *next_focus[] = {
		NULL, "phone", "login", "profile", NULL
	};
	
	PppDruid *ppp = (PppDruid *) data;

	ppp_druid_check_page (ppp);

	if (next_focus[ppp->current_page])
		gtk_widget_grab_focus (my_get_widget (ppp->glade, next_focus[ppp->current_page]));
	else
		gtk_widget_grab_focus (ppp_druid_get_button_next (ppp));
}

static void ppp_druid_on_page_last_finish (GtkWidget *w, gpointer arg1, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	ppp_druid_save (ppp);
}

static void ppp_druid_on_entry_changed (GtkWidget *w, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	ppp_druid_check_page (ppp);
}

static void ppp_druid_on_entry_activate (GtkWidget *w, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	if (!ppp->error_state)
		gtk_widget_grab_focus (ppp_druid_get_button_next (ppp));
}

static void ppp_druid_on_login_activate (GtkWidget *w, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	gtk_widget_grab_focus (ppp->passwd);
}

static void ppp_druid_on_passwd_activate (GtkWidget *w, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	gtk_widget_grab_focus (ppp->passwd2);
}

extern PppDruid *ppp_druid_new (XstTool *tool)
{
	PppDruid *ppp;
	XstDialogSignal signals[] = {
		{ "window",	"delete_event",	ppp_druid_on_window_delete_event },
		{ "druid",	"cancel",			ppp_druid_on_druid_cancel },
		{ "page0",	"next",			GTK_SIGNAL_FUNC (ppp_druid_on_page_next) },
		{ "page1",	"next",			GTK_SIGNAL_FUNC (ppp_druid_on_page_next) },
		{ "page2",	"next",			GTK_SIGNAL_FUNC (ppp_druid_on_page_next) },
		{ "page3",	"next",			GTK_SIGNAL_FUNC (ppp_druid_on_page_next) },
		{ "page1",	"back",			GTK_SIGNAL_FUNC (ppp_druid_on_page_back) },
		{ "page2",	"back",			GTK_SIGNAL_FUNC (ppp_druid_on_page_back) },
		{ "page3",	"back",			GTK_SIGNAL_FUNC (ppp_druid_on_page_back) },
		{ "page_last",	"back",			GTK_SIGNAL_FUNC (ppp_druid_on_page_back) },
		{ "page0",	"prepare",		GTK_SIGNAL_FUNC (ppp_druid_on_page_prepare) },
		{ "page1",	"prepare",		GTK_SIGNAL_FUNC (ppp_druid_on_page_prepare) },
		{ "page2",	"prepare",		GTK_SIGNAL_FUNC (ppp_druid_on_page_prepare) },
		{ "page3",	"prepare",		GTK_SIGNAL_FUNC (ppp_druid_on_page_prepare) },
		{ "page_last",	"prepare",		GTK_SIGNAL_FUNC (ppp_druid_on_page_prepare) },
		{ "page_last",	"finish",			ppp_druid_on_page_last_finish },
		{ "phone",	"changed",		ppp_druid_on_entry_changed },
		{ "phone",	"activate",		ppp_druid_on_entry_activate },
		{ "login",	"changed",		ppp_druid_on_entry_changed },
		{ "login",	"activate",		ppp_druid_on_login_activate },
		{ "passwd",	"changed",		ppp_druid_on_entry_changed },
		{ "passwd",	"activate",		ppp_druid_on_passwd_activate },
		{ "passwd2",	"changed",		ppp_druid_on_entry_changed },
		{ "passwd2",	"activate",		ppp_druid_on_entry_activate },
		{ "profile",	"changed",		ppp_druid_on_entry_changed },
		{ "profile",	"activate",		ppp_druid_on_entry_activate },
		{ NULL }
	};


	ppp = g_new0 (PppDruid, 1);
	ppp->glade = tool->main_dialog->gui;
	ppp->tool = tool;

	ppp->error_state = FALSE;
	ppp->current_page = 0;

	ppp->win = my_get_widget (ppp->glade, "window");
	gtk_object_set_data (GTK_OBJECT (ppp->win), "ppp", ppp);
	ppp->druid = GNOME_DRUID (my_get_widget (ppp->glade, "druid"));
	
	ppp->phone = my_get_widget (ppp->glade, "phone");
	ppp->login = my_get_widget (ppp->glade, "login");
	ppp->passwd = my_get_widget (ppp->glade, "passwd");
	ppp->passwd2 = my_get_widget (ppp->glade, "passwd2");
	ppp->profile = my_get_widget (ppp->glade, "profile");
	ppp_druid_connect_signals (ppp, signals);

	return ppp;
}	

extern void ppp_druid_show (PppDruid *ppp)
{
	g_return_if_fail (ppp != NULL);
	
	gtk_widget_show_all (ppp->win);
}

void ppp_druid_gui_to_xml(XstTool *tool, gpointer data)
{
	PppDruid *ppp = data;
	xmlNode *root = xst_xml_doc_get_root (tool->config);

	connection_save_to_node (ppp->cxn, root);
}
