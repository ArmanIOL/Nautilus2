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
#define NAUTILUS_PREFERENCES_DIALOG_FOLDER_PREVIEW_ICONS_WIDGET                \
        "folder_preview_icons_row"
#define NAUTILUS_PREFERENCES_DIALOG_FOLDER_PREVIEW_ITEMS_WIDGET                \
        "folder_preview_items_row"
#define NAUTILUS_PREFERENCES_DIALOG_FILE_TOOLTIPS_WIDGET                       \
        "file_tooltips_row"
#define NAUTILUS_PREFERENCES_DIALOG_TOOLTIP_THUMBNAILS_WIDGET                  \
        "tooltip_thumbnails_row"
#define NAUTILUS_PREFERENCES_DIALOG_TOOLTIP_DATE_MODIFIED_WIDGET               \
        "tooltip_date_modified_row"
#define NAUTILUS_PREFERENCES_DIALOG_TOOLTIP_DATE_ACCESSED_WIDGET               \
        "tooltip_date_accessed_row"
#define NAUTILUS_PREFERENCES_DIALOG_TOOLTIP_DATE_CREATED_WIDGET                \
        "tooltip_date_created_row"
#define NAUTILUS_PREFERENCES_DIALOG_TERMINAL_COMMAND_WIDGET                    \
        "terminal_command_row"
#define NAUTILUS_PREFERENCES_DIALOG_SIDEBAR_POSITION_WIDGET                    \
        "sidebar_position_row"
#define NAUTILUS_PREFERENCES_DIALOG_PREVIEW_PANEL_POSITION_WIDGET              \
        "preview_panel_position_row"
#define NAUTILUS_PREFERENCES_DIALOG_PREVIEW_PANEL_WIDTH_WIDGET                 \
        "preview_panel_width_row"
#define NAUTILUS_PREFERENCES_DIALOG_DENSE_LIST_ROWS_WIDGET                     \
        "dense_list_rows_row"
#define NAUTILUS_PREFERENCES_DIALOG_LIST_COLUMN_SEPARATORS_WIDGET              \
        "list_column_separators_row"
#define NAUTILUS_PREFERENCES_DIALOG_LIST_ROW_SEPARATORS_WIDGET                 \
        "list_row_separators_row"
#define NAUTILUS_PREFERENCES_DIALOG_GRID_LABEL_LINES_WIDGET                    \
        "grid_label_lines_row"
#define NAUTILUS_PREFERENCES_DIALOG_SIDEBAR_COMPACT_WIDGET                     \
        "sidebar_compact_row"
#define NAUTILUS_PREFERENCES_DIALOG_SIDEBAR_SMALL_ICONS_WIDGET                 \
        "sidebar_small_icons_row"
#define NAUTILUS_PREFERENCES_DIALOG_ROUNDED_THUMBNAILS_WIDGET                   \
        "rounded_thumbnails_row"
#define NAUTILUS_PREFERENCES_DIALOG_THUMBNAIL_SHADOW_WIDGET                    \
        "thumbnail_shadow_row"
#define NAUTILUS_PREFERENCES_DIALOG_ANIMATIONS_WIDGET                          \
        "animations_row"
#define NAUTILUS_PREFERENCES_DIALOG_SHOW_HIDDEN_FILES_WIDGET                   \
        "show_hidden_files_row"
#define NAUTILUS_PREFERENCES_DIALOG_HIDDEN_ENTRY_POINTS_GROUP                  \
        "hidden_entry_points_group"

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

static const char * const sidebar_position_values[] =
{
    "default", "start", "end",
    NULL
};

static const char * const preview_panel_position_values[] =
{
    "default", "start", "end",
    NULL
};

static const char * const usage_accent_values[] =
{
    "system", "blue", "teal", "green", "yellow",
    "orange", "red", "pink", "purple", "slate",
    NULL
};

static void update_hidden_entry_points_group (AdwPreferencesGroup *group);

static void
unhide_entry_point_cb (GtkButton *button,
                       gpointer   user_data)
{
    AdwActionRow *row = ADW_ACTION_ROW (user_data);
    AdwPreferencesGroup *group = g_object_get_data (G_OBJECT (row), "prefs-group");
    const char *uri = g_object_get_data (G_OBJECT (row), "entry-uri");

    /* Removing the row destroys the button which emitted this signal. */
    g_object_ref (button);
    nautilus_preferences_set_entry_point_hidden (uri, FALSE);
    update_hidden_entry_points_group (group);
    g_object_unref (button);
}

