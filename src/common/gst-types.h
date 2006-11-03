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
 * Authors: Jacob Berkman <jacob@ximian.com>
 */

#ifndef GST_TYPES_H
#define GST_TYPES_H

#include <glib.h>

typedef enum {
	GST_WIDGET_MODE_HIDDEN,
	GST_WIDGET_MODE_INSENSITIVE,
	GST_WIDGET_MODE_SENSITIVE
} GstWidgetMode;

typedef struct _GstDialog           GstDialog;
typedef struct _GstDialogClass      GstDialogClass;
typedef struct _GstDialogSignal     GstDialogSignal;

typedef struct _GstWidget           GstWidget;
typedef struct _GstWidgetPolicy     GstWidgetPolicy;
typedef struct _GstWidgetUserPolicy GstWidgetUserPolicy;

#endif /* GST_TYPES_H */
