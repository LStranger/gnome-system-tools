/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * XstSpinButton widget for GTK+
 * Copyright (C) 1998 Lars Hamann and Stefan Jeske
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

/* Modified by the XST team. If the adjustment is set to NULL
 * the spin button will crash in several places
 */
#ifndef __XST_SPIN_BUTTON_H__
#define __XST_SPIN_BUTTON_H__

#include <gnome.h>

BEGIN_GNOME_DECLS

#define XST_TYPE_SPIN_BUTTON                  (xst_spin_button_get_type ())
#define XST_SPIN_BUTTON(obj)                  (GTK_CHECK_CAST ((obj), XST_TYPE_SPIN_BUTTON, XstSpinButton))
#define XST_SPIN_BUTTON_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), XST_TYPE_SPIN_BUTTON, XstSpinButtonClass))
#define XST_IS_SPIN_BUTTON(obj)               (GTK_CHECK_TYPE ((obj), XST_TYPE_SPIN_BUTTON))
#define XST_IS_SPIN_BUTTON_CLASS(klass)       (XST_CHECK_CLASS_TYPE ((klass), XST_TYPE_SPIN_BUTTON))


typedef enum
{
  XST_UPDATE_ALWAYS,
  XST_UPDATE_IF_VALID
} XstSpinButtonUpdatePolicy;

typedef enum
{
  XST_SPIN_STEP_FORWARD,
  XST_SPIN_STEP_BACKWARD,
  XST_SPIN_PAGE_FORWARD,
  XST_SPIN_PAGE_BACKWARD,
  XST_SPIN_HOME,
  XST_SPIN_END,
  XST_SPIN_USER_DEFINED
} XstSpinType;

typedef struct _XstSpinButton	    XstSpinButton;
typedef struct _XstSpinButtonClass  XstSpinButtonClass;

struct _XstSpinButton
{
  GtkEntry entry;
  
  GtkAdjustment *adjustment;
  
  GdkWindow *panel;
  GtkShadowType shadow_type;
  
  guint32 timer;
  guint32 ev_time;
  
  gfloat climb_rate;
  gfloat timer_step;
  
  XstSpinButtonUpdatePolicy update_policy;
  
  guint in_child : 2;
  guint click_child : 2;
  guint button : 2;
  guint need_timer : 1;
  guint timer_calls : 3;
  guint digits : 3;
  guint numeric : 1;
  guint wrap : 1;
  guint snap_to_ticks : 1;
};

struct _XstSpinButtonClass
{
  GtkEntryClass parent_class;
};

GtkType		xst_spin_button_get_type	   (void);
void		xst_spin_button_configure	   (XstSpinButton  *spin_button,
						    GtkAdjustment  *adjustment,
						    gfloat	    climb_rate,
						    guint	    digits);

GtkWidget*	xst_spin_button_new                (void);
GtkWidget*	xst_spin_button_new_full	   (GtkAdjustment  *adjustment,
						    gfloat	    climb_rate,
						    guint	    digits);

void		xst_spin_button_set_adjustment	   (XstSpinButton  *spin_button,
						    GtkAdjustment  *adjustment);

GtkAdjustment*	xst_spin_button_get_adjustment	   (XstSpinButton  *spin_button);

void		xst_spin_button_set_digits	   (XstSpinButton  *spin_button,
						    guint	    digits);

gfloat		xst_spin_button_get_value_as_float (XstSpinButton  *spin_button);

gint		xst_spin_button_get_value_as_int   (XstSpinButton  *spin_button);

void		xst_spin_button_set_value	   (XstSpinButton  *spin_button, 
						    gfloat	    value);

void		xst_spin_button_set_update_policy  (XstSpinButton  *spin_button,
						    XstSpinButtonUpdatePolicy  policy);

void		xst_spin_button_set_numeric	   (XstSpinButton  *spin_button,
						    gboolean	    numeric);

void		xst_spin_button_spin		   (XstSpinButton  *spin_button,
						    XstSpinType     direction,
						    gfloat	    increment);

void		xst_spin_button_set_wrap	   (XstSpinButton  *spin_button,
						    gboolean	    wrap);

void		xst_spin_button_set_shadow_type	   (XstSpinButton  *spin_button,
						    GtkShadowType   shadow_type);

void		xst_spin_button_set_snap_to_ticks  (XstSpinButton  *spin_button,
						    gboolean	    snap_to_ticks);
void            xst_spin_button_update             (XstSpinButton  *spin_button);



END_GNOME_DECLS

#endif /* __XST_SPIN_BUTTON_H__ */
