/* -*- Mode: C; tab-width: 5; indent-tabs-mode: t; c-basic-offset: 2 -*- */
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
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
   
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <locale.h>
#include <gnome.h>

#include "xst-spin-button.h"

#define MIN_SPIN_BUTTON_WIDTH              30
#define ARROW_SIZE                         11
#define SPIN_BUTTON_INITIAL_TIMER_DELAY    200
#define SPIN_BUTTON_TIMER_DELAY            20
#define MAX_TEXT_LENGTH                    256
#define MAX_TIMER_CALLS                    5
#define EPSILON                            1e-5

enum {
  ARG_0,
  ARG_ADJUSTMENT,
  ARG_CLIMB_RATE,
  ARG_DIGITS,
  ARG_SNAP_TO_TICKS,
  ARG_NUMERIC,
  ARG_WRAP,
  ARG_UPDATE_POLICY,
  ARG_SHADOW_TYPE,
  ARG_VALUE
};


static void xst_spin_button_class_init     (XstSpinButtonClass *klass);
static void xst_spin_button_init           (XstSpinButton      *spin_button);
static void xst_spin_button_finalize       (GtkObject          *object);
static void xst_spin_button_set_arg        (GtkObject          *object,
					    GtkArg             *arg,
					    guint               arg_id);
static void xst_spin_button_get_arg        (GtkObject          *object,
					    GtkArg             *arg,
					    guint               arg_id);
static void xst_spin_button_map            (GtkWidget          *widget);
static void xst_spin_button_unmap          (GtkWidget          *widget);
static void xst_spin_button_realize        (GtkWidget          *widget);
static void xst_spin_button_unrealize      (GtkWidget          *widget);
static void xst_spin_button_size_request   (GtkWidget          *widget,
					    GtkRequisition     *requisition);
static void xst_spin_button_size_allocate  (GtkWidget          *widget,
					    GtkAllocation      *allocation);
static void xst_spin_button_paint          (GtkWidget          *widget,
					    GdkRectangle       *area);
static void xst_spin_button_draw           (GtkWidget          *widget,
					    GdkRectangle       *area);
static gint xst_spin_button_expose         (GtkWidget          *widget,
					    GdkEventExpose     *event);
static gint xst_spin_button_button_press   (GtkWidget          *widget,
					    GdkEventButton     *event);
static gint xst_spin_button_button_release (GtkWidget          *widget,
					    GdkEventButton     *event);
static gint xst_spin_button_motion_notify  (GtkWidget          *widget,
					    GdkEventMotion     *event);
static gint xst_spin_button_enter_notify   (GtkWidget          *widget,
					    GdkEventCrossing   *event);
static gint xst_spin_button_leave_notify   (GtkWidget          *widget,
					    GdkEventCrossing   *event);
static gint xst_spin_button_focus_out      (GtkWidget          *widget,
					    GdkEventFocus      *event);
static void xst_spin_button_draw_arrow     (XstSpinButton      *spin_button, 
					    guint               arrow);
static gint xst_spin_button_timer          (XstSpinButton      *spin_button);
static void xst_spin_button_value_changed  (GtkAdjustment      *adjustment,
					    XstSpinButton      *spin_button); 
static gint xst_spin_button_key_press      (GtkWidget          *widget,
					    GdkEventKey        *event);
static gint xst_spin_button_key_release    (GtkWidget          *widget,
					    GdkEventKey        *event);
static void xst_spin_button_activate       (GtkEditable        *editable);
static void xst_spin_button_snap           (XstSpinButton      *spin_button,
					    gfloat              val);
static void xst_spin_button_insert_text    (GtkEditable        *editable,
					    const gchar        *new_text,
					    gint                new_text_length,
					    gint               *position);
static void xst_spin_button_real_spin      (XstSpinButton      *spin_button,
					    gfloat              step);


static GtkEntryClass *parent_class = NULL;


