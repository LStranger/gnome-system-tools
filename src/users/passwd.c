/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* passwd.c: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
 * 
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
 * Authors: Tambet Ingo <tambet@ximian.com> and Arturo Espinosa <arturo@ximian.com>.
 */

/* All this for password generation and crypting. */

#include "config.h"
#include "passwd.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef HAVE_LIBCRACK
#include <crack.h>
#endif

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

#include "gst.h"
#include "md5.h"
#include "table.h"

#define RANDOM_PASSWD_SIZE 7

extern GstTool *tool;

static gboolean
uses_md5 (void)
{
	xmlNodePtr root;

	root = gst_xml_doc_get_root (tool->config);
	return gst_xml_element_get_boolean (root, "use_md5");
}

/* str must be a string of len + 1 allocated gchars */
static gchar *
rand_str (gchar *str, gint len)
{
 	gchar alphanum[] = "0ab1cd2ef3gh4ij5kl6mn7op8qr9st0uvwxyz0AB1CD2EF3GH4IJ5KL6MN7OP8QR9ST0UVWXYZ";
	gint i, alnum_len;
	
	alnum_len = strlen (alphanum);
	str[len] = 0;
	
	for (i = 0; i < len; i++) 
		str[i] = alphanum [(gint) ((((float) alnum_len) * rand () / (RAND_MAX + 1.0)))];
	
	return str;
}


static gboolean
passwd_check_cracklib_dict_path (const gchar *directory)
{
 	gchar *file1 = g_strdup_printf ("%s/cracklib_dict.pwi", directory);
 	gchar *file2 = g_strdup_printf ("%s/cracklib_dict.hwm", directory);
 	gchar *file3 = g_strdup_printf ("%s/cracklib_dict.pwd", directory);
	gboolean ret = TRUE;

 	if (!g_file_test (file1, G_FILE_TEST_EXISTS))
		ret = FALSE;
	if (!g_file_test (file2, G_FILE_TEST_EXISTS)) 
		ret = FALSE;
	if (!g_file_test (file3, G_FILE_TEST_EXISTS))
		ret = FALSE; 

	g_free (file1);
	g_free (file2);
	g_free (file3);

	return ret;
}

static gchar *
passwd_get_cracklib_dictionary_path (void)
{
	const gchar *known_paths [] = { "/usr/lib",
					"/var/cache/cracklib"};
	gint num, i;
	static gboolean warned = FALSE;
	
#ifndef	HAVE_LIBCRACK
	return NULL;
#endif
#ifdef GST_CRACK_LIB_DICT_PATH
	if (passwd_check_cracklib_dict_path (GST_CRACK_LIB_DICT_PATH))
		return g_strdup_printf ("%s/cracklib_dict", GST_CRACK_LIB_DICT_PATH);

	if (!warned) {
		warned = TRUE;
		g_warning ("The cracklib dictionary was not found in the location specified [%s].\nsearching for a dictionary", GST_CRACK_LIB_DICT_PATH);
	}
#endif	

	num = sizeof (known_paths) / sizeof (gchar *);
	for (i = 0; i < num; i++)
		if (passwd_check_cracklib_dict_path (known_paths[i]))
			return g_strdup_printf ("%s/cracklib_dict", known_paths[i]);

	if (!warned) {
		warned = TRUE;
		g_warning ("Could not find a cracklib dictionary, will not check for password quality");
	}
	
	return NULL;
}

gchar *
passwd_get_random (void)
{
	gchar *random_passwd;
	gchar *dictionary = passwd_get_cracklib_dictionary_path ();

	random_passwd = g_new0 (gchar, RANDOM_PASSWD_SIZE + 1);
	rand_str (random_passwd, RANDOM_PASSWD_SIZE);

	if (dictionary)
#ifdef	HAVE_LIBCRACK
		while (FascistCheck (random_passwd, dictionary))
#endif	
			rand_str (random_passwd, RANDOM_PASSWD_SIZE);

	g_free (dictionary);

	return random_passwd;
}

gchar *
passwd_check (gchar *pwd1, gchar *pwd2, gboolean check_quality)
{
	gchar *dictionary = passwd_get_cracklib_dictionary_path ();
#ifdef	HAVE_LIBCRACK
	gchar *check_err;
#endif	

	g_return_val_if_fail (pwd1 != NULL, FALSE);
	g_return_val_if_fail (pwd2 != NULL, FALSE);

	if (strcmp (pwd1, pwd2))
		return g_strdup (_("Passwords do not match"));
	
	if (dictionary)
#ifdef	HAVE_LIBCRACK
		if (check_quality && (check_err = (gchar *) FascistCheck (pwd1, dictionary)))
			return g_strdup_printf (_("Bad password: %s.\nPlease try with a new password"), check_err);
#endif	

	g_free (dictionary);
	
	return NULL;
}	

void
passwd_set (xmlNodePtr node, const gchar *pwd)
{
	gchar salt[9];
	gchar *password, *buf;

	g_return_if_fail (node != NULL);

	if (!pwd)
		password = passwd_get_random (); /* TODO: memory leak, free it! */
	else
		password = g_strdup (pwd);

	if (uses_md5 ()) 
		buf = g_strdup (crypt_md5 (password, rand_str (salt, 8)));
	else
		buf = g_strdup (crypt (password, rand_str (salt, 2)));

	gst_xml_set_child_content (node, "password", buf);

	g_free (buf);
	g_free (password);
}
