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

#ifndef __GST_IFACE_MODEM_
#define __GST_IFACE_MODEM_

#ifdef cplusplus
extern "C" {
#endif

#include "network-iface.h"
#include "gst-xml.h"

typedef enum {
  GST_MODEM_VOLUME_SILENT,
  GST_MODEM_VOLUME_LOW,
  GST_MODEM_VOLUME_MEDIUM,
  GST_MODEM_VOLUME_LOUD
} GstModemVolume;

typedef enum {
  GST_DIAL_TYPE_TONES,
  GST_DIAL_TYPE_PULSES
} GstDialType;

#define GST_MODEM_VOLUME               (gst_modem_volume_get_type ())
#define GST_DIAL_TYPE                  (gst_dial_type_get_type ())
#define GST_TYPE_IFACE_MODEM           (gst_iface_modem_get_type ())
#define GST_IFACE_MODEM(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_IFACE_MODEM, GstIfaceModem))
#define GST_IFACE_MODEM_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    GST_TYPE_IFACE_MODEM, GstIfaceModemClass))
#define GST_IS_IFACE_MODEM(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_IFACE_MODEM))
#define GST_IS_IFACE_MODEM_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    GST_TYPE_IFACE_MODEM))
#define GST_IFACE_MODEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_IFACE_MODEM, GstIfaceModemClass))

typedef struct _GstIfaceModem      GstIfaceModem;
typedef struct _GstIfaceModemClass GstIfaceModemClass;
typedef struct _GstIfaceModemPriv  GstIfaceModemPriv;

struct _GstIfaceModem
{
  GstIface parent;

  GstIfaceModemPriv *_priv;
};

struct _GstIfaceModemClass
{
  GstIfaceClass parent_class;
};

GType gst_iface_modem_get_type ();

GstIfaceModem* gst_iface_modem_new_from_xml (xmlNodePtr);
  
#ifdef cplusplus
}
#endif

#endif /* __GST_IFACE_MODEM_ */