GtkType
xst_spin_button_get_type (void)
{
  static guint spin_button_type = 0;

  if (!spin_button_type)
    {
      static const GtkTypeInfo spin_button_info =
      {
	"XstSpinButton",
	sizeof (XstSpinButton),
	sizeof (XstSpinButtonClass),
	(GtkClassInitFunc) xst_spin_button_class_init,
	(GtkObjectInitFunc) xst_spin_button_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      spin_button_type = gtk_type_unique (GTK_TYPE_ENTRY, &spin_button_info);
    }
  return spin_button_type;
}

static void
xst_spin_button_class_init (XstSpinButtonClass *class)
{
  GtkObjectClass   *object_class;
  GtkWidgetClass   *widget_class;
  GtkEditableClass *editable_class;

  object_class   = (GtkObjectClass*)   class;
  widget_class   = (GtkWidgetClass*)   class;
  editable_class = (GtkEditableClass*) class; 

  parent_class = gtk_type_class (GTK_TYPE_ENTRY);

  gtk_object_add_arg_type ("XstSpinButton::adjustment",
			   GTK_TYPE_ADJUSTMENT,
			   GTK_ARG_READWRITE,
			   ARG_ADJUSTMENT);
  gtk_object_add_arg_type ("XstSpinButton::climb_rate",
			   GTK_TYPE_FLOAT,
			   GTK_ARG_READWRITE,
			   ARG_CLIMB_RATE);
  gtk_object_add_arg_type ("XstSpinButton::digits",
			   GTK_TYPE_UINT,
			   GTK_ARG_READWRITE,
			   ARG_DIGITS);
  gtk_object_add_arg_type ("XstSpinButton::snap_to_ticks",
			   GTK_TYPE_BOOL,
			   GTK_ARG_READWRITE,
			   ARG_SNAP_TO_TICKS);
  gtk_object_add_arg_type ("XstSpinButton::numeric",
			   GTK_TYPE_BOOL,
			   GTK_ARG_READWRITE,
			   ARG_NUMERIC);
  gtk_object_add_arg_type ("XstSpinButton::wrap",
			   GTK_TYPE_BOOL,
			   GTK_ARG_READWRITE,
			   ARG_WRAP);
  gtk_object_add_arg_type ("XstSpinButton::update_policy",
			   GTK_TYPE_SPIN_BUTTON_UPDATE_POLICY,
			   GTK_ARG_READWRITE,
			   ARG_UPDATE_POLICY);
  gtk_object_add_arg_type ("XstSpinButton::shadow_type",
			   GTK_TYPE_SHADOW_TYPE,
			   GTK_ARG_READWRITE,
			   ARG_SHADOW_TYPE);
  gtk_object_add_arg_type ("XstSpinButton::value",
			   GTK_TYPE_FLOAT,
			   GTK_ARG_READWRITE,
			   ARG_VALUE);
  

  object_class->set_arg = xst_spin_button_set_arg;
  object_class->get_arg = xst_spin_button_get_arg;
  object_class->finalize = xst_spin_button_finalize;

  widget_class->map = xst_spin_button_map;
  widget_class->unmap = xst_spin_button_unmap;
  widget_class->realize = xst_spin_button_realize;
  widget_class->unrealize = xst_spin_button_unrealize;
  widget_class->size_request = xst_spin_button_size_request;
  widget_class->size_allocate = xst_spin_button_size_allocate;
  widget_class->draw = xst_spin_button_draw;
  widget_class->expose_event = xst_spin_button_expose;
  widget_class->button_press_event = xst_spin_button_button_press;
  widget_class->button_release_event = xst_spin_button_button_release;
  widget_class->motion_notify_event = xst_spin_button_motion_notify;
  widget_class->key_press_event = xst_spin_button_key_press;
  widget_class->key_release_event = xst_spin_button_key_release;
  widget_class->enter_notify_event = xst_spin_button_enter_notify;
  widget_class->leave_notify_event = xst_spin_button_leave_notify;
  widget_class->focus_out_event = xst_spin_button_focus_out;

  editable_class->insert_text = xst_spin_button_insert_text;
  editable_class->activate = xst_spin_button_activate;
}

static void
xst_spin_button_set_arg (GtkObject        *object,
			 GtkArg           *arg,
			 guint             arg_id)
{
  XstSpinButton *spin_button;

  spin_button = XST_SPIN_BUTTON (object);
  
  switch (arg_id)
    {
      GtkAdjustment *adjustment;

    case ARG_ADJUSTMENT:
      adjustment = GTK_VALUE_POINTER (*arg);
      if (!adjustment)
	adjustment = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
      xst_spin_button_set_adjustment (spin_button, adjustment);
      break;
    case ARG_CLIMB_RATE:
      xst_spin_button_configure (spin_button,
				 spin_button->adjustment,
				 GTK_VALUE_FLOAT (*arg),
				 spin_button->digits);
      break;
    case ARG_DIGITS:
      xst_spin_button_configure (spin_button,
				 spin_button->adjustment,
				 spin_button->climb_rate,
				 GTK_VALUE_UINT (*arg));
      break;
    case ARG_SNAP_TO_TICKS:
      xst_spin_button_set_snap_to_ticks (spin_button, GTK_VALUE_BOOL (*arg));
      break;
    case ARG_NUMERIC:
      xst_spin_button_set_numeric (spin_button, GTK_VALUE_BOOL (*arg));
      break;
    case ARG_WRAP:
      xst_spin_button_set_wrap (spin_button, GTK_VALUE_BOOL (*arg));
      break;
    case ARG_UPDATE_POLICY:
      xst_spin_button_set_update_policy (spin_button, GTK_VALUE_ENUM (*arg));
      break;
    case ARG_SHADOW_TYPE:
      xst_spin_button_set_shadow_type (spin_button, GTK_VALUE_ENUM (*arg));
      break;
    case ARG_VALUE:
      xst_spin_button_set_value (spin_button, GTK_VALUE_FLOAT (*arg));
      break;
    default:
      break;
    }
}

static void
xst_spin_button_get_arg (GtkObject        *object,
			 GtkArg           *arg,
			 guint             arg_id)
{
  XstSpinButton *spin_button;

  spin_button = XST_SPIN_BUTTON (object);
  
  switch (arg_id)
    {
    case ARG_ADJUSTMENT:
      GTK_VALUE_POINTER (*arg) = spin_button->adjustment;
      break;
    case ARG_CLIMB_RATE:
      GTK_VALUE_FLOAT (*arg) = spin_button->climb_rate;
      break;
    case ARG_DIGITS:
      GTK_VALUE_UINT (*arg) = spin_button->digits;
      break;
    case ARG_SNAP_TO_TICKS:
      GTK_VALUE_BOOL (*arg) = spin_button->snap_to_ticks;
      break;
    case ARG_NUMERIC:
      GTK_VALUE_BOOL (*arg) = spin_button->numeric;
      break;
    case ARG_WRAP:
      GTK_VALUE_BOOL (*arg) = spin_button->wrap;
      break;
    case ARG_UPDATE_POLICY:
      GTK_VALUE_ENUM (*arg) = spin_button->update_policy;
      break;
    case ARG_SHADOW_TYPE:
      GTK_VALUE_ENUM (*arg) = spin_button->shadow_type;
      break;
    case ARG_VALUE:
      GTK_VALUE_FLOAT (*arg) = spin_button->adjustment->value;
      break;
    default:
      arg->type = GTK_TYPE_INVALID;
      break;
    }
}

static void
xst_spin_button_init (XstSpinButton *spin_button)
{
  spin_button->adjustment = NULL;
  spin_button->panel = NULL;
  spin_button->shadow_type = GTK_SHADOW_NONE;
  spin_button->timer = 0;
  spin_button->ev_time = 0;
  spin_button->climb_rate = 0.0;
  spin_button->timer_step = 0.0;
  spin_button->update_policy = GTK_UPDATE_ALWAYS;
  spin_button->in_child = 2;
  spin_button->click_child = 2;
  spin_button->button = 0;
  spin_button->need_timer = FALSE;
  spin_button->timer_calls = 0;
  spin_button->digits = 0;
  spin_button->numeric = FALSE;
  spin_button->wrap = FALSE;
  spin_button->snap_to_ticks = FALSE;

  xst_spin_button_set_adjustment (spin_button,
				  (GtkAdjustment*) gtk_adjustment_new (0, 0, 0, 0, 0, 0));
}

static void
xst_spin_button_finalize (GtkObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (object));

  gtk_object_unref (GTK_OBJECT (XST_SPIN_BUTTON (object)->adjustment));
  
  GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
xst_spin_button_map (GtkWidget *widget)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (widget));

  if (GTK_WIDGET_REALIZED (widget) && !GTK_WIDGET_MAPPED (widget))
    {
      GTK_WIDGET_CLASS (parent_class)->map (widget);
      gdk_window_show (XST_SPIN_BUTTON (widget)->panel);
    }
}

