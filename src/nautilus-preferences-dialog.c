/*
 * SPDX-FileCopyrightText: 2002 Jan Arne Petersen
 * SPDX-FileCopyrightText: 2016 Carlos Soriano <csoriano@gnome.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Authors: Jan Arne Petersen <jpetersen@uni-bonn.de>
 */

#include <config.h>

#include "nautilus-preferences-dialog.h"

#include <adwaita.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>

#include "nautilus-date-utilities.h"
#include "nautilus-global-preferences.h"

/* bool preferences */
#define NAUTILUS_PREFERENCES_DIALOG_FOLDERS_FIRST_WIDGET                       \
        "sort_folders_first_row"
#define NAUTILUS_PREFERENCES_DIALOG_DELETE_PERMANENTLY_WIDGET                  \
        "show_delete_permanently_row"
#define NAUTILUS_PREFERENCES_DIALOG_CREATE_LINK_WIDGET                         \
        "show_create_link_row"
#define NAUTILUS_PREFERENCES_DIALOG_LIST_VIEW_USE_TREE_WIDGET                  \
        "use_tree_view_row"
#define NAUTILUS_PREFERENCES_DIALOG_SHOW_SIDEBAR_TRASH_WIDGET                  \
        "show_sidebar_trash_row"
#define NAUTILUS_PREFERENCES_DIALOG_SHOW_SIDEBAR_NETWORK_WIDGET                \
        "show_sidebar_network_row"
#define NAUTILUS_PREFERENCES_DIALOG_SHOW_SIDEBAR_VOLUMES_WIDGET                \
        "show_sidebar_volumes_row"
#define NAUTILUS_PREFERENCES_DIALOG_SHOW_UNMOUNTED_VOLUMES_WIDGET              \
        "show_unmounted_volumes_row"
#define NAUTILUS_PREFERENCES_DIALOG_SHOW_VOLUME_SIZES_WIDGET                   \
        "show_volume_sizes_row"
#define NAUTILUS_PREFERENCES_DIALOG_SHOW_VOLUME_USAGE_WIDGET                   \
        "show_volume_usage_row"
#define NAUTILUS_PREFERENCES_DIALOG_FOLDER_THUMBNAILS_WIDGET                   \
        "folder_thumbnails_row"
#define NAUTILUS_PREFERENCES_DIALOG_LIST_THUMBNAILS_WIDGET                     \
        "list_thumbnails_row"
#define NAUTILUS_PREFERENCES_DIALOG_VIDEO_THUMBNAILS_WIDGET                    \
        "video_thumbnails_row"
#define NAUTILUS_PREFERENCES_DIALOG_PDF_THUMBNAILS_WIDGET                      \
        "pdf_thumbnails_row"
#define NAUTILUS_PREFERENCES_DIALOG_OFFICE_THUMBNAILS_WIDGET                   \
        "office_thumbnails_row"
#define NAUTILUS_PREFERENCES_DIALOG_PREVIEW_PANEL_WIDGET                       \
        "preview_panel_row"
#define NAUTILUS_PREFERENCES_DIALOG_THUMBNAIL_LIMIT_WIDGET                     \
        "thumbnail_limit_row"
#define NAUTILUS_PREFERENCES_DIALOG_CLEAR_THUMBNAIL_CACHE_WIDGET               \
        "clear_thumbnail_cache_button"

/* toggle group preferences */
#define NAUTILUS_PREFERENCES_DIALOG_OPEN_ACTION_TOGGLE_GROUP                   \
        "open_action_toggle_group"

/* combo preferences */
#define NAUTILUS_PREFERENCES_DIALOG_SEARCH_RECURSIVE_ROW                       \
        "search_recursive_row"
#define NAUTILUS_PREFERENCES_DIALOG_THUMBNAILS_ROW                       \
        "thumbnails_row"
#define NAUTILUS_PREFERENCES_DIALOG_COUNT_ROW                       \
        "count_row"
#define NAUTILUS_PREFERENCES_DIALOG_USAGE_ACCENT_ROW                           \
        "usage_accent_row"

static const char * const speed_tradeoff_values[] =
{
    "local-only", "always", "never",
    NULL
};

