/* Timezone map.
 *
 * Copyright (C) 2000 Helix Code, Inc.
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


#ifndef _E_TZ_MAP_H
#define _E_TZ_MAP_H

#define TZ_MAP_POINT_NORMAL_RGBA 0xf010d0ff
#define TZ_MAP_POINT_HOVER_RGBA 0xffff60ff
#define TZ_MAP_POINT_SELECTED_1_RGBA 0xf040e0ff
#define TZ_MAP_POINT_SELECTED_2_RGBA 0x000000ff

typedef struct _ETzMap ETzMap;

struct _ETzMap
{
	EMap *map;
	TzDB *tzdb;
	
	EMapPoint *point_selected,
	          *point_hover;
};


/* --- Fake widget --- */

ETzMap *e_tz_map_new (void);

#endif
