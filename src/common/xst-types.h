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

typedef enum {
	XST_DIALOG_BASIC,
	XST_DIALOG_INTERMEDIATE,
	XST_DIALOG_ADVANCED
} XstDialogComplexity;

typedef enum {
	XST_REPORT_HOOK_LOAD,
	XST_REPORT_HOOK_SAVE,
	XST_REPORT_HOOK_LOADSAVE
} XstReportHookType;

typedef struct _XstTool             XstTool;
typedef struct _XstToolClass        XstToolClass;

typedef struct _XstDialog           XstDialog;
typedef struct _XstDialogClass      XstDialogClass;

typedef struct _XstDialogSignal     XstDialogSignal;

typedef struct _XstReportLine       XstReportLine;

typedef struct _XstReportHook       XstReportHook;
typedef struct _XstReportHookEntry  XstReportHookEntry;

typedef struct _XstPlatform         XstPlatform;

#endif /* XST_TYPES_H */
