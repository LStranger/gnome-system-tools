/* Copyright (C) 2000 Helix Code, Inc.
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
 * Authors: Hans Petter Jansson <hpj@helixcode.com>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include "text.h"


gchar *tool_text_size_from_kb(guint size_kb)
{
  gchar *r;

  if (size_kb >= 1000000)
    r = g_strdup_printf("%.1fG", (double) size_kb / 1000000.0);
  else if (size_kb >= 1000)
    r = g_strdup_printf("%.1fM", (double) size_kb / 1000.0);
  else
    r = g_strdup_printf("%dK", size_kb);

  return(r);
}


gchar *tool_text_size_from_mb(guint size_mb)
{
  gchar *r;

  if (size_mb >= 1000)
    r = g_strdup_printf("%.1fG", (double) size_mb / 1000.0);
  else
    r = g_strdup_printf("%.1fM", (double) size_mb);

  return(r);
}


gchar *tool_text_description_from_device(const gchar *devin)
{
  char *r;
  int num;
  char *device;

  if (strlen(devin) < 3) return(NULL);
  device = strdup(devin);

  while (*(device + strlen(device) - 1) - '0' < 10 &&
               *(device + strlen(device) - 1) - '0' >= 0)
    *(device + strlen(device) - 1) = '\0';

  if (!strncasecmp(device + strlen(device) - 3, "hd", 2))
  {
      num = tolower(*(device + strlen(device) - 1)) - 'a';
      if (num <= 1)
        r = g_strdup_printf("IDE primary %d", num + 1);
      else
        r = g_strdup_printf("IDE secondary %d", num - 1);
    }
  else if (!strncasecmp(device + strlen(device) - 3, "sd", 2))
  {
      num = tolower(*(device + strlen(device) - 1)) - 'a' + 1;
      r = g_strdup_printf("SCSI %d", num);
    }
  else if (!strncasecmp(device + strlen(device) - 3, "xd", 2))
  {
      num = tolower(*(device + strlen(device) - 1)) - 'a' + 1;
      r = g_strdup_printf("XT %d", num);
    }
  else r = strdup(devin);

  free(device);
  return(r);
}
