/* Generic timezone utilities.
 *
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Authors: Hans Petter Jansson <hpj@helixcode.com>
 * 
 * Largely based on Michael Fulbright's work on Anaconda.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */


#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "tz.h"


/* Forward declarations for private functions */

static float convert_pos (gchar *pos, int digits);
static int compare_country_names (const void *a, const void *b);
static void sort_locations_by_country (GPtrArray *locations);


/* ---------------- *
 * Public interface *
 * ---------------- */


TzDB *
tz_load_db (void)
{
	TzDB *tz_db;
	FILE *tzfile;
	char buf[4096];
	
	tzfile = fopen (TZ_DATA_FILE, "r");
	if (!tzfile) return NULL;

	tz_db = g_new0 (TzDB, 1);
	tz_db->locations = g_ptr_array_new ();

	while (fgets (buf, sizeof(buf), tzfile))
	{
		gchar **tmpstrarr;
		gchar *latstr, *lngstr, *p;
		TzLocation *loc;

		if (*buf == '#') continue;

		g_strchomp(buf);
		tmpstrarr = g_strsplit(buf,"\t", 4);
		
		latstr = g_strdup (tmpstrarr[1]);
		p = latstr + 1;
		while (*p != '-' && *p != '+') p++;
		lngstr = g_strdup (p);
		*p = '\0';
		
		loc = g_new (TzLocation, 1);
		loc->country = g_strdup (tmpstrarr[0]);
		loc->zone = g_strdup (tmpstrarr[2]);
		loc->comment = (tmpstrarr[3]) ? g_strdup(tmpstrarr[3]) : NULL;
		loc->latitude  = convert_pos (latstr, 2);
		loc->longitude = convert_pos (lngstr, 3);

		g_ptr_array_add (tz_db->locations, (gpointer) loc);
		
		g_free (latstr);
		g_free (lngstr);
		g_strfreev (tmpstrarr);
	}
	
	fclose (tzfile);
	
	/* now sort by country */
	sort_locations_by_country (tz_db->locations);
	
	return tz_db;
}    


GPtrArray *
tz_get_locations (TzDB *db)
{
	return db->locations;
}


gchar *
tz_location_get_country (TzLocation *loc)
{
	return loc->country;
}


gchar *
tz_location_get_zone (TzLocation *loc)
{
	return loc->zone;
}


gchar *
tz_location_get_comment (TzLocation *loc)
{
	return loc->comment;
}


void
tz_location_get_position (TzLocation *loc, double *longitude, double *latitude)
{
	*longitude = loc->longitude;
	*latitude = loc->latitude;
}


TzInfo *
tz_info_from_location (TzLocation *loc)
{
	TzInfo *tzinfo;
	gchar *str;
	
	g_return_val_if_fail (loc != NULL, NULL);
	g_return_val_if_fail (loc->zone != NULL, NULL);
	
	str = g_strdup_printf ("TZ=%s", loc->zone);
	g_print ("%s %s\n", loc->zone, str);
	putenv (str);
	tzset ();
	g_free (str);
	tzinfo = g_new0 (TzInfo, 1);
	
	g_print ("%s %s %ld %d\n",tzname[0], tzname[1], timezone, daylight);
	
	/* Currently this solution doesnt seem to work - I get that */
	/* America/Phoenix uses daylight savings, which is wrong    */
	tzinfo->tzname_normal = (tzname[0]) ? g_strdup (tzname[0]) : NULL;
	tzinfo->tzname_daylight = (tzname[1]) ? g_strdup (tzname[1]) : NULL;
	tzinfo->utc_offset = timezone;
	tzinfo->daylight = daylight;
	
	return tzinfo;
}


void
tz_info_free (TzInfo *tzinfo)
{
	g_return_if_fail (tzinfo != NULL);
	
	if (tzinfo->tzname_normal) g_free (tzinfo->tzname_normal);
	if (tzinfo->tzname_daylight) g_free (tzinfo->tzname_daylight);
	g_free (tzinfo);
}


/* ----------------- *
 * Private functions *
 * ----------------- */


static float
convert_pos (gchar *pos, int digits)
{
	gchar whole[10];
	gchar *fraction;
	gint i;
	float t1, t2;
	
	if (!pos || strlen(pos) < 4 || digits > 9) return 0.0;
	
	for (i = 0; i < digits + 1; i++) whole[i] = pos[i];
	whole[i] = '\0';
	fraction = pos + digits + 1;

	t1 = g_strtod (whole, NULL);
	t2 = g_strtod (fraction, NULL);

	if (t1 >= 0.0) return t1 + t2/pow (10.0, strlen(fraction));
	else return t1 - t2/pow (10.0, strlen(fraction));
}


#if 0

/* Currently not working */
static void
free_tzdata (TzLocation *tz)
{
	
	if (tz->country)
	  g_free(tz->country);
	if (tz->zone)
	  g_free(tz->zone);
	if (tz->comment)
	  g_free(tz->comment);
	
	g_free(tz);
}
#endif


static int
compare_country_names (const void *a, const void *b)
{
	const TzLocation *tza = * (TzLocation **) a;
	const TzLocation *tzb = * (TzLocation **) b;
	
	return strcmp (tza->zone, tzb->zone);
}


static void
sort_locations_by_country (GPtrArray *locations)
{
	qsort (locations->pdata, locations->len, sizeof (gpointer),
	       compare_country_names);
}
