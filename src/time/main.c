#include "config.h"

#include <stdio.h>

#include <gnome.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "global.h"

#include "tz.h"
#include "timeserv.h"
#include "transfer.h"
#include "e-map/e-map.h"
#include "tz-map.h"

ETzMap *tzmap;


void init_map (void);
void populate_ntp_list(void);
gint clock_tick(gpointer data);
void on_apply_clicked(GtkButton *button, gpointer data);
void on_cancel_clicked(GtkButton *button, gpointer data);
void on_help_clicked(GtkButton *button, gpointer data);
void delete_event (GtkWidget * widget, GdkEvent * event, gpointer gdata);
void connect_signals(void);


TransStringSpin transfer_string_spin_table[] =
{
	{ "hour", "hour" },
	{ "minute", "minute" },
	{ "second", "second" },
	{ 0, 0 }
};


TransStringCalendar transfer_string_calendar_table[] =
{
	{ "year", "month", "monthday", /* <-> */ "calendar" },
	{ 0, 0, 0, 0 }
};


TransTree trans_tree =
{
	0,
	0,
	0,
	0,
	transfer_string_calendar_table,
	transfer_string_spin_table
};


char *ntp_servers[] =
{
	"time.nrc.ca (Canada)",
	"ntp1.cmc.ec.gc.ca (Eastern Canada)",
	"ntp2.cmc.ec.gc.ca (Eastern Canada)",
	"clock.tricity.wsu.edu (Washington, USA)",
	"wuarchive.wustl.edu (Washington, USA)",
	"clock.psu.edu (Pennsylvania, USA)",
	"constellation.ecn.uoknor.edu (Oklahoma, USA)",
	"gilbreth.ecn.purdue.edu (Indiana, USA)",
	"molecule.ecn.purdue.edu (Indiana, USA)",
	"libra.rice.edu (Texas, USA)",
	"ntp.cox.smu.edu (Texas, USA)",
	"ntp.tmc.edu (Texas, USA)",
	"louie.udel.edu (Delaware, USA)",
	"ntp.cmr.gov (Virginia, USA)",
	"ntp0.cornell.edu (New York, USA)",
	"ntp-0.cso.uiuc.edu (Illinois, USA)",
	"ntp1.cs.wisc.edu (Wisconsin, USA)",
	"tick.cs.unlv.edu (Las Vegas, USA)",
	"ntp2a.mcc.ac.uk (England, Europe)",
	"ntp2b.mcc.ac.uk (England, Europe)",
	"salmon.maths.tcd.ie (Ireland, Europe)",
	"ntp.cs.strath.ac.uk (Scotland, Europe)",
	"bernina.ethz.ch (Switzerland, Europe)",
	"ntp.univ-lyon1.fr (France, Europe)",
	"tick.keso.fi (Finland, Europe)",
	"fartein.ifi.uio.no (Norway, Europe)",
	"ntp1.arnes.si (Slovenia, Europe)",
	"ntp2.arnes.si (Slovenia, Europe)",
	"ntp.landau.ac.ru (Moscow, Russia)",
	"time.esec.com.au (Australia)",
	"ntp.adelaide.edu.au (South Australia)",
	"ntp.shim.org (Singapore, Asia)",
	"time.nuri.net (Korea, Asia)",
	0
};


void
populate_ntp_list ()
{
	GtkWidget *ntp_list, *item;
	GList *list_add = 0;
	int i;
	
	ntp_list = tool_widget_get("ntp_list");
	if (!ntp_list) return;  /* Broken interface file */
	
	for (i = 0; ntp_servers[i]; i++)
	{
		item = gtk_list_item_new_with_label(ntp_servers[i]);
		gtk_widget_show(item);
		list_add = g_list_append(list_add, item);
	}
	
	gtk_list_append_items(GTK_LIST(ntp_list), list_add);
}


void
init_map ()
{
	tzmap = e_tz_map_new ();
	gtk_container_add (GTK_CONTAINER (tool_widget_get ("map_window")),
			   GTK_WIDGET (tzmap->map));
}


gint
clock_tick(gpointer data)
{
	GtkWidget *w;

	tool_set_frozen(TRUE);
  
	w = tool_widget_get("second");
	gtk_spin_button_spin(GTK_SPIN_BUTTON(w), GTK_SPIN_STEP_FORWARD, 1.0);
  
	if (!gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w)))
	{
		w = tool_widget_get("minute");
		gtk_spin_button_spin(GTK_SPIN_BUTTON(w), GTK_SPIN_STEP_FORWARD, 1.0);
		
		if (!gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w)))
		{
			w = tool_widget_get("hour");
			gtk_spin_button_spin(GTK_SPIN_BUTTON(w), GTK_SPIN_STEP_FORWARD, 1.0);
			
			/* FIXME: Update calendar if we pass midnight */
		}
	}
	
	tool_set_frozen(FALSE);
	
	return(TRUE);
}


void
on_apply_clicked(GtkButton *button, gpointer data)
{
	transfer_gui_to_xml(&trans_tree, xml_doc_get_root(tool_config_get_xml()));
	tool_config_save();
	tool_set_modified(FALSE);
}


void
on_help_clicked(GtkButton *button, gpointer data)
{
	/* TODO: Have some help handy */
}


void
connect_signals()
{
#if 0
	GtkWidget *w;

	w = GTK_COMBO(tool_widget_get("timezone_combo"))->list;
	gtk_signal_connect(GTK_OBJECT(w), "select-child", tz_select_combo, NULL);
	gtk_signal_connect(GTK_OBJECT(w), "select-child", tool_modified_cb, NULL);
#endif
	
	gtk_timeout_add(1000, clock_tick, NULL);
}


int
main(int argc, char *argv[])
{
#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);
#endif

	tool_init("time", argc, argv);
	populate_ntp_list();
	init_map();
	connect_signals();

	tool_set_frozen(TRUE);
	transfer_xml_to_gui(&trans_tree, xml_doc_get_root(tool_config_get_xml()));
	tool_set_frozen(FALSE);

	gtk_widget_show_all (tool_get_top_window ());
	gtk_main();

	return 0;
}
