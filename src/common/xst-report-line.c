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
 * Authors: Hans Petter Jansson <hpj@ximian.com>
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <gnome.h>

#include "xst-report-line.h"

static gboolean
xst_report_hash_eq (gconstpointer a, gconstpointer b)
{
	return !strcmp ((gchar *) a, (gchar *) b);
}

static guint
xst_report_hash_func (gconstpointer key)
{
	gchar *c;
	guint res;

	res = 0;

	for (c = (gchar *) key; *c; c++)
		res += (guint) *c;

	return res;
}

static GHashTable *
xst_report_hash_init (void)
{
	static gchar *table[] = {
		"begin",    N_("Start of work report."),
		"end",      N_("End of work report."),
		"progress", N_("%s"),
		"compat",   N_("%s."),

		"platform_list",            N_("Supported: [%s]"),
		"platform_unsup",           N_("Your platform is not supported."),
		"platform_undet",           N_("Unable to determine host platform."),
		"platform_success",         N_("Configuring for platform [%s]."),

		"xml_unexp_tag",            N_("Unexpected tag [%s]."),
		"xml_unexp_arg",            N_("Unexpected argument [%s] to tag [%s]."),

		"file_open_read_failed",    N_("Could not open [%s] for reading."),
		"file_open_read_success",   N_("Reading options from [%s]."),
		"file_open_write_failed",   N_("Failed to write to [%s]."),
		"file_open_write_create",   N_("Could not find [%s] for writing. Creating [%s]."),
		"file_open_write_success",  N_("Writing to [%s]."),
		"file_create_path",         N_("Directory [%s] created."),
		"file_backup_rotate",       N_("Backup directory [%s] was rotated."),
		"file_backup_success",      N_("Saved backup for [%s]."),
		"file_open_filter_failed",  N_("No file to patch: [%s]."),
		"file_open_filter_create",  N_("Could not find [%s] for patching. Creating [%s]."),
		"file_open_filter_success", N_("Found [%s]. Patching [%s]."),
		"file_buffer_load",         N_("Loading file [%s] to buffer."),
		"file_buffer_save",         N_("Saving buffer to file [%s]."),
		"file_run",                 N_("Running command [%s]."),
		"file_locate_tool_success", N_("Found tool [%s]."),
		"file_locate_tool_failed",  N_("Couldn't find tool [%s]."),
     
		"parse_trivial",            N_("Trivialy passing [%s]."),
		"parse_split",              N_("Getting option [%s] from [%s]."),
		"parse_split_hash",         N_("Getting configurations from [%s]."),
		"parse_sh",                 N_("Getting shell option [%s] from [%s]."),
		"parse_kw",                 N_("Getting keyword [%s] from [%s]."),
		"parse_line_first",         N_("Getting information from [%s]."),
		"parse_chat",               N_("Getting chat information from [%s]."),
		"parse_ini",                N_("Getting option [%s] from [%s, section [%s]."),
		"parse_ifaces_str",         N_("Getting option [%s] from interface [%s]."),
		"parse_ifaces_kw",          N_("Getting keyword [%s] from interface [%s]."),
		"parse_ifaces_kw_strange",  N_("Keyword for interface [%s] in [%s] had unexpected value."),

		"replace_split",            N_("Replacing key [%s] in [%s]."),
		"replace_sh",               N_("Replacing shell var [%s] in [%s]."),
		"replace_kw",               N_("Replacing keyword [%s] in [%s]."),
		"replace_line_first",       N_("Replacing contents of file [%s]."),
		"replace_chat",             N_("Replacing values in [%s]."),
		"replace_ini",              N_("Replacing variable [%s] in section [%s] of [%s]."),
		"replace_ifaces_str",       N_("Replacing option [%s] from interface [%s]."),
		"replace_ifaces_kw",        N_("Replacing keyword [%s] from interface [%s]."),
     
		"service_status_running",   N_("Service [%s] is running."),
		"service_status_stopped",   N_("Service [%s] is stopped."),
		"service_sysv_not_found",   N_("Could not find SystemV scripts for service [%s]."),
		"service_sysv_no_runlevel", N_("Could not find SystemV runlevel [%s] directory [%s]."),
		"service_sysv_remove_link", N_("Removed SystemV link [%s]."),
		"service_sysv_add_link",    N_("Created SystemV link [%s]."),
		"service_sysv_op_unk",      N_("Unknown initd operation [%s]."),
		"service_sysv_op_success",  N_("Service [%s] %s."),
		"service_sysv_op_failed",   N_("Service [%s] cound not be %s."),

		"network_dialing_get",      N_("Loading ISP configurations."),
		"network_iface_active_get", N_("Finding active interfaces."),
		"network_iface_is_active",  N_("Checking if interface [%s] is active."),
		"network_hostname_set",     N_("Setting hostname."),
		"network_dialing_set",      N_("Saving ISP configurations."),
		"network_remove_pap",       N_("Removing entry [%s] from [%s]."),
		"network_iface_set",        N_("Configuring interface [%s]."),
		"network_iface_activate",   N_("Activating interface [%s]."),
		"network_iface_deactivate", N_("Deactivating interface [%s]."),
		"network_ifaces_set",       N_("Setting up interfaces."),
		"network_get_pap_passwd",   N_("Getting PAP/CHAP password for [%s] from [%s]."),
		"network_get_ppp_option",   N_("Getting option [%s] from [%s]."),
		"network_set_ppp_option",   N_("Setting option [%s] in [%s]."),
		"network_set_ppp_connect",  N_("Setting connect option in [%s]."),
		"network_get_ppp_unsup",    N_("Getting additional options from [%s]."),
		"network_set_ppp_unsup",    N_("Setting additional options in [%s]."),
		"network_bootproto_unsup",  N_("Boot method [%s] for interface [%s] not supported."),
		"network_get_remote",       N_("Getting remote address for interface [%s]."),
		"network_set_remote",       N_("Setting remote address for interface [%s]."),
		"network_ensure_lo",        N_("Ensuring loopback interface configuration."),
     
		"boot_lilo_failed",         N_("Failed to run lilo."),
		"boot_lilo_success",        N_("Succesfully executed lilo."),

		"disks_fstab_add",          N_("Adding [%s] to fstab."),
		"disks_partition_probe",    N_("Looking for partitions on [%s]."),
		"disks_size_query",         N_("Querying size of [%s]."),
		"disks_mount",              N_("Mounting [%s]."),
		"disks_umount",             N_("Unmounting [%s]."),
		"disks_mount_error",        N_("Could not find mount tools. No mounting done."),
     
		"memory_swap_found",        N_("Found swap entry [%s]."),
		"memory_swap_probe",        N_("Looking for swap entries."),
     
		"print_no_printtool",       N_("No printtool setup in directory [%s]."),

		"time_timezone_scan",       N_("Scanning timezones."),
		"time_timezone_cmp",        N_("Scanning timezones: [%s]."),
		"time_timezone_set",        N_("Setting timezone as [%s]."),
		"time_localtime_set",       N_("Setting local time as [%s]."),

		"users_getting_db",         N_("Getting user database."),
		NULL, NULL
	};

	GHashTable *hash;
	gint i;

	/* Hell: GHashTable should have some decent defaults for common cases. */
	hash = g_hash_table_new (xst_report_hash_func, xst_report_hash_eq);

	for (i = 0; table[i]; i+= 2)
		g_hash_table_insert (hash, table[i], table[i + 1]);

	return hash;
}