static const char * const usage_accent_values[] =
{
    "system", "blue", "teal", "green", "yellow",
    "orange", "red", "pink", "purple", "slate",
    NULL
};

static void
bind_builder_bool (GtkBuilder *builder,
                   GSettings  *settings,
                   const char *widget_name,
                   const char *prefs)
{
    g_settings_bind (settings, prefs, gtk_builder_get_object (builder, widget_name),
                     "active", G_SETTINGS_BIND_DEFAULT);
}

/* Translators: Both %s will be replaced with formatted timestamps. */
#define DATE_FORMAT_ROW_SUBTITLE _("Examples: “%s”, “%s”")

static void
setup_detailed_date (GtkBuilder *builder)
{
    AdwActionRow *simple_row = ADW_ACTION_ROW (gtk_builder_get_object (builder, "date_format_simple_row"));
    AdwActionRow *detailed_row = ADW_ACTION_ROW (gtk_builder_get_object (builder, "date_format_detailed_row"));

    g_autoptr (GDateTime) now = g_date_time_new_now_local ();
    g_autoptr (GDateTime) earlier = g_date_time_add_days (now, -3);

    g_autofree gchar *simple_date_now = nautilus_date_preview_detailed_format (now, FALSE);
    g_autofree gchar *simple_date_earlier = nautilus_date_preview_detailed_format (earlier, FALSE);
    g_autofree gchar *simple_row_subtitle = g_strdup_printf (DATE_FORMAT_ROW_SUBTITLE,
                                                             simple_date_now,
                                                             simple_date_earlier);
    adw_action_row_set_subtitle (simple_row, simple_row_subtitle);

    g_autofree gchar *detailed_date_now = nautilus_date_preview_detailed_format (now, TRUE);
    g_autofree gchar *detailed_date_earlier = nautilus_date_preview_detailed_format (earlier, TRUE);
    g_autofree gchar *detailed_row_subtitle = g_strdup_printf (DATE_FORMAT_ROW_SUBTITLE,
                                                               detailed_date_now,
                                                               detailed_date_earlier);
    adw_action_row_set_subtitle (detailed_row, detailed_row_subtitle);
}

static GVariant *
combo_row_mapping_set (const GValue       *gvalue,
                       const GVariantType *expected_type,
                       gpointer            user_data)
{
    const gchar **values = user_data;

    return g_variant_new_string (values[g_value_get_uint (gvalue)]);
}

static gboolean
combo_row_mapping_get (GValue   *gvalue,
                       GVariant *variant,
                       gpointer  user_data)
{
    const gchar **values = user_data;
    const gchar *value;

    value = g_variant_get_string (variant, NULL);

    for (int i = 0; values[i]; i++)
    {
        if (g_strcmp0 (value, values[i]) == 0)
        {
            g_value_set_uint (gvalue, i);

            return TRUE;
        }
    }

    return FALSE;
}

static void
bind_builder_combo_row (GtkBuilder  *builder,
                        GSettings   *settings,
                        const char  *widget_name,
                        const char  *prefs,
                        const char **values)
{
    g_settings_bind_with_mapping (settings, prefs, gtk_builder_get_object (builder, widget_name),
                                  "selected", G_SETTINGS_BIND_DEFAULT,
                                  combo_row_mapping_get, combo_row_mapping_set,
                                  (gpointer) values, NULL);
}

static void
setup_combo (GtkBuilder  *builder,
             const char  *widget_name,
             const char **strings)
{
    AdwComboRow *combo_row;
    g_autoptr (GtkStringList) list_store = NULL;

    combo_row = (AdwComboRow *) gtk_builder_get_object (builder, widget_name);
    g_assert (ADW_IS_COMBO_ROW (combo_row));

    list_store = gtk_string_list_new (strings);
    adw_combo_row_set_model (combo_row, G_LIST_MODEL (list_store));
}

static void
bind_builder_toggle_group (GtkBuilder *builder,
                           GSettings  *settings,
                           const char *prefs,
                           const char *group_name)
{
    AdwToggleGroup *group = ADW_TOGGLE_GROUP (gtk_builder_get_object (builder, group_name));

    g_settings_bind (settings, prefs,
                     G_OBJECT (group), "active-name",
                     G_SETTINGS_BIND_DEFAULT);
}

