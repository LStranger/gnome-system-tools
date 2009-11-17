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
 * Authors: Carlos Garnacho Parro <carlosg@gnome.org>,
 *          Milan Bouchet-Valat <nalimilan@club.fr>.
 */

#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include "user-profiles.h"
#include "user-settings.h"
#include "group-settings.h"

#define PROFILES_FILE "/etc/gnome-system-tools/users/profiles"
#define GST_USER_PROFILES_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_USER_PROFILES, GstUserProfilesPrivate))

typedef struct _GstUserProfilesPrivate GstUserProfilesPrivate;

struct _GstUserProfilesPrivate
{
	GstUserProfile *default_profile;
	GstUserProfile *current_profile;
	GList          *profiles;
	GList          *all_groups;
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
	profile->description = g_key_file_get_locale_string (key_file, group,
	                                                     "description", NULL, NULL);
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
	gchar **groups, **group, **group_name;
	GstUserProfile *profile;
	GList *l, *item;

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
		priv->profiles = g_list_prepend (priv->profiles, profile);

		if (profile->is_default)
			priv->default_profile = profile;

		/* Add the groups to global list */
		group_name = profile->groups;
		while (group_name && *group_name) {
			if (!g_list_find_custom (priv->all_groups,
			                         *group_name,
			                         (GCompareFunc) strcmp))
				priv->all_groups = g_list_append (priv->all_groups,
				                                  *group_name);

			group_name++;
		}

		group++;
	}

	g_strfreev (groups);
	g_key_file_free (key_file);
}

static void
free_profile (GstUserProfile *profile)
{
	g_free (profile->name);
	g_free (profile->description);
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

	priv->profiles = NULL;
	load_profiles (profiles);
}

static void
gst_user_profiles_finalize (GObject *object)
{
	GstUserProfilesPrivate *priv;
	GList *l;

	priv = GST_USER_PROFILES_GET_PRIVATE (object);

	if (priv->profiles) {
		for (l = priv->profiles; l; l = l->next)
			free_profile ((GstUserProfile *) l->data);

		g_list_free (priv->profiles);
	}

	/* Free list structure, actual strings are owned by the profiles */
	g_list_free (priv->all_groups);
}

GstUserProfiles*
gst_user_profiles_get ()
{
	return g_object_new (GST_TYPE_USER_PROFILES, NULL);
}

/*
 * Get the profile correspoding to a given name,
 * or NULL if no such profile exists.
 */
GstUserProfile*
gst_user_profiles_get_from_name (GstUserProfiles *profiles,
                                 const gchar      *name)
{
	GstUserProfilesPrivate *priv;
	GstUserProfile *profile;
	GList *l;

	g_return_val_if_fail (GST_IS_USER_PROFILES (profiles), NULL);

	priv = GST_USER_PROFILES_GET_PRIVATE (profiles);

	for (l = priv->profiles; l; l = l->next) {
		profile = l->data;
		if (strcmp (name, profile->name) == 0)
			return profile;
	    }

	return NULL;
}

GList*
gst_user_profiles_get_list (GstUserProfiles *profiles)
{
	GstUserProfilesPrivate *priv;
	GstUserProfile *profile;
	GList *l;

	g_return_val_if_fail (GST_IS_USER_PROFILES (profiles), NULL);
	priv = GST_USER_PROFILES_GET_PRIVATE (profiles);

	return priv->profiles;
}

GstUserProfile*
gst_user_profiles_get_default_profile (GstUserProfiles *profiles)
{
	GstUserProfilesPrivate *priv;

	g_return_if_fail (GST_IS_USER_PROFILES (profiles));

	priv = GST_USER_PROFILES_GET_PRIVATE (profiles);

	return priv->default_profile;
}

/*
 * Find the profile that matches the settings of a given user.
 * Returns NULL if no profile exactly fits.
 *
 * Note that our groups handling checks that user is member of all
 * groups of the returned profile, as well as not member of any
 * group of any other profile. This ensures we are tolerant to
 * custom groups while not completely breaking the profiles concept.
 *
 * If @strict is FALSE, only groups and shell will be checked,
 * for consistency with gst_user_profiles_apply().
 */
