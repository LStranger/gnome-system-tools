/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* Copyright (C) 2004 Carlos Garnacho
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
 * Authors: Carlos Garnacho Parro  <carlosg@gnome.org>
 */

#ifndef __LOCATION_H_
#define __LOCATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>
#include "gst.h"

#define GST_TYPE_LOCATION           (gst_location_get_type ())
#define GST_LOCATION(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_LOCATION, GstLocation))
#define GST_LOCATION_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    GST_TYPE_LOCATION, GstLocationClass))
#define GST_IS_LOCATION(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_LOCATION))
#define GST_IS_LOCATION_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    GST_TYPE_LOCATION))
#define GST_LOCATION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_LOCATION, GstLocationClass))

typedef struct _GstLocation        GstLocation;
typedef struct _GstLocationClass   GstLocationClass;

struct _GstLocation
{
  GtkComboBox parent;
};

struct _GstLocationClass
{
  GtkComboBoxClass parent_class;
};

GstLocation* gst_location_combo_new   (void);
void         gst_location_combo_setup (GstLocation*, xmlNodePtr);

#ifdef __cpluspplus
}
#endif

#endif /* __LOCATION_H_ */
