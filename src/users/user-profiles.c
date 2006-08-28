/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Copyright (C) 2006 Carlos Garnacho.
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
 * Authors: Carlos Garnacho Parro <carlosg@gnome.org>.
 */

#include <glib.h>
#include "user-profiles.h"

#define PROFILES_FILE "/etc/gnome-system-tools/users/profiles"
#define GST_USER_PROFILES_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_USER_PROFILES, GstUserProfilesPrivate))

typedef struct _GstUserProfilesPrivate GstUserProfilesPrivate;

struct _GstUserProfilesPrivate
{
	GstUserProfile *default_profile;
	GstUserProfile *current_profile;
	GHashTable     *profiles;
};

static void   gst_user_profiles_class_init (GstUserProfilesClass *class);
static void   gst_user_profiles_init       (GstUserProfiles *profiles);
static void   gst_user_profiles_finalize   (GObject *object);


G_DEFINE_TYPE (GstUserProfiles, gst_user_profiles, G_TYPE_OBJECT);

static void
gst_user_profiles_class_init (GstUserProfilesClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = gst_user_profiles_finalize;

	g_type_class_add_private (object_class,
				  sizeof (GstUserProfilesPrivate));
}

static GstUserProfile*
create_profile (GKeyFile    *key_file,
		const gchar *group)
{
	GstUserProfile *profile;

	profile = g_new0 (GstUserProfile, 1);
	profile->name = g_key_file_get_locale_string (key_file, group, "name", NULL, NULL);
	profile->is_default = g_key_file_get_boolean (key_file, group, "default", NULL);
	profile->shell = g_key_file_get_string (key_file, group, "shell", NULL);
	profile->home_prefix = g_key_file_get_string (key_file, group, "home-prefix", NULL);
	profile->groups = g_key_file_get_string_list (key_file, group, "groups", NULL, NULL);
	profile->uid_min = g_key_file_get_integer (key_file, group, "uid-min", NULL);
	profile->uid_max = g_key_file_get_integer (key_file, group, "uid-max", NULL);

	return profile;
}

static void
load_profiles (GstUserProfiles *profiles)
{
	GstUserProfilesPrivate *priv;
	GKeyFile *key_file;
	gchar **groups, **group;
	GstUserProfile *profile;

	priv = GST_USER_PROFILES_GET_PRIVATE (profiles);
	key_file = g_key_file_new ();
	g_key_file_set_list_separator (key_file, ',');

	if (!g_key_file_load_from_file (key_file, PROFILES_FILE, 0, NULL)) {
		g_key_file_free (key_file);
		return;
	}

	group = groups = g_key_file_get_groups (key_file, NULL);

	if (!groups)
		return;

	while (*group) {
		profile = create_profile (key_file, *group);
		g_hash_table_replace (priv->profiles, profile->name, profile);

		if (profile->is_default)
			priv->default_profile = priv->current_profile = profile;

		group++;
	}

	g_strfreev (groups);
	g_key_file_free (key_file);
}

static void
free_profile (GstUserProfile *profile)
{
	g_free (profile->name);
	g_free (profile->shell);
	g_free (profile->home_prefix);
	g_strfreev (profile->groups);
	g_free (profile);
}

static void
gst_user_profiles_init (GstUserProfiles *profiles)
{
	GstUserProfilesPrivate *priv;

	priv = GST_USER_PROFILES_GET_PRIVATE (profiles);

	priv->profiles = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, free_profile);
	load_profiles (profiles);
}

static void
gst_user_profiles_finalize (GObject *object)
{
	GstUserProfilesPrivate *priv;

	priv = GST_USER_PROFILES_GET_PRIVATE (object);

	if (priv->profiles)
		g_hash_table_unref (priv->profiles);
}

GstUserProfiles*
gst_user_profiles_get ()
{
	return g_object_new (GST_TYPE_USER_PROFILES, NULL);
}

void
retrieve_profile_names (gchar           *name,
			GstUserProfile  *profile,
			GList          **list)
{
	*list = g_list_prepend (*list, name);
}

GList*
gst_user_profiles_get_names (GstUserProfiles *profiles)
{
	GstUserProfilesPrivate *priv;
	GList *names = NULL;

	g_return_val_if_fail (GST_IS_USER_PROFILES (profiles), NULL);
	priv = GST_USER_PROFILES_GET_PRIVATE (profiles);

	g_hash_table_foreach (priv->profiles, (GHFunc) retrieve_profile_names, &names);
	return names;
}

GstUserProfile*
gst_user_profiles_set_current (GstUserProfiles *profiles,
			       const gchar     *profile)
{
	GstUserProfilesPrivate *priv;

	g_return_if_fail (GST_IS_USER_PROFILES (profiles));

	priv = GST_USER_PROFILES_GET_PRIVATE (profiles);

	priv->current_profile = g_hash_table_lookup (priv->profiles, profile);

	return priv->current_profile;
}

GstUserProfile*
gst_user_profiles_get_current (GstUserProfiles *profiles)
{
	GstUserProfilesPrivate *priv;

	g_return_if_fail (GST_IS_USER_PROFILES (profiles));

	priv = GST_USER_PROFILES_GET_PRIVATE (profiles);

	return priv->default_profile;
}

GstUserProfile*
gst_user_profiles_get_default_profile (GstUserProfiles *profiles)
{
	GstUserProfilesPrivate *priv;

	g_return_if_fail (GST_IS_USER_PROFILES (profiles));

	priv = GST_USER_PROFILES_GET_PRIVATE (profiles);

	return priv->default_profile;
}
