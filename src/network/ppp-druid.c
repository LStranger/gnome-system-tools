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

#include "../common/global.h"
#include "ppp-druid.h"

#define PPP_DRUID_MAX_PAGES 5

typedef gchar *(*PppDruidCheckFunc) (PppDruid *);

static GtkWidget *my_get_widget (GladeXML *glade, const gchar *name)
{
	GtkWidget *w;

	g_return_val_if_fail (glade != NULL, NULL);
	
	w = glade_xml_get_widget (glade, name);
	if (!w)
		g_warning ("my_get_widget: Unexistent widget %s", name);

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

	/* Release allocated memory */
	
	/* What to do here is not defined yet. Probably just exit gtk_main. */
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

	if (error) {
		gtk_widget_show (w);
		
		widget_name = g_strdup_printf ("page%dlabel", ppp->current_page);
		w = my_get_widget (ppp->glade, widget_name);
		g_free (widget_name);
		gtk_label_set_text (GTK_LABEL (w), error);
	} else
		gtk_widget_hide (w);
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
	gchar *login, *passwd;

	g_return_val_if_fail (ppp != NULL, NULL);

	login = gtk_entry_get_text (GTK_ENTRY (ppp->login));
	passwd = gtk_entry_get_text (GTK_ENTRY (ppp->passwd));

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

static void ppp_druid_check_page (PppDruid *ppp)
{
	PppDruidCheckFunc func;
	PppDruidCheckFunc checks[] = {
		NULL,
		ppp_druid_check_phone,
		ppp_druid_check_login_pass,
		ppp_druid_check_profile,
		NULL
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
	PppDruid *ppp = (PppDruid *) data;

	ppp_druid_check_page (ppp);
}

static void ppp_druid_on_page_last_finish (GtkWidget *w, gpointer arg1, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	/* Apply the data to the xml tree, call backend */


	/* and quit */
	ppp_druid_exit (ppp);
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

extern PppDruid *ppp_druid_new (void)
{
	PppDruid *ppp;
	gchar *path;
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
		{ "page1",	"prepare",		GTK_SIGNAL_FUNC (ppp_druid_on_page_prepare) },
		{ "page2",	"prepare",		GTK_SIGNAL_FUNC (ppp_druid_on_page_prepare) },
		{ "page3",	"prepare",		GTK_SIGNAL_FUNC (ppp_druid_on_page_prepare) },
		{ "page_last",	"finish",			ppp_druid_on_page_last_finish },
		{ "phone",	"changed",		ppp_druid_on_entry_changed },
		{ "phone",	"activate",		ppp_druid_on_entry_activate },
		{ "login",	"changed",		ppp_druid_on_entry_changed },
		{ "login",	"activate",		ppp_druid_on_entry_activate },
		{ "passwd",	"changed",		ppp_druid_on_entry_changed },
		{ "passwd",	"activate",		ppp_druid_on_entry_activate },
		{ "profile",	"changed",		ppp_druid_on_entry_changed },
		{ "profile",	"activate",		ppp_druid_on_entry_activate },
		{ NULL }
	};


	ppp = g_new0 (PppDruid, 1);

	path = g_strdup_printf ("%s/internet_connection_druid.glade", INTERFACES_DIR);
	
	ppp->glade = glade_xml_new (path, NULL);
	
	g_free (path);
	if (!ppp->glade) {
		g_free (ppp);
		return NULL;
	}

	ppp->error_state = FALSE;
	ppp->current_page = 0;

	ppp->win = my_get_widget (ppp->glade, "window");
	ppp->druid = GNOME_DRUID (my_get_widget (ppp->glade, "druid"));
	
	ppp->phone = my_get_widget (ppp->glade, "phone");
	ppp->login = my_get_widget (ppp->glade, "login");
	ppp->passwd = my_get_widget (ppp->glade, "passwd");
	ppp->profile = my_get_widget (ppp->glade, "profile");
	ppp_druid_connect_signals (ppp, signals);

	return ppp;
}	

extern void ppp_druid_show (PppDruid *ppp)
{
	g_return_if_fail (ppp != NULL);
	
	gtk_widget_show_all (ppp->win);
}