static gchar *
hidden_entry_point_title (const char *uri)
{
    g_autoptr (GFile) file = g_file_new_for_uri (uri);
    g_autofree gchar *basename = g_file_get_basename (file);

    if (basename != NULL && *basename != '\0')
    {
        return g_steal_pointer (&basename);
    }

    return g_strdup (uri);
}

static void
update_hidden_entry_points_group (AdwPreferencesGroup *group)
{
    g_auto (GStrv) hidden = NULL;
    GtkWidget *row;

    while ((row = adw_preferences_group_get_row (group, 0)) != NULL)
    {
        adw_preferences_group_remove (group, row);
    }

    hidden = nautilus_preferences_get_hidden_entry_points ();

    if (hidden[0] == NULL)
    {
        row = adw_action_row_new ();
        adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row),
                                       _("No hidden entry points"));
        gtk_widget_set_sensitive (row, FALSE);
        adw_preferences_group_add (group, row);

        return;
    }

    for (guint i = 0; hidden[i] != NULL; i++)
    {
        g_autofree gchar *title = hidden_entry_point_title (hidden[i]);
        GtkWidget *button;

        row = adw_action_row_new ();
        adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title);
        adw_action_row_set_subtitle (ADW_ACTION_ROW (row), hidden[i]);
        g_object_set_data_full (G_OBJECT (row), "entry-uri",
                                g_strdup (hidden[i]), g_free);
        g_object_set_data (G_OBJECT (row), "prefs-group", group);

        button = gtk_button_new_from_icon_name ("view-reveal-symbolic");
        gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
        gtk_widget_set_tooltip_text (button, _("Show Again"));
        gtk_widget_add_css_class (button, "flat");
        g_signal_connect (button, "clicked",
                          G_CALLBACK (unhide_entry_point_cb), row);
        adw_action_row_add_suffix (ADW_ACTION_ROW (row), button);

        adw_preferences_group_add (group, row);
    }
}

static void
bind_builder_bool (GtkBuilder *builder,
                   GSettings  *settings,
                   const char *widget_name,
                   const char *prefs)
{
    g_settings_bind (settings, prefs, gtk_builder_get_object (builder, widget_name),
                     "active", G_SETTINGS_BIND_DEFAULT);
}

