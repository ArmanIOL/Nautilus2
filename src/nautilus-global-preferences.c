/*
 * SPDX-FileCopyrightText: 1999, 2000, 2001 Eazel, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Authors: Ramiro Estrugo <ramiro@eazel.com>
 */

#include <config.h>
#include "nautilus-global-preferences.h"

#include <gtk/gtk.h>

#include "nautilus-file-utilities.h"

GSettings *nautilus_preferences;
GSettings *nautilus_compression_preferences;
GSettings *nautilus_icon_view_preferences;
GSettings *nautilus_list_view_preferences;
GSettings *nautilus_window_state;
GSettings *gtk_filechooser_preferences;
GSettings *gnome_lockdown_preferences;
GSettings *gnome_interface_preferences;
GSettings *gnome_privacy_preferences;
GSettings *gnome_user_share_preferences = NULL;
GSettings *localsearch_preferences;

#define FILE_SHARING_SCHEMA_ID "org.gnome.desktop.file-sharing"
#define FILE_SHARING_SERVICE_PATH "/org/gnome/settings-daemon/plugins/sharing/gnome-user-share-webdav/"
#define FILE_SHARING_SERVICE_SCHEMA_ID "org.gnome.settings-daemon.plugins.sharing.service"
#define UPPER_MOUSE_LIMIT 14
/* Forward and back buttons on the mouse */
static gboolean mouse_extra_buttons = TRUE;
static guint mouse_forward_button = 9;
static guint mouse_back_button = 8;

static void
apply_interface_animations (void)
{
    GtkSettings *settings = gtk_settings_get_default ();

    if (settings != NULL)
    {
        gboolean enabled = nautilus_preferences_get_boolean (NAUTILUS_PREFERENCES_UI_ANIMATIONS,
                                                             TRUE);

        g_object_set (settings, "gtk-enable-animations", enabled, NULL);
    }
}

static void
use_extra_mouse_buttons_changed (gpointer callback_data)
{
    mouse_extra_buttons = g_settings_get_boolean (nautilus_preferences, NAUTILUS_PREFERENCES_MOUSE_USE_EXTRA_BUTTONS);
}

static void
mouse_back_button_changed (gpointer callback_data)
{
    int new_back_button;

    new_back_button = g_settings_get_uint (nautilus_preferences, NAUTILUS_PREFERENCES_MOUSE_BACK_BUTTON);

    /* Bounds checking */
    if (new_back_button < 6 || new_back_button > UPPER_MOUSE_LIMIT)
    {
        return;
    }

    mouse_back_button = new_back_button;
}

static void
mouse_forward_button_changed (gpointer callback_data)
{
    int new_forward_button;

    new_forward_button = g_settings_get_uint (nautilus_preferences, NAUTILUS_PREFERENCES_MOUSE_FORWARD_BUTTON);

    /* Bounds checking */
    if (new_forward_button < 6 || new_forward_button > UPPER_MOUSE_LIMIT)
    {
        return;
    }

    mouse_forward_button = new_forward_button;
}

void
nautilus_global_preferences_init (void)
{
    static gboolean initialized = FALSE;

    if (initialized)
    {
        return;
    }

    initialized = TRUE;

    nautilus_preferences = g_settings_new ("org.gnome.nautilus.preferences");
    nautilus_compression_preferences = g_settings_new ("org.gnome.nautilus.compression");
    nautilus_window_state = g_settings_new ("org.gnome.nautilus.window-state");
    nautilus_icon_view_preferences = g_settings_new ("org.gnome.nautilus.icon-view");
    nautilus_list_view_preferences = g_settings_new ("org.gnome.nautilus.list-view");
    /* Some settings such as show hidden files are shared between Nautilus and GTK file chooser */
    gtk_filechooser_preferences = g_settings_new_with_path ("org.gtk.gtk4.Settings.FileChooser",
                                                            "/org/gtk/gtk4/settings/file-chooser/");
    gnome_lockdown_preferences = g_settings_new ("org.gnome.desktop.lockdown");
    gnome_interface_preferences = g_settings_new ("org.gnome.desktop.interface");
    gnome_privacy_preferences = g_settings_new ("org.gnome.desktop.privacy");
    localsearch_preferences = g_settings_new ("org.freedesktop.Tracker3.Miner.Files");

    if (check_schema_available (FILE_SHARING_SCHEMA_ID) &&
        check_schema_available (FILE_SHARING_SERVICE_SCHEMA_ID))
    {
        gnome_user_share_preferences = g_settings_new_with_path (FILE_SHARING_SERVICE_SCHEMA_ID,
                                                                 FILE_SHARING_SERVICE_PATH);
    }

    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_MOUSE_BACK_BUTTON,
                              G_CALLBACK (mouse_back_button_changed),
                              NULL);

    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_MOUSE_FORWARD_BUTTON,
                              G_CALLBACK (mouse_forward_button_changed),
                              NULL);

    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_MOUSE_USE_EXTRA_BUTTONS,
                              G_CALLBACK (use_extra_mouse_buttons_changed),
                              NULL);

    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_UI_ANIMATIONS,
                              G_CALLBACK (apply_interface_animations),
                              NULL);
    apply_interface_animations ();
}

