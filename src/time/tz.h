/*  GNOME canvas based interface to a map using a simple cylindrical proj */
/*                                                                        */
/* Copyright (C) 1999 Red Hat, Incorportated                              */
/* Original work by Michael Fulbright <drmike@redhat.com> */

#ifndef _E_TZ_H
#define _E_TZ_H


#define TZ_DATA_FILE "/usr/share/zoneinfo/zone.tab"


typedef struct _TzLocation TzLocation;
typedef struct _TzInfo TzInfo;
typedef struct _TzDB TzDB;


struct _TzLocation
{
	gchar *country;
	gdouble latitude;
	gdouble longitude;
	gchar *zone;
	gchar *comment;
};

/* see the glibc info page information on time zone information */
/*  tzname_normal    is the default name for the timezone */
/*  tzname_daylight  is the name of the zone when in daylight savings */
/*  utc_offset       is offset in seconds from utc */
/*  daylight         if non-zero then location obeys daylight savings */

struct _TzInfo
{
	gchar *tzname_normal;
	gchar *tzname_daylight;
	glong utc_offset;
	gint daylight;
};

struct _TzDB
{
	GPtrArray *locations;
};


TzDB *tz_load_db (void);
GPtrArray *tz_get_locations (TzDB *db);
char *tz_location_get_country (TzLocation *loc);
gchar *tz_location_get_zone (TzLocation *loc);
gchar *tz_location_get_comment (TzLocation *loc);
TzInfo *tz_info_from_location (TzLocation *loc);
void tz_info_free (TzInfo *tz_info);


#endif