GstUserProfile*
gst_user_profiles_get_for_user (GstUserProfiles *profiles,
                                OobsUser        *user,
                                gboolean         strict)
{
	GstUserProfilesPrivate *priv;
	GstUserProfile *profile;
	GstUserProfile *matched;
	GHashTableIter iter;
	gpointer value;
	GFile *file_home, *file_prefix;
	const gchar *shell, *home;
	gint uid;
	gchar **groups, **group_name;
	OobsGroup *group;
	GList *l, *m;
	gboolean matched_groups, in_profile, in_group;

	g_return_val_if_fail (GST_IS_USER_PROFILES (profiles), NULL);
	g_return_val_if_fail (OOBS_IS_USER (user), NULL);

	shell = oobs_user_get_shell (user);
	home = oobs_user_get_home_directory (user);
	uid = oobs_user_get_uid (user);

	priv = GST_USER_PROFILES_GET_PRIVATE (profiles);

	matched = NULL;
	for (l = priv->profiles; l; l = l->next) {
		profile = (GstUserProfile *) l->data;

		/* also check for settings that should not be changed for existing users */
		if (strict)
		  {
			  /* check that UID is in the range */
			  if (!(profile->uid_min <= uid && uid <= profile->uid_max))
				  continue;

			  /* check that home is under the prefix */
			  file_home = g_file_new_for_path (home);
			  file_prefix = g_file_new_for_path (profile->home_prefix);
			  if (!g_file_has_prefix (file_home, file_prefix)) {
				  g_object_unref (file_home);
				  g_object_unref (file_prefix);
				  continue;
			  }
			  else {
				  g_object_unref (file_home);
				  g_object_unref (file_prefix);
			  }
		  }

		/* check that shell is the right one */
		if (strcmp (shell, profile->shell) != 0)
			continue;

		/* check user's membership to all groups of the profile,
		/* also check that user is not member of any group
		 * that defines another profile vs the current one */
		matched_groups = TRUE;
		for (m = priv->all_groups; m && matched_groups; m = m->next) {
			in_profile = FALSE;
			group_name = profile->groups;
			while (group_name && *group_name) {
				if (strcmp ((char *) m->data, *group_name) == 0)
					in_profile = TRUE;

				group_name++;
			}

			group = group_settings_get_group_from_name ((char *) m->data);
			in_group = user_settings_is_user_in_group (user, group);
			if ((in_profile && !in_group) || (!in_profile && in_group)) {
				if (group)
					g_object_unref (group);

				matched_groups = FALSE;
				break;
			}
		}

		/* stop at first match, since the list has been reverted on loading,
		 * most privileged profiles must be at the end of the config file */
		if (matched_groups) {
			matched = profile;
			break;
		}
	}

	return matched;
}

/*
 * Change user settings to fit a given profile. User will be added to groups of
 * the passed profile, and removed from groups defining other profiles. If user_new
 * is TRUE, only shell and groups will be changed: forcing other settings would
 * break the account.
 */
void
gst_user_profiles_apply (GstUserProfiles *profiles,
                         GstUserProfile  *profile,
                         OobsUser        *user,
                         gboolean         new_user)
{
	GstUserProfilesPrivate *priv;
	gint uid;
	char *home;
	char **group_name;
	OobsGroup *group;
	GList *l;
	gboolean in_profile;

	g_return_if_fail (GST_IS_USER_PROFILES (profiles));
	g_return_if_fail (profile != NULL);
	g_return_if_fail (OOBS_IS_USER (user));
	/* used to build home dir, we normally ensure this before calling */
	g_return_if_fail (oobs_user_get_login_name (user) != NULL);

	priv = GST_USER_PROFILES_GET_PRIVATE (profiles);

	/* default shell */
	oobs_user_set_shell (user, profile->shell);

	/* add user to groups from the profile, remove it from groups of other profiles */
	for (l = priv->all_groups; l; l = l->next) {
		in_profile = FALSE;
		group_name = profile->groups;
		while (group_name && *group_name) {
			if (strcmp ((char *) l->data, *group_name) == 0)
				in_profile = TRUE;

			group_name++;
		}

		group = group_settings_get_group_from_name ((char *) l->data);
		if (in_profile)
			oobs_group_add_user (group, user);
		else
			oobs_group_remove_user (group, user);
	}

	if (!new_user) /* don't apply settings below to existing users */
		return;

	/* default UID */
	uid = user_settings_find_new_uid (profile->uid_min, profile->uid_max);

	/* default home prefix */
	home = g_build_path (G_DIR_SEPARATOR_S,
			     profile->home_prefix,
			     oobs_user_get_login_name (user),
			     NULL);
	oobs_user_set_home_directory (user, home);
	g_free (home);
}