static void
xst_spin_button_unmap (GtkWidget *widget)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (widget));

  if (GTK_WIDGET_MAPPED (widget))
    {
      gdk_window_hide (XST_SPIN_BUTTON (widget)->panel);
      GTK_WIDGET_CLASS (parent_class)->unmap (widget);
    }
}

static void
xst_spin_button_realize (GtkWidget *widget)
{
  XstSpinButton *spin;
  GdkWindowAttr attributes;
  gint attributes_mask;
  guint real_width;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (widget));
  
  spin = XST_SPIN_BUTTON (widget);

  real_width = widget->allocation.width;
  widget->allocation.width -= ARROW_SIZE + 2 * widget->style->klass->xthickness;
  gtk_widget_set_events (widget, gtk_widget_get_events (widget) |
			 GDK_KEY_RELEASE_MASK);
  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  widget->allocation.width = real_width;
  
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK 
    | GDK_BUTTON_RELEASE_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK 
    | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  attributes.x = (widget->allocation.x + widget->allocation.width - ARROW_SIZE -
		  2 * widget->style->klass->xthickness);
  attributes.y = widget->allocation.y + (widget->allocation.height -
					 widget->requisition.height) / 2;
  attributes.width = ARROW_SIZE + 2 * widget->style->klass->xthickness;
  attributes.height = widget->requisition.height;
  
  spin->panel = gdk_window_new (gtk_widget_get_parent_window (widget), 
				&attributes, attributes_mask);
  gdk_window_set_user_data (spin->panel, widget);

  gtk_style_set_background (widget->style, spin->panel, GTK_STATE_NORMAL);
}

static void
xst_spin_button_unrealize (GtkWidget *widget)
{
  XstSpinButton *spin;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (widget));

  spin = XST_SPIN_BUTTON (widget);

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);

  if (spin->panel)
    {
      gdk_window_set_user_data (spin->panel, NULL);
      gdk_window_destroy (spin->panel);
      spin->panel = NULL;
    }
}

static void
xst_spin_button_size_request (GtkWidget      *widget,
			      GtkRequisition *requisition)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (requisition != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (widget));

  GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);
  
  requisition->width = MIN_SPIN_BUTTON_WIDTH + ARROW_SIZE 
    + 2 * widget->style->klass->xthickness;
}

static void
xst_spin_button_size_allocate (GtkWidget     *widget,
			       GtkAllocation *allocation)
{
  GtkAllocation child_allocation;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (widget));
  g_return_if_fail (allocation != NULL);

  child_allocation = *allocation;
  if (child_allocation.width > ARROW_SIZE + 2 * widget->style->klass->xthickness)
    child_allocation.width -= ARROW_SIZE + 2 * widget->style->klass->xthickness;

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, &child_allocation);

  widget->allocation = *allocation;

  if (GTK_WIDGET_REALIZED (widget))
    {
      child_allocation.width = ARROW_SIZE + 2 * widget->style->klass->xthickness;
      child_allocation.height = widget->requisition.height;  
      child_allocation.x = (allocation->x + allocation->width - ARROW_SIZE - 
			    2 * widget->style->klass->xthickness);
      child_allocation.y = allocation->y + (allocation->height - widget->requisition.height) / 2;

      gdk_window_move_resize (XST_SPIN_BUTTON (widget)->panel, 
			      child_allocation.x,
			      child_allocation.y,
			      child_allocation.width,
			      child_allocation.height); 
    }
}

static GtkShadowType
xst_spin_button_get_shadow_type (XstSpinButton *spin_button)
{
  GtkWidget *widget = GTK_WIDGET (spin_button);
  
  GtkShadowType shadow_type =
    gtk_style_get_prop_experimental (widget->style,
				     "XstSpinButton::shadow_type", -1);

  if (shadow_type != (GtkShadowType)-1)
    return shadow_type;
  else
    return spin_button->shadow_type;
}

static void
xst_spin_button_paint (GtkWidget    *widget,
		       GdkRectangle *area)
{
  XstSpinButton *spin;
  GtkShadowType shadow_type;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (widget));

  spin = XST_SPIN_BUTTON (widget);
  shadow_type = xst_spin_button_get_shadow_type (spin);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      if (shadow_type != GTK_SHADOW_NONE)
	gtk_paint_box (widget->style, spin->panel,
		       GTK_STATE_NORMAL, shadow_type,
		       area, widget, "spinbutton",
		       0, 0, 
		       ARROW_SIZE + 2 * widget->style->klass->xthickness,
		       widget->requisition.height); 
      else
	{
	  gdk_window_set_back_pixmap (spin->panel, NULL, TRUE);
	  gdk_window_clear_area (spin->panel, area->x, area->y, area->width, area->height);
	}
      xst_spin_button_draw_arrow (spin, GTK_ARROW_UP);
      xst_spin_button_draw_arrow (spin, GTK_ARROW_DOWN);
      
      GTK_WIDGET_CLASS (parent_class)->draw (widget, area);
    }
}