static void
clear_thumbnail_cache (GtkButton *button,
                       gpointer   user_data)
{
    static const char *cache_sizes[] = { "normal", "large", "x-large", "xx-large" };
    g_autofree char *cache_root = g_build_filename (g_get_user_cache_dir (), "thumbnails", NULL);

    for (guint i = 0; i < G_N_ELEMENTS (cache_sizes); i++)
    {
        g_autofree char *directory = g_build_filename (cache_root, cache_sizes[i], NULL);
        g_autoptr (GDir) dir = g_dir_open (directory, 0, NULL);
        const char *name;

        if (dir == NULL)
        {
            continue;
        }

        while ((name = g_dir_read_name (dir)) != NULL)
        {
            g_autofree char *path = g_build_filename (directory, name, NULL);

            g_unlink (path);
        }
    }

    gtk_button_set_label (button, _("Cleared"));
    gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
}

static void
thumbnail_limit_changed (AdwSpinRow *row,
                         GParamSpec *pspec,
                         gpointer    user_data)
{
    g_settings_set_uint64 (nautilus_preferences,
                           NAUTILUS_PREFERENCES_FILE_THUMBNAIL_LIMIT,
                           (guint64) adw_spin_row_get_value (row));
}

static void
setup_thumbnail_limit (GtkBuilder *builder)
{
    AdwSpinRow *row = ADW_SPIN_ROW (gtk_builder_get_object (builder,
                                                            NAUTILUS_PREFERENCES_DIALOG_THUMBNAIL_LIMIT_WIDGET));

    adw_spin_row_set_value (row,
                            (double) g_settings_get_uint64 (nautilus_preferences,
                                                            NAUTILUS_PREFERENCES_FILE_THUMBNAIL_LIMIT));
    g_signal_connect (row, "notify::value", G_CALLBACK (thumbnail_limit_changed), NULL);
}

