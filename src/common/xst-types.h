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

#ifndef XST_TYPES_H
#define XST_TYPES_H

#include <glib.h>

#define XST_CONF_ROOT "/xst"

typedef enum {
	XST_DIALOG_BASIC,
	XST_DIALOG_ADVANCED,
	XST_DIALOG_NONE
} XstDialogComplexity;

typedef enum {
	XST_REPORT_HOOK_LOAD,
	XST_REPORT_HOOK_SAVE,
	XST_REPORT_HOOK_LOADSAVE
} XstReportHookType;

typedef enum {
	XST_WIDGET_MODE_HIDDEN,
	XST_WIDGET_MODE_INSENSITIVE,
	XST_WIDGET_MODE_SENSITIVE
} XstWidgetMode;

typedef enum {
	XST_MAJOR_SYS     = 0,
	XST_MAJOR_ERROR   = 1,
	XST_MAJOR_WARN    = 2,
	XST_MAJOR_INFO    = 3,
	XST_MAJOR_DEBUG   = 4,
	XST_MAJOR_MAX     = 5, /* To make report major array declarations look nice */
	XST_MAJOR_INVALID = 6  /* Only for validation purposes. Always last in enum */
} XstReportMajor;

typedef struct _XstTool             XstTool;
typedef struct _XstToolClass        XstToolClass;

typedef struct _XstDialog           XstDialog;
typedef struct _XstDialogClass      XstDialogClass;

typedef struct _XstDialogSignal     XstDialogSignal;

typedef struct _XstWidget           XstWidget;
typedef struct _XstWidgetPolicy     XstWidgetPolicy;
typedef struct _XstWidgetUserPolicy XstWidgetUserPolicy;

typedef struct _XstReportLine       XstReportLine;

typedef struct _XstReportHook       XstReportHook;
typedef struct _XstReportHookEntry  XstReportHookEntry;

typedef struct _XstPlatform         XstPlatform;

#endif /* XST_TYPES_H */
