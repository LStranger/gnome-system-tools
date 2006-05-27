/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2005 Carlos Garnacho.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro <carlosg@gnome.org>
 */

#include "ntp-servers-list.h"
#include "time-tool.h"
#include "gst.h"

enum {
	COL_ACTIVE,
	COL_DESC,
	COL_URL,
	COL_ITER,
	COL_LAST
};

struct NtpServer {
	const gchar *url;
	const gchar *location;
} ntp_servers [] = {
	{ "time.nrc.ca", "Canada" },
	{ "ntp1.cmc.ec.gc.ca", "Eastern Canada" },
	{ "ntp2.cmc.ec.gc.ca", "Eastern Canada" },
	{ "clock.tricity.wsu.edu", "Washington, USA" },
	{ "wuarchive.wustl.edu", "Missouri, USA" },
	{ "clock.psu.edu", "Pennsylvania, USA" },
	{ "constellation.ecn.uoknor.edu", "Oklahoma, USA" },
	{ "gilbreth.ecn.purdue.edu", "Indiana, USA" },
	{ "molecule.ecn.purdue.edu", "Indiana, USA" },
	{ "libra.rice.edu", "Texas, USA" },
	{ "ntp.cox.smu.edu", "Texas, USA" },
	{ "ntp.tmc.edu", "Texas, USA" },
	{ "louie.udel.edu", "Delaware, USA" },
	{ "ntp.cmr.gov", "Virginia, USA" },
	{ "ntp0.cornell.edu", "New York, USA" },
	{ "ntp-0.cso.uiuc.edu", "Illinois, USA" },
	{ "ntp1.cs.wisc.edu", "Wisconsin, USA" },
	{ "tick.cs.unlv.edu", "Las Vegas, USA" },
	{ "ntp2a.mcc.ac.uk", "England, Europe" },
	{ "ntp2b.mcc.ac.uk", "England, Europe" },
	{ "ntp.maths.tcd.ie", "Ireland, Europe" },
	{ "ntp.cs.strath.ac.uk", "Scotland, Europe" },
	{ "bernina.ethz.ch", "Switzerland, Europe" },
	{ "ntp.univ-lyon1.fr", "France, Europe" },
	{ "tick.keso.fi", "Finland, Europe" },
	{ "fartein.ifi.uio.no", "Norway, Europe" },
	{ "ntp1.arnes.si", "Slovenia, Europe" },
	{ "ntp2.arnes.si", "Slovenia, Europe" },
	{ "ntp.landau.ac.ru", "Moscow, Russia" },
	{ "time.esec.com.au", "Australia" },
	{ "ntp.adelaide.edu.au", "South Australia" },
	{ "ntp.shim.org", "Singapore, Asia" },
	{ "time.nuri.net", "Korea, Asia" },
	{ "ntp.cs.mu.oz.au", "Melbourne, Australia" },
	{ "ntp.mel.nml.csiro.au", "Melbourne, Australia" },
	{ "ntp.nml.csiro.au", "Sydney, Australia" },
	{ "ntp.per.nml.csiro.au", "Perth, Australia" },
	{ "swisstime.ethz.ch", "Zurich, Switzerland" },
	{ "ntp.cesnet.cz", "Prague, Czech Republic" },
	{ "ntpa2.kph.uni-mainz.de", "Mainz, Germany" },
	{ "ntps1-0.cs.tu-berlin.de", "Berlin, Germany" },
	{ "ntps1-1.cs.tu-berlin.de", "Berlin, Germany" },
	{ "ntps1-2.uni-erlangen.de", "Erlangen, Germany" },
	{ "canon.inria.fr", "Rocquencourt, France" },
	{ "chronos.cru.fr", "Britany, France" },
	{ "stdtime.gov.hk", "Hong Kong, China" },
	{ "clock.cuhk.edu.hk", "Hong Kong, China" },
	{ "time.ien.it", "Torino, Italy" },
	{ "ntp.jst.mfeed.ad.jp", "Japan" },
	{ "cronos.cenam.mx", "Queretaro, Mexico" },
	{ "ntp0.nl.net", "Amsterdam, The Netherlands" },
	{ "ntp1.nl.net", "Amsterdam, The Netherlands" },
	{ "ntp2.nl.net", "Amsterdam, The Netherlands" },
	{ "time.service.uit.no", "Norway" },
	{ "ntp.certum.pl", "Poland" },
	{ "vega.cbk.poznan.pl", "Borowiec, Poland" },
	{ "time1.stupi.se", "Stockholm, Sweden" },
	{ "goodtime.ijs.si", "Ljubljana, Slovenia" },
	{ "ntp2.ja.net", "United Kingdom" },
	{ NULL }
};