static void
xst_spin_button_draw (GtkWidget    *widget,
		      GdkRectangle *area)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (widget));
  g_return_if_fail (area != NULL);

  if (GTK_WIDGET_DRAWABLE (widget))
    xst_spin_button_paint (widget, area);
}

static gint
xst_spin_button_expose (GtkWidget      *widget,
			GdkEventExpose *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (XST_IS_SPIN_BUTTON (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (GTK_WIDGET_DRAWABLE (widget))
    xst_spin_button_paint (widget, &event->area);

  return FALSE;
}

static void
xst_spin_button_draw_arrow (XstSpinButton *spin_button, 
			    guint          arrow)
{
  GtkStateType state_type;
  GtkShadowType shadow_type;
  GtkShadowType spin_shadow_type;
  GtkWidget *widget;
  gint x;
  gint y;

  g_return_if_fail (spin_button != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (spin_button));
  
  widget = GTK_WIDGET (spin_button);

  spin_shadow_type = xst_spin_button_get_shadow_type (spin_button);

  if (GTK_WIDGET_DRAWABLE (spin_button))
    {
      if (!spin_button->wrap &&
	  (((arrow == GTK_ARROW_UP &&
	  (spin_button->adjustment->upper - spin_button->adjustment->value
	   <= EPSILON))) ||
	  ((arrow == GTK_ARROW_DOWN &&
	  (spin_button->adjustment->value - spin_button->adjustment->lower
	   <= EPSILON)))))
	{
	  shadow_type = GTK_SHADOW_ETCHED_IN;
	  state_type = GTK_STATE_NORMAL;
	}
      else
	{
	  if (spin_button->in_child == arrow)
	    {
	      if (spin_button->click_child == arrow)
		state_type = GTK_STATE_ACTIVE;
	      else
		state_type = GTK_STATE_PRELIGHT;
	    }
	  else
	    state_type = GTK_STATE_NORMAL;
	  
	  if (spin_button->click_child == arrow)
	    shadow_type = GTK_SHADOW_IN;
	  else
	    shadow_type = GTK_SHADOW_OUT;
	}
      if (arrow == GTK_ARROW_UP)
	{
	  if (spin_shadow_type != GTK_SHADOW_NONE)
	    {
	      x = widget->style->klass->xthickness;
	      y = widget->style->klass->ythickness;
	    }
	  else
	    {
	      x = widget->style->klass->xthickness - 1;
	      y = widget->style->klass->ythickness - 1;
	    }
	  gtk_paint_arrow (widget->style, spin_button->panel,
			   state_type, shadow_type, 
			   NULL, widget, "spinbutton",
			   arrow, TRUE, 
			   x, y, ARROW_SIZE, widget->requisition.height / 2 
			   - widget->style->klass->ythickness);
	}
      else
	{
	  if (spin_shadow_type != GTK_SHADOW_NONE)
	    {
	      x = widget->style->klass->xthickness;
	      y = widget->requisition.height / 2;
	    }
	  else
	    {
	      x = widget->style->klass->xthickness - 1;
	      y = widget->requisition.height / 2 + 1;
	    }
	  gtk_paint_arrow (widget->style, spin_button->panel,
			   state_type, shadow_type, 
			   NULL, widget, "spinbutton",
			   arrow, TRUE, 
			   x, y, ARROW_SIZE, widget->requisition.height / 2 
			   - widget->style->klass->ythickness);
	}
    }
}

static gint
xst_spin_button_enter_notify (GtkWidget        *widget,
			      GdkEventCrossing *event)
{
  XstSpinButton *spin;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (XST_IS_SPIN_BUTTON (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  spin = XST_SPIN_BUTTON (widget);

  if (event->window == spin->panel)
    {
      gint x;
      gint y;

      gdk_window_get_pointer (spin->panel, &x, &y, NULL);

      if (y <= widget->requisition.height / 2)
	{
	  spin->in_child = GTK_ARROW_UP;
	  if (spin->click_child == 2) 
	    xst_spin_button_draw_arrow (spin, GTK_ARROW_UP);
	}
      else
	{
	  spin->in_child = GTK_ARROW_DOWN;
	  if (spin->click_child == 2) 
	    xst_spin_button_draw_arrow (spin, GTK_ARROW_DOWN);
	}
    }
  return FALSE;
}

static gint
xst_spin_button_leave_notify (GtkWidget        *widget,
			      GdkEventCrossing *event)
{
  XstSpinButton *spin;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (XST_IS_SPIN_BUTTON (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  spin = XST_SPIN_BUTTON (widget);

  if (event->window == spin->panel && spin->click_child == 2)
    {
      if (spin->in_child == GTK_ARROW_UP) 
	{
	  spin->in_child = 2;
	  xst_spin_button_draw_arrow (spin, GTK_ARROW_UP);
	}
      else
	{
	  spin->in_child = 2;
	  xst_spin_button_draw_arrow (spin, GTK_ARROW_DOWN);
	}
    }
  return FALSE;
}

static gint
xst_spin_button_focus_out (GtkWidget     *widget,
			   GdkEventFocus *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (XST_IS_SPIN_BUTTON (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

#if 0  
  if (GTK_EDITABLE (widget)->editable)
    xst_spin_button_update (XST_SPIN_BUTTON (widget));
#endif	

  return GTK_WIDGET_CLASS (parent_class)->focus_out_event (widget, event);
}

static gint
xst_spin_button_button_press (GtkWidget      *widget,
			      GdkEventButton *event)
{
  XstSpinButton *spin;
  gboolean up;
  gint step;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (XST_IS_SPIN_BUTTON (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  spin = XST_SPIN_BUTTON (widget);

  if (spin->button) {
    return FALSE;
  }
    
  if (event->window != spin->panel) {
    GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);
    return FALSE;
  }
  
  if (!GTK_WIDGET_HAS_FOCUS (widget))
    gtk_widget_grab_focus (widget);
  gtk_grab_add (widget);
  spin->button = event->button;

#if 0	
  if (GTK_EDITABLE (widget)->editable)
    xst_spin_button_update (spin);
#endif	

  up = (event->y <= widget->requisition.height / 2);
  spin->click_child = up ? GTK_ARROW_UP : GTK_ARROW_DOWN;

  if (event->button == 1)
    step = 1;
  else if (event->button == 2)
    step = 10;
  else
    return FALSE;

#if 0	
  xst_spin_button_real_spin (spin, 
					    spin->adjustment->step_increment);
#endif
  
  if (!spin->timer)
  {
    spin->timer_step = step;
    spin->need_timer = TRUE;
    spin->timer = gtk_timeout_add 
	 (SPIN_BUTTON_INITIAL_TIMER_DELAY, 
	  (GtkFunction) xst_spin_button_timer, (gpointer) spin);
  }
  
  xst_spin_button_draw_arrow (spin, spin->click_child);

  return FALSE;
}

static gint
xst_spin_button_button_release (GtkWidget      *widget,
				GdkEventButton *event)
{
  XstSpinButton *spin;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (XST_IS_SPIN_BUTTON (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  spin = XST_SPIN_BUTTON (widget);

  if (event->button == spin->button)
    {
      guint click_child;

      if (spin->timer)
	{
	  gtk_timeout_remove (spin->timer);
	  spin->timer = 0;
	  spin->timer_calls = 0;
	  spin->need_timer = FALSE;
	}

      if (event->button == 3)
	{
	  if (event->y >= 0 && event->x >= 0 && 
	      event->y <= widget->requisition.height &&
	      event->x <= ARROW_SIZE + 2 * widget->style->klass->xthickness)
	    {
	      if (spin->click_child == GTK_ARROW_UP &&
		  event->y <= widget->requisition.height / 2)
		{
		  gfloat diff;

		  diff = spin->adjustment->upper - spin->adjustment->value;
		  if (diff > EPSILON)
		    xst_spin_button_real_spin (spin, diff);
		}
	      else if (spin->click_child == GTK_ARROW_DOWN &&
		       event->y > widget->requisition.height / 2)
		{
		  gfloat diff;

		  diff = spin->adjustment->value - spin->adjustment->lower;
		  if (diff > EPSILON)
		    xst_spin_button_real_spin (spin, -diff);
		}
	    }
	}		  
      gtk_grab_remove (widget);
      click_child = spin->click_child;
      spin->click_child = 2;
      spin->button = 0;
      xst_spin_button_draw_arrow (spin, click_child);
    }
  else
    GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);

  return FALSE;
}

static gint
xst_spin_button_motion_notify (GtkWidget      *widget,
			       GdkEventMotion *event)
{
  XstSpinButton *spin;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (XST_IS_SPIN_BUTTON (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  spin = XST_SPIN_BUTTON (widget);
  
  if (spin->button)
    return FALSE;

  if (event->window == spin->panel)
    {
      gint y;

      y = event->y;
      if (event->is_hint)
	gdk_window_get_pointer (spin->panel, NULL, &y, NULL);

      if (y <= widget->requisition.height / 2 && 
	  spin->in_child == GTK_ARROW_DOWN)
	{
	  spin->in_child = GTK_ARROW_UP;
	  xst_spin_button_draw_arrow (spin, GTK_ARROW_UP);
	  xst_spin_button_draw_arrow (spin, GTK_ARROW_DOWN);
	}
      else if (y > widget->requisition.height / 2 && 
	  spin->in_child == GTK_ARROW_UP)
	{
	  spin->in_child = GTK_ARROW_DOWN;
	  xst_spin_button_draw_arrow (spin, GTK_ARROW_UP);
	  xst_spin_button_draw_arrow (spin, GTK_ARROW_DOWN);
	}
      return FALSE;
    }
	  
  return GTK_WIDGET_CLASS (parent_class)->motion_notify_event (widget, event);
}

static gint
xst_spin_button_timer (XstSpinButton *spin_button)
{
  gboolean retval = FALSE;
  
  GDK_THREADS_ENTER ();

#if 0	
  g_print ("Button timer ..\n");
#endif	
  
  if (spin_button->timer)
  {
    if (spin_button->click_child == GTK_ARROW_UP)
	 ;
#if 0	
	 g_print ("Up ..\n");
#endif	
    else
#if 0
	 g_print ("Down ..\n");
#endif	
    
    if (spin_button->need_timer)
    {
	 spin_button->need_timer = FALSE;
	 spin_button->timer = gtk_timeout_add 
	   (SPIN_BUTTON_TIMER_DELAY, (GtkFunction) xst_spin_button_timer, 
	    (gpointer) spin_button);
    }
    else 
    {
	 if (spin_button->climb_rate > 0.0 && spin_button->timer_step 
		< spin_button->adjustment->page_increment)
	 {
	   if (spin_button->timer_calls < MAX_TIMER_CALLS)
		spin_button->timer_calls++;
	   else 
	   {
		spin_button->timer_calls = 0;
		spin_button->timer_step += spin_button->climb_rate;
	   }
	 }
	 retval = TRUE;
    }
  }

  GDK_THREADS_LEAVE ();

  return retval;
}

static void
xst_spin_button_value_changed (GtkAdjustment *adjustment,
			       XstSpinButton *spin_button)
{
  char buf[MAX_TEXT_LENGTH];

  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  sprintf (buf, "%0.*f", spin_button->digits, adjustment->value);
  gtk_entry_set_text (GTK_ENTRY (spin_button), buf);

  xst_spin_button_draw_arrow (spin_button, GTK_ARROW_UP);
  xst_spin_button_draw_arrow (spin_button, GTK_ARROW_DOWN);
}

static gint
xst_spin_button_key_press (GtkWidget     *widget,
			   GdkEventKey   *event)
{
  XstSpinButton *spin;
  gint key;
  gboolean key_repeat = FALSE;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (XST_IS_SPIN_BUTTON (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  spin = XST_SPIN_BUTTON (widget);
  key = event->keyval;

  key_repeat = (event->time == spin->ev_time);

  if (GTK_EDITABLE (widget)->editable &&
      (key == GDK_Up || key == GDK_Down || 
       key == GDK_Page_Up || key == GDK_Page_Down))
    xst_spin_button_update (spin);

  switch (key)
    {
    case GDK_Up:

      if (GTK_WIDGET_HAS_FOCUS (widget))
	{
#if 0	
	  g_print ("Here here\n");
#endif	
	  return FALSE;
	  
	  gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), 
					"key_press_event");
	  if (!key_repeat)
	    spin->timer_step = spin->adjustment->step_increment;

	 xst_spin_button_real_spin (spin, spin->timer_step);

	  if (key_repeat)
	    {
	      if (spin->climb_rate > 0.0 && spin->timer_step
		  < spin->adjustment->page_increment)
		{
		  if (spin->timer_calls < MAX_TIMER_CALLS)
		    spin->timer_calls++;
		  else 
		    {
		      spin->timer_calls = 0;
		      spin->timer_step += spin->climb_rate;
		    }
		}
	    }
	  return TRUE;
	}
      return FALSE;

    case GDK_Down:

      if (GTK_WIDGET_HAS_FOCUS (widget))
	{
	  gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), 
					"key_press_event");
	  if (!key_repeat)
	    spin->timer_step = spin->adjustment->step_increment;

	 xst_spin_button_real_spin (spin, -spin->timer_step);

	  if (key_repeat)
	    {
	      if (spin->climb_rate > 0.0 && spin->timer_step
		  < spin->adjustment->page_increment)
		{
		  if (spin->timer_calls < MAX_TIMER_CALLS)
		    spin->timer_calls++;
		  else 
		    {
		      spin->timer_calls = 0;
		      spin->timer_step += spin->climb_rate;
		    }
		}
	    }
	  return TRUE;
	}
      return FALSE;

    case GDK_Page_Up:

      if (event->state & GDK_CONTROL_MASK)
	{
	  gfloat diff = spin->adjustment->upper - spin->adjustment->value;
	  if (diff > EPSILON)
	    xst_spin_button_real_spin (spin, diff);
	}
      else
	xst_spin_button_real_spin (spin, spin->adjustment->page_increment);
      return TRUE;

    case GDK_Page_Down:

      if (event->state & GDK_CONTROL_MASK)
	{
	  gfloat diff = spin->adjustment->value - spin->adjustment->lower;
	  if (diff > EPSILON)
	    xst_spin_button_real_spin (spin, -diff);
	}
      else
	xst_spin_button_real_spin (spin, -spin->adjustment->page_increment);
      return TRUE;

    default:
      break;
    }

#if 0	
  g_print ("Call paretn->key_press_event\n");
#endif	
  return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
}

static gint
xst_spin_button_key_release (GtkWidget   *widget,
			     GdkEventKey *event)
{
  XstSpinButton *spin;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (XST_IS_SPIN_BUTTON (widget), FALSE);
  
  spin = XST_SPIN_BUTTON (widget);
  
  spin->ev_time = event->time;
  return TRUE;
}

static void
xst_spin_button_snap (XstSpinButton *spin_button,
		      gfloat         val)
{
  gfloat inc;
  gfloat tmp;
  
  inc = spin_button->adjustment->step_increment;
  tmp = (val - spin_button->adjustment->lower) / inc;
  if (tmp - floor (tmp) < ceil (tmp) - tmp)
    val = spin_button->adjustment->lower + floor (tmp) * inc;
  else
    val = spin_button->adjustment->lower + ceil (tmp) * inc;

  if (fabs (val - spin_button->adjustment->value) > EPSILON)
    gtk_adjustment_set_value (spin_button->adjustment, val);
  else
    {
      char buf[MAX_TEXT_LENGTH];

      sprintf (buf, "%0.*f", spin_button->digits, 
	       spin_button->adjustment->value);
      if (strcmp (buf, gtk_entry_get_text (GTK_ENTRY (spin_button))))
	gtk_entry_set_text (GTK_ENTRY (spin_button), buf);
    }
}

void 
xst_spin_button_update (XstSpinButton *spin_button)
{
  gfloat val;
  gchar *error = NULL;

  g_return_if_fail (spin_button != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (spin_button));

  val = strtod (gtk_entry_get_text (GTK_ENTRY (spin_button)), &error);

  if (!spin_button->adjustment) {
#if 0	
    g_print ("dont update ..\n");
#endif
    return;
  }
  
  if (spin_button->update_policy == GTK_UPDATE_ALWAYS)
    {
      if (val < spin_button->adjustment->lower)
	val = spin_button->adjustment->lower;
      else if (val > spin_button->adjustment->upper)
	val = spin_button->adjustment->upper;
    }
  else if ((spin_button->update_policy == GTK_UPDATE_IF_VALID) && 
	   (*error ||
	   val < spin_button->adjustment->lower ||
	   val > spin_button->adjustment->upper))
    {
      xst_spin_button_value_changed (spin_button->adjustment, spin_button);
      return;
    }

  if (spin_button->snap_to_ticks)
    xst_spin_button_snap (spin_button, val);
  else
    {
      if (fabs (val - spin_button->adjustment->value) > EPSILON)
	gtk_adjustment_set_value (spin_button->adjustment, val);
      else
	{
	  char buf[MAX_TEXT_LENGTH];
	  
	  sprintf (buf, "%0.*f", spin_button->digits, 
		   spin_button->adjustment->value);
	  if (strcmp (buf, gtk_entry_get_text (GTK_ENTRY (spin_button))))
	    gtk_entry_set_text (GTK_ENTRY (spin_button), buf);
	}
    }
}

static void
xst_spin_button_activate (GtkEditable *editable)
{
  g_return_if_fail (editable != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (editable));

  if (editable->editable)
    xst_spin_button_update (XST_SPIN_BUTTON (editable));
}

static void
xst_spin_button_insert_text (GtkEditable *editable,
			     const gchar *new_text,
			     gint         new_text_length,
			     gint        *position)
{
  GtkEntry *entry;
  XstSpinButton *spin;
 
  g_return_if_fail (editable != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (editable));

  entry = GTK_ENTRY (editable);
  spin  = XST_SPIN_BUTTON (editable);

  if (spin->adjustment == NULL)
    return;
  
  if (spin->numeric)
    {
      struct lconv *lc;
      gboolean sign;
      gint dotpos = -1;
      gint i;
      GdkWChar pos_sign;
      GdkWChar neg_sign;
      gint entry_length;

      entry_length = entry->text_length;

      lc = localeconv ();

      if (*(lc->negative_sign))
	neg_sign = *(lc->negative_sign);
      else 
	neg_sign = '-';

      if (*(lc->positive_sign))
	pos_sign = *(lc->positive_sign);
      else 
	pos_sign = '+';

      for (sign=0, i=0; i<entry_length; i++)
	if ((entry->text[i] == neg_sign) ||
	    (entry->text[i] == pos_sign))
	  {
	    sign = 1;
	    break;
	  }

      if (sign && !(*position))
	return;

      for (dotpos=-1, i=0; i<entry_length; i++)
	if (entry->text[i] == *(lc->decimal_point))
	  {
	    dotpos = i;
	    break;
	  }

      if (dotpos > -1 && *position > dotpos &&
	  (gint)spin->digits - entry_length
	    + dotpos - new_text_length + 1 < 0)
	return;

      for (i = 0; i < new_text_length; i++)
	{
	  if (new_text[i] == neg_sign || new_text[i] == pos_sign)
	    {
	      if (sign || (*position) || i)
		return;
	      sign = TRUE;
	    }
	  else if (new_text[i] == *(lc->decimal_point))
	    {
	      if (!spin->digits || dotpos > -1 || 
 		  (new_text_length - 1 - i + entry_length
		    - *position > (gint)spin->digits)) 
		return;
	      dotpos = *position + i;
	    }
	  else if (new_text[i] < 0x30 || new_text[i] > 0x39)
	    return;
	}
    }

  GTK_EDITABLE_CLASS (parent_class)->insert_text (editable, new_text,
						  new_text_length, position);
}

static void
xst_spin_button_real_spin (XstSpinButton *spin_button,
			   gfloat         increment)
{
  GtkAdjustment *adj;
  gfloat new_value = 0.0;

  g_return_if_fail (spin_button != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (spin_button));

#if 0	
  g_print ("Real Spin !!!!!!!!! 1\n");
#endif	
  return;
  
  adj = spin_button->adjustment;

  new_value = adj->value + increment;

  if (increment > 0)
    {
      if (spin_button->wrap)
	{
	  if (fabs (adj->value - adj->upper) < EPSILON)
	    new_value = adj->lower;
	  else if (new_value > adj->upper)
	    new_value = adj->upper;
	}
      else
	new_value = MIN (new_value, adj->upper);
    }
  else if (increment < 0) 
    {
      if (spin_button->wrap)
	{
	  if (fabs (adj->value - adj->lower) < EPSILON)
	    new_value = adj->upper;
	  else if (new_value < adj->lower)
	    new_value = adj->lower;
	}
      else
	new_value = MAX (new_value, adj->lower);
    }

  if (fabs (new_value - adj->value) > EPSILON)
    gtk_adjustment_set_value (adj, new_value);
}


/***********************************************************
 ***********************************************************
 ***                  Public interface                   ***
 ***********************************************************
 ***********************************************************/


void
xst_spin_button_configure (XstSpinButton  *spin_button,
			   GtkAdjustment  *adjustment,
			   gfloat          climb_rate,
			   guint           digits)
{
  g_return_if_fail (spin_button != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (spin_button));
  g_return_if_fail (digits < 6);

  if (adjustment)
    xst_spin_button_set_adjustment (spin_button, adjustment);
  else
    adjustment = spin_button->adjustment;

  spin_button->digits = digits;
  spin_button->climb_rate = climb_rate;
  gtk_adjustment_value_changed (adjustment);
}

GtkWidget *
xst_spin_button_new (void)
{
  XstSpinButton *spin;

  spin = gtk_type_new (XST_TYPE_SPIN_BUTTON);

  return GTK_WIDGET (spin);
}

GtkWidget *
xst_spin_button_new_full (GtkAdjustment *adjustment,
			  gfloat         climb_rate,
			  guint          digits)
{
  XstSpinButton *spin;

  if (adjustment)
    g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), NULL);
  g_return_val_if_fail (digits < 6, NULL);

  spin = gtk_type_new (XST_TYPE_SPIN_BUTTON);

  xst_spin_button_configure (spin, adjustment, climb_rate, digits);

  return GTK_WIDGET (spin);
}

/* Callback used when the spin button's adjustment changes.  We need to redraw
 * the arrows when the adjustment's range changes.
 */
static void
adjustment_changed_cb (GtkAdjustment *adjustment, gpointer data)
{
  XstSpinButton *spin_button;

  spin_button = XST_SPIN_BUTTON (data);

  xst_spin_button_draw_arrow (spin_button, GTK_ARROW_UP);
  xst_spin_button_draw_arrow (spin_button, GTK_ARROW_DOWN);
}

void
xst_spin_button_set_adjustment (XstSpinButton *spin_button,
				GtkAdjustment *adjustment)
{
  g_return_if_fail (spin_button != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (spin_button));

  if (spin_button->adjustment != adjustment)
    {
      if (spin_button->adjustment)
        {
          gtk_signal_disconnect_by_data (GTK_OBJECT (spin_button->adjustment),
                                         (gpointer) spin_button);
          gtk_object_unref (GTK_OBJECT (spin_button->adjustment));
        }
      spin_button->adjustment = adjustment;
      if (adjustment)
        {
          gtk_object_ref (GTK_OBJECT (adjustment));
	  gtk_object_sink (GTK_OBJECT (adjustment));
          gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
			      (GtkSignalFunc) xst_spin_button_value_changed,
			      (gpointer) spin_button);
	  gtk_signal_connect (GTK_OBJECT (adjustment), "changed",
			      (GtkSignalFunc) adjustment_changed_cb,
			      (gpointer) spin_button);
        }
    }
}

GtkAdjustment *
xst_spin_button_get_adjustment (XstSpinButton *spin_button)
{
  g_return_val_if_fail (spin_button != NULL, NULL);
  g_return_val_if_fail (XST_IS_SPIN_BUTTON (spin_button), NULL);

  return spin_button->adjustment;
}

void
xst_spin_button_set_digits (XstSpinButton *spin_button,
			    guint          digits)
{
  g_return_if_fail (spin_button != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (spin_button));
  g_return_if_fail (digits < 6);

  if (spin_button->digits != digits)
    {
      spin_button->digits = digits;
      xst_spin_button_value_changed (spin_button->adjustment, spin_button);
    }
}

gfloat
xst_spin_button_get_value_as_float (XstSpinButton *spin_button)
{
  g_return_val_if_fail (spin_button != NULL, 0.0);
  g_return_val_if_fail (XST_IS_SPIN_BUTTON (spin_button), 0.0);

  return spin_button->adjustment->value;
}

gint
xst_spin_button_get_value_as_int (XstSpinButton *spin_button)
{
  gfloat val;

  g_return_val_if_fail (spin_button != NULL, 0);
  g_return_val_if_fail (XST_IS_SPIN_BUTTON (spin_button), 0);

  val = spin_button->adjustment->value;
  if (val - floor (val) < ceil (val) - val)
    return floor (val);
  else
    return ceil (val);
}

void 
xst_spin_button_set_value (XstSpinButton *spin_button, 
			   gfloat         value)
{
  g_return_if_fail (spin_button != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (spin_button));

  if (fabs (value - spin_button->adjustment->value) > EPSILON)
    gtk_adjustment_set_value (spin_button->adjustment, value);
  else
    {
      char buf[MAX_TEXT_LENGTH];

      sprintf (buf, "%0.*f", spin_button->digits, 
               spin_button->adjustment->value);
      if (strcmp (buf, gtk_entry_get_text (GTK_ENTRY (spin_button))))
        gtk_entry_set_text (GTK_ENTRY (spin_button), buf);
    }
}

void
xst_spin_button_set_update_policy (XstSpinButton             *spin_button,
				   XstSpinButtonUpdatePolicy  policy)
{
  g_return_if_fail (spin_button != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (spin_button));

  spin_button->update_policy = policy;
}

void
xst_spin_button_set_numeric (XstSpinButton  *spin_button,
			     gboolean        numeric)
{
  g_return_if_fail (spin_button != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (spin_button));

  spin_button->numeric = (numeric != 0);
}

void
xst_spin_button_set_wrap (XstSpinButton  *spin_button,
			  gboolean        wrap)
{
  g_return_if_fail (spin_button != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (spin_button));

  spin_button->wrap = (wrap != 0);
}

void
xst_spin_button_set_shadow_type (XstSpinButton *spin_button,
				 GtkShadowType  shadow_type)
{
  g_return_if_fail (spin_button != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (spin_button));

  if (shadow_type != spin_button->shadow_type)
    {
      spin_button->shadow_type = shadow_type;
      if (GTK_WIDGET_DRAWABLE (spin_button))
	gtk_widget_queue_draw (GTK_WIDGET (spin_button));
    }
}

void
xst_spin_button_set_snap_to_ticks (XstSpinButton *spin_button,
				   gboolean       snap_to_ticks)
{
  guint new_val;

  g_return_if_fail (spin_button != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (spin_button));

  new_val = (snap_to_ticks != 0);

  if (new_val != spin_button->snap_to_ticks)
    {
      spin_button->snap_to_ticks = new_val;
      if (new_val)
	{
	  gchar *error = NULL;
	  gfloat val;

	  val = strtod (gtk_entry_get_text (GTK_ENTRY (spin_button)), &error);
	  xst_spin_button_snap (spin_button, val);
	}
    }
}

void
xst_spin_button_spin (XstSpinButton *spin_button,
		      GtkSpinType    direction,
		      gfloat         increment)
{
  GtkAdjustment *adj;
  gfloat diff;

  g_return_if_fail (spin_button != NULL);
  g_return_if_fail (XST_IS_SPIN_BUTTON (spin_button));
  
  adj = spin_button->adjustment;

  /* for compatibility with the 1.0.x version of this function */
  if (increment != 0 && increment != adj->step_increment &&
      (direction == XST_SPIN_STEP_FORWARD ||
       direction == XST_SPIN_STEP_BACKWARD))
    {
      if (direction == XST_SPIN_STEP_BACKWARD && increment > 0)
	increment = -increment;
      direction = XST_SPIN_USER_DEFINED;
    }

  switch (direction)
    {
    case XST_SPIN_STEP_FORWARD:

      xst_spin_button_real_spin (spin_button, adj->step_increment);
      break;

    case XST_SPIN_STEP_BACKWARD:

      xst_spin_button_real_spin (spin_button, -adj->step_increment);
      break;

    case XST_SPIN_PAGE_FORWARD:

      xst_spin_button_real_spin (spin_button, adj->page_increment);
      break;

    case XST_SPIN_PAGE_BACKWARD:

      xst_spin_button_real_spin (spin_button, -adj->page_increment);
      break;

    case XST_SPIN_HOME:

      diff = adj->value - adj->lower;
      if (diff > EPSILON)
	xst_spin_button_real_spin (spin_button, -diff);
      break;

    case XST_SPIN_END:

      diff = adj->upper - adj->value;
      if (diff > EPSILON)
	xst_spin_button_real_spin (spin_button, diff);
      break;

    case XST_SPIN_USER_DEFINED:

      if (increment != 0)
	xst_spin_button_real_spin (spin_button, increment);
      break;

    default:
      break;
    }
}
