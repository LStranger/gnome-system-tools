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
 * Authors: Arturo Espinosa <arturo@ximian.com>
 */

#ifndef PPP_DRUID_H
#define PPP_DRUID_H

typedef struct {
	GladeXML *glade;

	GtkWidget *win;
	GnomeDruid *druid;

	GtkWidget *phone;
	GtkWidget *login;
	GtkWidget *passwd;
	GtkWidget *passwd2;
	GtkWidget *profile;

	gboolean error_state;
	gint current_page;

} PppDruid;
	
extern PppDruid *ppp_druid_new (void);
extern void ppp_druid_show (PppDruid *ppp);

#endif /* PPP_DRUID_H */
