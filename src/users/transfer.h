#include <glade/glade.h>
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>



typedef struct __logindefs _logindefs;

struct __logindefs
{
	gint passwd_max_day_use;
	guint new_group_min_id;
	guint passwd_min_length;
	guint new_group_max_id;
	guint new_user_min_id;
	guint passwd_warning_advance_days;
	guint new_user_max_id;
	gchar *mailbox_dir;
	gboolean create_home;
	gint passwd_min_day_use;
};

typedef struct _user user;

struct _user
{
	guint key;			/* ?? */
	gchar *login;			/* Login name */
	gchar *password;		/* Password */
	guint uid;			/* User id */
	guint gid;			/* Group id */
	gchar *comment;			/* Usually account owner's name */
	gchar *home;			/* Home directory */
	gchar *shell;			/* Account's shell */
	gint last_mod;			/* */
	gint passwd_max_life;		/* Password expiration time, 99999 if doesn't expire */
	gint passwd_exp_warn;		/* Number of days before users gets account exp. warning */
	gboolean passwd_exp_disable;	/* true if password doesn't expire */
	gboolean passwd_disable;	/* true if password is disabled */
	gint reserved;			/* */
	gboolean is_shadow;		/* true if using shadow password */
};

typedef struct _group group;

struct _group
{
	guint key;
	gchar *name;
	gchar *password;
	guint gid;
	GList *users;
};


extern user *current_user;
extern GList *user_list;

extern group *current_group;
extern GList *group_list;

extern _logindefs logindefs;

extern const gchar *user_list_data_key;
extern const gchar *group_list_data_key;

void transfer_config_saved(xmlNodePtr root);
void transfer_xml_to_gui(xmlNodePtr root);
void transfer_gui_to_xml(xmlNodePtr root);