gboolean
nautilus_preferences_get_boolean (const char *key,
                                  gboolean    fallback)
{
    GSettingsSchemaSource *source;
    GSettingsSchema *schema;
    gboolean has_key;
    gboolean value;

    source = g_settings_schema_source_get_default ();
    schema = source != NULL
             ? g_settings_schema_source_lookup (source,
                                                "org.gnome.nautilus.preferences",
                                                TRUE)
             : NULL;
    has_key = schema != NULL && g_settings_schema_has_key (schema, key);
    value = has_key ? g_settings_get_boolean (nautilus_preferences, key) : fallback;

    if (schema != NULL)
    {
        g_settings_schema_unref (schema);
    }

    return value;
}

gint
nautilus_preferences_get_int (const char *key,
                              gint        fallback)
{
    GSettingsSchemaSource *source;
    GSettingsSchema *schema;
    gboolean has_key;
    gint value;

    source = g_settings_schema_source_get_default ();
    schema = source != NULL
             ? g_settings_schema_source_lookup (source,
                                                "org.gnome.nautilus.preferences",
                                                TRUE)
             : NULL;
    has_key = schema != NULL && g_settings_schema_has_key (schema, key);
    value = has_key ? g_settings_get_int (nautilus_preferences, key) : fallback;

    if (schema != NULL)
    {
        g_settings_schema_unref (schema);
    }

    return value;
}

gchar *
nautilus_preferences_get_string (const char *key,
                                 const char *fallback)
{
    GSettingsSchemaSource *source;
    GSettingsSchema *schema;
    gboolean has_key;
    gchar *value;

    source = g_settings_schema_source_get_default ();
    schema = source != NULL
             ? g_settings_schema_source_lookup (source,
                                                "org.gnome.nautilus.preferences",
                                                TRUE)
             : NULL;
    has_key = schema != NULL && g_settings_schema_has_key (schema, key);
    value = has_key ? g_settings_get_string (nautilus_preferences, key)
                    : g_strdup (fallback != NULL ? fallback : "");

    if (schema != NULL)
    {
        g_settings_schema_unref (schema);
    }

    return value;
}

static gboolean
hidden_entry_points_key_available (void)
{
    GSettingsSchemaSource *source;
    GSettingsSchema *schema;
    gboolean has_key;

    source = g_settings_schema_source_get_default ();
    schema = source != NULL
             ? g_settings_schema_source_lookup (source,
                                                "org.gnome.nautilus.preferences",
                                                TRUE)
             : NULL;
    has_key = schema != NULL && g_settings_schema_has_key (schema,
                                                           NAUTILUS_PREFERENCES_HIDDEN_ENTRY_POINTS);

    if (schema != NULL)
    {
        g_settings_schema_unref (schema);
    }

    return has_key;
}

gchar **
nautilus_preferences_get_hidden_entry_points (void)
{
    if (!hidden_entry_points_key_available ())
    {
        return g_new0 (gchar *, 1);
    }

    return g_settings_get_strv (nautilus_preferences,
                                NAUTILUS_PREFERENCES_HIDDEN_ENTRY_POINTS);
}

gboolean
nautilus_preferences_is_entry_point_hidden (const char *uri)
{
    g_auto (GStrv) hidden = NULL;
    gboolean found = FALSE;

    if (uri == NULL)
    {
        return FALSE;
    }

    hidden = nautilus_preferences_get_hidden_entry_points ();
    for (guint i = 0; !found && hidden[i] != NULL; i++)
    {
        found = g_strcmp0 (hidden[i], uri) == 0;
    }

    return found;
}

void
nautilus_preferences_set_entry_point_hidden (const char *uri,
                                             gboolean    hidden)
{
    g_auto (GStrv) current = NULL;
    g_autoptr (GStrvBuilder) builder = g_strv_builder_new ();
    gboolean changed = FALSE;

    if (uri == NULL || !hidden_entry_points_key_available ())
    {
        return;
    }

    current = nautilus_preferences_get_hidden_entry_points ();

    for (guint i = 0; current[i] != NULL; i++)
    {
        if (g_strcmp0 (current[i], uri) == 0)
        {
            changed |= !hidden;
            continue;
        }

        g_strv_builder_add (builder, current[i]);
    }

    if (hidden)
    {
        changed |= !nautilus_preferences_is_entry_point_hidden (uri);
        g_strv_builder_add (builder, uri);
    }

    if (!changed)
    {
        return;
    }

    g_auto (GStrv) next = g_strv_builder_end (builder);

    g_settings_set_strv (nautilus_preferences,
                         NAUTILUS_PREFERENCES_HIDDEN_ENTRY_POINTS,
                         (const char * const *) next);
}

guint
nautilus_global_preferences_get_back_button (void)
{
    return mouse_back_button;
}

guint
nautilus_global_preferences_get_forward_button (void)
{
    return mouse_forward_button;
}

gboolean
nautilus_global_preferences_get_use_extra_buttons (void)
{
    return mouse_extra_buttons;
}
