/* passwd.c: this file is part of users-admin, a helix-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2000 Helix Code, Inc.
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
 * Authors: Tambet Ingo <tambeti@sa.ee> and Arturo Espinosa <arturo@helixcode.com>.
 */

#include "passwd.h"

/* All this for password generation and crypting. */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <crack.h>
#include <crypt.h>
#include "md5.h"

#include "e-table.h"

#define CRACK_DICT_PATH "/usr/lib/cracklib_dict"
#define RANDOM_PASSWD_SIZE 6

static gchar *pam_passwd_files[] = { "/etc/pam.d/passwd", NULL };

static gboolean passwd_uses_md5 (void);
static gchar *passwd_rand_str (gchar *str, gint len);

extern gchar *
passwd_get_random (void)
{
	gchar *random_passwd;
	
	random_passwd = g_new0 (gchar, RANDOM_PASSWD_SIZE + 1);
	while (FascistCheck (random_passwd, CRACK_DICT_PATH))
		passwd_rand_str (random_passwd, RANDOM_PASSWD_SIZE);
	
	return random_passwd;
}

extern gchar *
passwd_set (gchar *new_passwd, gchar *confirm, gboolean check_quality)
{
	gchar salt[9];
	gchar *check_err;
	gchar *password;
	
	if (strcmp (new_passwd, confirm))
		return (gchar *) -1;

	if (check_quality &&
			(check_err = FascistCheck (new_passwd, CRACK_DICT_PATH)))
		return check_err;

	if (passwd_uses_md5 ()) 
		password = g_strdup (crypt_md5 (new_passwd, passwd_rand_str (salt, 8)));
	else
		password = g_strdup (crypt (new_passwd, passwd_rand_str (salt, 2)));

	e_table_change_user ("password", password);
	return (gchar *) 0;
}

static gboolean
passwd_uses_md5 (void)
{
	gint i;
	gint fd = -1;
	gint last_line = 1;
	static gboolean been_here = FALSE;
	static gboolean found = FALSE;
	GScanner *scanner;
	GScannerConfig scanner_config =
	{
		(
		 " \t\r\n"
		 )			/* cset_skip_characters */,
		(
		 G_CSET_a_2_z
		 "_/.="
		 G_CSET_A_2_Z
		 )			/* cset_identifier_first */,
		(
		 G_CSET_a_2_z
		 "_/.="
		 G_CSET_A_2_Z
		 "1234567890"
		 G_CSET_LATINS
		 G_CSET_LATINC
		 )			/* cset_identifier_nth */,
		( "#\n" )		/* cpair_comment_single */,
		
		FALSE			/* case_sensitive */,
		
		TRUE			/* skip_comment_multi */,
		TRUE			/* skip_comment_single */,
		TRUE			/* scan_comment_multi */,
		TRUE			/* scan_identifier */,
		FALSE			/* scan_identifier_1char */,
		FALSE			/* scan_identifier_NULL */,
		FALSE			/* scan_symbols */,
		FALSE			/* scan_binary */,
		FALSE			/* scan_octal */,
		FALSE			/* scan_float */,
		FALSE			/* scan_hex */,
		FALSE			/* scan_hex_dollar */,
		FALSE			/* scan_string_sq */,
		FALSE			/* scan_string_dq */,
		FALSE			/* numbers_2_int */,
		FALSE			/* int_2_float */,
		FALSE			/* identifier_2_string */,
		FALSE			/* char_2_token */,
		FALSE			/* symbol_2_token */,
		FALSE			/* scope_0_fallback */,
	};
	
	if (been_here)
		return found;
	
	for (i = 0; pam_passwd_files[i]; i++)
		if ((fd = open (pam_passwd_files[i], O_RDONLY)) != -1)
			break;
	
	if (fd == -1)
		return FALSE;
	
	found = FALSE;
	scanner = g_scanner_new (&scanner_config);
	g_scanner_input_file (scanner, fd);
	
	/* Scan the file, until the md5 argument for /lib/security/pam_pwdb.so
	 * in the module-type password is found, or EOF */
	while ((g_scanner_get_next_token (scanner) != G_TOKEN_EOF) && !found)
	{
		
		/* Has a password module directive been found? */
		if ((scanner->token == G_TOKEN_IDENTIFIER) &&
				(scanner->position == 8) &&
				(!strcmp (scanner->value.v_identifier, "password")))
		{
			last_line = scanner->line;
			g_scanner_get_next_token (scanner);
			g_scanner_get_next_token (scanner);
			
			/* Check that the following arguments are for /lib/security/pam_pwdb.so. */
			if ((scanner->token == G_TOKEN_IDENTIFIER) &&
					(!strcmp (scanner->value.v_identifier, "/lib/security/pam_pwdb.so")))
				
				/* Cool: search all identifiers on the same line */
				while ((g_scanner_peek_next_token (scanner) != G_TOKEN_EOF) && 
							 (scanner->next_line == last_line) &&
							 !found)
			    {
						g_scanner_get_next_token (scanner);
				
						/* Is this the md5 argument? */
						if ((scanner->token == G_TOKEN_IDENTIFIER) &&
								(!strcmp (scanner->value.v_identifier, "md5")))
						{
							found = TRUE;
							break;
						}
					}
		}
	}
	
	g_scanner_destroy (scanner);
	close (fd);
	
	return found;
}

/* str must be a string of len + 1 allocated gchars */
static gchar *
passwd_rand_str (gchar *str, gint len)
{
 	gchar alphanum[] = "0ab1cd2ef3gh4ij5kl6mn7op8qr9st0uvwxyz0AB1CD2EF3GH4IJ5KL6MN7OP8QR9ST0UVWXYZ";
	gint i, alnum_len;
	
	alnum_len = strlen (alphanum);
	str[len] = 0;
	
	for (i = 0; i < len; i++) 
		str[i] = alphanum [(gint) ((((float) alnum_len) * rand () / (RAND_MAX + 1.0)))];
	
	return str;
}