static void
populate_model (GtkListStore *store)
{
	GtkTreeIter iter;
	gchar *str;
	gint i;

	for (i = 0; ntp_servers[i].url; i++) {
		str = g_strdup_printf ("%s (%s)", ntp_servers[i].url, ntp_servers[i].location);

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COL_ACTIVE, FALSE,
				    COL_DESC, str,
				    COL_URL, ntp_servers[i].url,
				    COL_ITER, NULL,
				    -1);
		g_free (str);
	}
}

static GtkTreeModel*
create_model (void)
{
	GtkListStore *store;

	store = gtk_list_store_new (COL_LAST,
				    G_TYPE_BOOLEAN,
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    OOBS_TYPE_LIST_ITER);
	return GTK_TREE_MODEL (store);
}

static void
on_server_toggled (GtkCellRendererToggle *renderer,
		   gchar		  *path_str,
		   gpointer		   data)
{
	GtkListStore *store = (GtkListStore *) data;
	GtkTreePath *path  = gtk_tree_path_new_from_string (path_str);
	GstTimeTool *tool;
	OobsList *list;
	OobsListIter *list_iter;
	GtkTreeIter iter;
	gchar *url;

	if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path)) {
		g_critical ("could not get list iter");
		return;
	}

	tool = g_object_get_data (G_OBJECT (renderer), "tool");
	list = oobs_ntp_config_get_servers (tool->ntp_config);

	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
			    COL_URL, &url,
			    COL_ITER, &list_iter,
			    -1);

	if (list_iter) {
		oobs_list_remove (list, list_iter);

		gtk_list_store_set (store, &iter,
				    COL_ACTIVE, FALSE,
				    COL_ITER, NULL,
				    -1);
	} else {
		GObject *server;
		OobsListIter new_list_iter;

		server = (GObject*) oobs_ntp_server_new (url);
		oobs_list_append (list, &new_list_iter);
		oobs_list_set (list, &new_list_iter, server);

		gtk_list_store_set (store, &iter,
				    COL_ACTIVE, TRUE,
				    COL_ITER, &new_list_iter,
				    -1);
	}

	oobs_object_commit (tool->ntp_config);
	g_free (url);
}

static void
add_columns (GtkTreeView  *list,
	     GtkTreeModel *model,
	     GstTimeTool  *tool)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	column = gtk_tree_view_column_new ();

	renderer = gtk_cell_renderer_toggle_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "active", COL_ACTIVE);

	g_object_set_data (G_OBJECT (renderer), "tool", tool);
	g_signal_connect (G_OBJECT (renderer), "toggled", G_CALLBACK (on_server_toggled), model);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_end (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text", COL_DESC);

	gtk_tree_view_append_column (list, column);
}

GtkWidget*
ntp_servers_list_get (GstTimeTool *tool)
{
	GtkWidget *list;
	GtkTreeModel *model;

	list = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "ntp_list");
	model = create_model ();
	populate_model (GTK_LIST_STORE (model));

	add_columns (GTK_TREE_VIEW (list), model, tool);

	gtk_tree_view_set_model (GTK_TREE_VIEW (list), model);
	g_object_unref (model);

	return list;
}

void
ntp_servers_list_check (GtkWidget     *ntp_list,
			OobsListIter  *list_iter,
			OobsNTPServer *server)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid, found;
	const gchar *str, *address;

	address = oobs_ntp_server_get_hostname (server);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ntp_list));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	found = FALSE;

	while (valid && !found) {
		gtk_tree_model_get (model, &iter,
				    COL_URL, &str,
				    -1);

		if (strcmp (address, str) == 0) {
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    COL_ACTIVE, TRUE,
					    COL_ITER, list_iter,
					    -1);
			found = TRUE;
		}

		valid = gtk_tree_model_iter_next (model, &iter);
	}

	if (!found) {
		/* append the new server */
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    COL_ACTIVE, TRUE,
				    COL_DESC, address,
				    COL_URL, address,
				    COL_ITER, list_iter,
				    -1);
	}
}