static void
bind_builder_string (GtkBuilder *builder,
                     GSettings  *settings,
                     const char *widget_name,
                     const char *prefs)
{
    g_settings_bind (settings, prefs, gtk_builder_get_object (builder, widget_name),
                     "text", G_SETTINGS_BIND_DEFAULT);
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
folder_preview_items_changed (AdwSpinRow *row,
                              GParamSpec *pspec,
                              gpointer    user_data)
{
    g_settings_set_int (nautilus_preferences,
                        NAUTILUS_PREFERENCES_FOLDER_PREVIEW_ITEMS,
                        (gint) adw_spin_row_get_value (row));
}

static void
setup_folder_preview_items (GtkBuilder *builder)
{
    AdwSpinRow *row = ADW_SPIN_ROW (gtk_builder_get_object (builder,
                                                            NAUTILUS_PREFERENCES_DIALOG_FOLDER_PREVIEW_ITEMS_WIDGET));

    adw_spin_row_set_value (row,
                            (double) nautilus_preferences_get_int (NAUTILUS_PREFERENCES_FOLDER_PREVIEW_ITEMS,
                                                                   4));
    g_signal_connect (row, "notify::value",
                      G_CALLBACK (folder_preview_items_changed), NULL);
}

static void
int_row_changed (AdwSpinRow *row,
                 GParamSpec *pspec,
                 gpointer    user_data)
{
    const char *key = g_object_get_data (G_OBJECT (row), "prefs-key");

    if (key != NULL)
    {
        g_settings_set_int (nautilus_preferences, key,
                            (gint) adw_spin_row_get_value (row));
    }
}

static void
setup_int_row (GtkBuilder *builder,
               const char *widget_name,
               const char *prefs,
               gint        fallback)
{
    AdwSpinRow *row = ADW_SPIN_ROW (gtk_builder_get_object (builder, widget_name));

    adw_spin_row_set_value (row, (double) nautilus_preferences_get_int (prefs, fallback));
    g_object_set_data_full (G_OBJECT (row), "prefs-key", g_strdup (prefs), g_free);
    g_signal_connect (row, "notify::value", G_CALLBACK (int_row_changed), NULL);
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
    setup_combo (builder, NAUTILUS_PREFERENCES_DIALOG_SIDEBAR_POSITION_WIDGET,
                 (const char *[]) { _("Default"), _("Left"), _("Right"), NULL });
    setup_combo (builder, NAUTILUS_PREFERENCES_DIALOG_PREVIEW_PANEL_POSITION_WIDGET,
                 (const char *[]) { _("Default"), _("Left"), _("Right"), NULL });

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
    bind_builder_string (builder, nautilus_preferences,
                         NAUTILUS_PREFERENCES_DIALOG_TERMINAL_COMMAND_WIDGET,
                         NAUTILUS_PREFERENCES_TERMINAL_COMMAND);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_DENSE_LIST_ROWS_WIDGET,
                       NAUTILUS_PREFERENCES_UI_DENSE_LIST_ROWS);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_LIST_COLUMN_SEPARATORS_WIDGET,
                       NAUTILUS_PREFERENCES_UI_LIST_COLUMN_SEPARATORS);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_LIST_ROW_SEPARATORS_WIDGET,
                       NAUTILUS_PREFERENCES_UI_LIST_ROW_SEPARATORS);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_SIDEBAR_COMPACT_WIDGET,
                       NAUTILUS_PREFERENCES_UI_SIDEBAR_COMPACT);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_SIDEBAR_SMALL_ICONS_WIDGET,
                       NAUTILUS_PREFERENCES_UI_SIDEBAR_SMALL_ICONS);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_ROUNDED_THUMBNAILS_WIDGET,
                       NAUTILUS_PREFERENCES_UI_ROUNDED_THUMBNAILS);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_THUMBNAIL_SHADOW_WIDGET,
                       NAUTILUS_PREFERENCES_UI_THUMBNAIL_SHADOW);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_ANIMATIONS_WIDGET,
                       NAUTILUS_PREFERENCES_UI_ANIMATIONS);
    bind_builder_bool (builder, gtk_filechooser_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_SHOW_HIDDEN_FILES_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_FOLDER_PREVIEW_ICONS_WIDGET,
                       NAUTILUS_PREFERENCES_FOLDER_PREVIEW_ICONS);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_FILE_TOOLTIPS_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_FILE_TOOLTIPS);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_TOOLTIP_THUMBNAILS_WIDGET,
                       NAUTILUS_PREFERENCES_TOOLTIP_THUMBNAILS);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_TOOLTIP_DATE_MODIFIED_WIDGET,
                       NAUTILUS_PREFERENCES_TOOLTIP_DATE_MODIFIED);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_TOOLTIP_DATE_ACCESSED_WIDGET,
                       NAUTILUS_PREFERENCES_TOOLTIP_DATE_ACCESSED);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DIALOG_TOOLTIP_DATE_CREATED_WIDGET,
                       NAUTILUS_PREFERENCES_TOOLTIP_DATE_CREATED);

    setup_folder_preview_items (builder);
    setup_thumbnail_limit (builder);
    setup_int_row (builder,
                   NAUTILUS_PREFERENCES_DIALOG_PREVIEW_PANEL_WIDTH_WIDGET,
                   NAUTILUS_PREFERENCES_UI_PREVIEW_PANEL_WIDTH,
                   0);
    setup_int_row (builder,
                   NAUTILUS_PREFERENCES_DIALOG_GRID_LABEL_LINES_WIDGET,
                   NAUTILUS_PREFERENCES_UI_GRID_LABEL_LINES,
                   3);
    update_hidden_entry_points_group (ADW_PREFERENCES_GROUP (
        gtk_builder_get_object (builder, NAUTILUS_PREFERENCES_DIALOG_HIDDEN_ENTRY_POINTS_GROUP)));
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
    bind_builder_combo_row (builder, nautilus_preferences,
                            NAUTILUS_PREFERENCES_DIALOG_SIDEBAR_POSITION_WIDGET,
                            NAUTILUS_PREFERENCES_UI_SIDEBAR_POSITION,
                            (const char **) sidebar_position_values);
    bind_builder_combo_row (builder, nautilus_preferences,
                            NAUTILUS_PREFERENCES_DIALOG_PREVIEW_PANEL_POSITION_WIDGET,
                            NAUTILUS_PREFERENCES_UI_PREVIEW_PANEL_POSITION,
                            (const char **) preview_panel_position_values);
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