static gchar *
xst_report_format_from_key (gchar *key)
{
	static GHashTable *hash = NULL;
	gchar *ret;

	if (!hash)
		hash = xst_report_hash_init ();
	
	ret = g_hash_table_lookup (hash, key);

	if (!ret)
		g_warning ("Key not found: %s", key);
			
	return ret;
}

/* printf, perl style, with a 1024 char limit. Gets the job done. */
static gchar *
xst_report_sprintf (gchar *fmt, gchar **argv)
{
	char *orig_fmt;
	char str[1024], ret[1024];
	char *c, *fmt_p, *fmt_p2;
	int i;

	ret[0] = 0;
	orig_fmt = g_strdup (fmt);
	fmt_p = orig_fmt;
	c = strchr (orig_fmt, '%');
	
	for (i = 0; argv[i]; i++) {
		if (c)
			*c = '%';
		c = strchr (c + 1, '%');
		if (c)
			*c = 0;
		fmt_p2 = c;
		
		snprintf (str, 1023, fmt_p, argv[i]);
		fmt_p = fmt_p2;
		strncpy (ret + strlen (ret), str, 1023 - strlen (ret));
	}

	if (!argv[0])
		return orig_fmt;
	
	g_free (orig_fmt);
	return g_strdup (ret);
}

XstReportLine *
xst_report_line_new (gchar *key, gchar **argv)
{
	XstReportLine *xrl;
	gchar *fmt;

	fmt = xst_report_format_from_key (key);

	g_return_val_if_fail (fmt != NULL, NULL);

	xrl = g_new0 (XstReportLine, 1);
	xrl->key = g_strdup (key);
	xrl->message = xst_report_sprintf (fmt, argv);
	xrl->handled = FALSE;

	return xrl;
}

XstReportLine *
xst_report_line_new_from_string (gchar *string)
{
	XstReportLine *xrl;
	gchar **parts;

	g_return_val_if_fail (strlen (string) > 1, NULL);

	parts = g_strsplit (string, "::", 0);
	xrl = xst_report_line_new (parts[0], &parts[1]);
	g_strfreev (parts);

	return xrl;
}

void
xst_report_line_free (XstReportLine *line)
{
	g_free (line->message);
	g_free (line);
}

const gchar *
xst_report_line_get_key (XstReportLine *line)
{
	g_return_val_if_fail (line != NULL, 0);
	return (line->key);
}

const gchar *
xst_report_line_get_message (XstReportLine *line)
{
	g_return_val_if_fail (line != NULL, NULL);
	return (line->message);
}

gboolean
xst_report_line_get_handled (XstReportLine *line)
{
	g_return_val_if_fail (line != NULL, FALSE);
	return (line->handled);
}

void
xst_report_line_set_handled (XstReportLine *line, gboolean handled)
{
	g_return_if_fail (line != NULL);

	line->handled = handled;
}