static void
nautilus_preferences_dialog_setup (GtkBuilder *builder)
{
    setup_combo (builder, NAUTILUS_PREFERENCES_DIALOG_SEARCH_RECURSIVE_ROW,
                 (const char *[]) { _("On This Device Only"), _("All Locations"), _("Never"), NULL });
    setup_combo (builder, NAUTILUS_PREFERENCES_DIALOG_THUMBNAILS_ROW,
                 (const char *[]) { _("On This Device Only"), _("All Files"), _("Never"), NULL });
    setup_combo (builder, NAUTILUS_PREFERENCES_DIALOG_COUNT_ROW,
                 (const char *[]) { _("On This Device Only"), _("All Folders"), _("Never"), NULL });
    setup_combo (builder, NAUTILUS_PREFERENCES_DIALOG_USAGE_ACCENT_ROW,
                 (const char *[]) { _("Follow GNOME Accent"), _("Blue"), _("Teal"),
                                    _("Green"), _("Yellow"), _("Orange"), _("Red"),
                                    _("Pink"), _("Purple"), _("Slate"), NULL });

    /* setup preferences */
    bind_builder_bool (builder, gtk_filechooser_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_FOLDERS_FIRST_WIDGET,
                       NAUTILUS_PREFERENCES_SORT_DIRECTORIES_FIRST);
    bind_builder_bool (builder, nautilus_list_view_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_LIST_VIEW_USE_TREE_WIDGET,
                       NAUTILUS_PREFERENCES_LIST_VIEW_USE_TREE);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_CREATE_LINK_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_CREATE_LINK);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_DELETE_PERMANENTLY_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_DELETE_PERMANENTLY);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_SHOW_SIDEBAR_TRASH_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_SIDEBAR_TRASH);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_SHOW_SIDEBAR_NETWORK_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_SIDEBAR_NETWORK);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_SHOW_SIDEBAR_VOLUMES_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_SIDEBAR_VOLUMES);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_SHOW_UNMOUNTED_VOLUMES_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_UNMOUNTED_VOLUMES);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_SHOW_VOLUME_SIZES_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_VOLUME_SIZES);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_SHOW_VOLUME_USAGE_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_VOLUME_USAGE);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_FOLDER_THUMBNAILS_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_FOLDER_THUMBNAILS);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_LIST_THUMBNAILS_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_LIST_THUMBNAILS);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_VIDEO_THUMBNAILS_WIDGET,
                       NAUTILUS_PREFERENCES_THUMBNAIL_VIDEO);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_PDF_THUMBNAILS_WIDGET,
                       NAUTILUS_PREFERENCES_THUMBNAIL_PDF);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_OFFICE_THUMBNAILS_WIDGET,
                       NAUTILUS_PREFERENCES_THUMBNAIL_OFFICE);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_PREVIEW_PANEL_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_PREVIEW_PANEL);

    setup_thumbnail_limit (builder);
    g_signal_connect (gtk_builder_get_object (builder,
                                              NAUTILUS_PREFERENCES_DIALOG_CLEAR_THUMBNAIL_CACHE_WIDGET),
                      "clicked", G_CALLBACK (clear_thumbnail_cache), NULL);

    setup_detailed_date (builder);

    bind_builder_toggle_group (builder, nautilus_preferences,
                               NAUTILUS_PREFERENCES_CLICK_POLICY,
                               NAUTILUS_PREFERENCES_DIALOG_OPEN_ACTION_TOGGLE_GROUP);
    bind_builder_combo_row (builder, nautilus_preferences,
                            NAUTILUS_PREFERENCES_DIALOG_SEARCH_RECURSIVE_ROW,
                            NAUTILUS_PREFERENCES_RECURSIVE_SEARCH,
                            (const char **) speed_tradeoff_values);
    bind_builder_combo_row (builder, nautilus_preferences,
                            NAUTILUS_PREFERENCES_DIALOG_THUMBNAILS_ROW,
                            NAUTILUS_PREFERENCES_SHOW_FILE_THUMBNAILS,
                            (const char **) speed_tradeoff_values);
    bind_builder_combo_row (builder, nautilus_preferences,
                            NAUTILUS_PREFERENCES_DIALOG_COUNT_ROW,
                            NAUTILUS_PREFERENCES_SHOW_DIRECTORY_ITEM_COUNTS,
                            (const char **) speed_tradeoff_values);
    bind_builder_combo_row (builder, nautilus_preferences,
                            NAUTILUS_PREFERENCES_DIALOG_USAGE_ACCENT_ROW,
                            NAUTILUS_PREFERENCES_SIDEBAR_USAGE_ACCENT,
                            (const char **) usage_accent_values);
}

void
nautilus_preferences_dialog_show (GtkWidget *parent)
{
    static AdwPreferencesDialog *preferences_dialog = NULL;
    g_autoptr (GtkBuilder) builder = NULL;
    g_autoptr (GSimpleActionGroup) action_group = g_simple_action_group_new ();
    g_autoptr (GAction) date_time_action = NULL;

    if (preferences_dialog != NULL)
    {
        /* Destroy existing window, which might be hidden behind other windows,
         * attached to another parent. */
        adw_dialog_force_close (ADW_DIALOG (preferences_dialog));
        g_clear_weak_pointer (&preferences_dialog);
    }

    builder = gtk_builder_new_from_resource ("/org/gnome/nautilus/ui/nautilus-preferences-dialog.ui");
    nautilus_preferences_dialog_setup (builder);

    preferences_dialog = ADW_PREFERENCES_DIALOG (gtk_builder_get_object (builder, "preferences_dialog"));
    g_object_add_weak_pointer (G_OBJECT (preferences_dialog), (gpointer *) &preferences_dialog);

    date_time_action = g_settings_create_action (nautilus_preferences,
                                                 NAUTILUS_PREFERENCES_DATE_TIME_FORMAT);
    g_action_map_add_action (G_ACTION_MAP (action_group), date_time_action);
    gtk_widget_insert_action_group (GTK_WIDGET (preferences_dialog),
                                    "preferences",
                                    G_ACTION_GROUP (action_group));

    adw_dialog_present (ADW_DIALOG (preferences_dialog), parent);
}
