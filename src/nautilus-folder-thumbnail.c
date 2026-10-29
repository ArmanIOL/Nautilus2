/*
 * SPDX-FileCopyrightText: 2026 The Nautilus2 contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nautilus-folder-thumbnail.h"

#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <math.h>
#include <unistd.h>

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-desktop-thumbnail.h>

#include "nautilus-global-preferences.h"

#define FOLDER_THUMBNAIL_MAX_ITEMS 4
#define FOLDER_THUMBNAIL_SCAN_LIMIT 64

typedef struct
{
    char *name;
    gboolean is_dir;
    GdkPixbuf *pixbuf;
} FolderThumbnailEntry;

typedef struct
{
    int size;
    int max_items;
    gboolean show_icons;
} FolderThumbnailOptions;

typedef struct
{
    GPtrArray *entries;
    int size;
    gboolean show_icons;
} FolderThumbnailResult;

static void
folder_thumbnail_entry_free (FolderThumbnailEntry *entry)
{
    g_free (entry->name);
    g_clear_object (&entry->pixbuf);
    g_free (entry);
}

static void
folder_thumbnail_options_free (FolderThumbnailOptions *options)
{
    g_free (options);
}

static void
folder_thumbnail_result_free (FolderThumbnailResult *result)
{
    g_clear_pointer (&result->entries, g_ptr_array_unref);
    g_free (result);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (FolderThumbnailResult, folder_thumbnail_result_free)

static int
compare_entry_names (gconstpointer a,
                     gconstpointer b)
{
    const FolderThumbnailEntry *ea = *(FolderThumbnailEntry * const *) a;
    const FolderThumbnailEntry *eb = *(FolderThumbnailEntry * const *) b;

    /* Directories first, then files, alphabetically. */
    if (ea->is_dir != eb->is_dir)
    {
        return ea->is_dir ? -1 : 1;
    }

    return g_strcmp0 (ea->name, eb->name);
}

static GdkPixbuf *
load_pdf_thumbnail (const char *path,
                    int         size)
{
    g_autofree char *base = NULL;
    g_autofree char *output = NULL;
    g_autofree char *size_arg = g_strdup_printf ("%d", size);
    int fd;
    int status = 0;
    g_autoptr (GError) error = NULL;
    const char *argv[] = {
        "pdftoppm", "-png", "-f", "1", "-l", "1", "-singlefile",
        "-scale-to", size_arg, path, NULL, NULL
    };

    fd = g_file_open_tmp ("nautilus-folder-pdf-XXXXXX", &base, &error);

    if (fd < 0)
    {
        return NULL;
    }

    close (fd);
    g_unlink (base);
    output = g_strconcat (base, ".png", NULL);
    argv[10] = base;

    if (!g_spawn_sync (NULL, (char **) argv, NULL, G_SPAWN_SEARCH_PATH,
                       NULL, NULL, NULL, NULL, &status, &error) ||
        !g_spawn_check_wait_status (status, NULL))
    {
        g_unlink (output);
        return NULL;
    }

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale (output, size, size, TRUE, NULL);

    g_unlink (output);

    return pixbuf;
}

static GdkPixbuf *
load_pixbuf_for_file (const char *path,
                      const char *name,
                      int         size)
{
    g_autofree char *content_type = g_content_type_guess (name, NULL, 0, NULL);

    if (g_strcmp0 (content_type, "application/pdf") == 0)
    {
        return load_pdf_thumbnail (path, size);
    }

    if (g_content_type_is_a (content_type, "image/*"))
    {
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale (path, size, size, TRUE, NULL);

        if (pixbuf != NULL)
        {
            return pixbuf;
        }
    }

    /* Let the freedesktop thumbnailers deal with videos, office documents
     * and any other supported format. */
    g_autofree char *uri = g_filename_to_uri (path, NULL, NULL);

    if (uri != NULL)
    {
        GnomeDesktopThumbnailFactory *factory =
            gnome_desktop_thumbnail_factory_new (GNOME_DESKTOP_THUMBNAIL_SIZE_LARGE);
        GdkPixbuf *pixbuf = gnome_desktop_thumbnail_factory_generate_thumbnail (factory,
                                                                                uri,
                                                                                content_type,
                                                                                NULL,
                                                                                NULL);

        g_object_unref (factory);

        if (pixbuf != NULL)
        {
            return pixbuf;
        }
    }

    return NULL;
}

static void
folder_thumbnail_thread (GTask        *task,
                         gpointer      source_object,
                         gpointer      task_data,
                         GCancellable *cancellable)
{
    GFile *location = G_FILE (source_object);
    FolderThumbnailOptions *options = task_data;
    int size = options->size;
    g_autofree char *path = g_file_get_path (location);
    g_autoptr (GPtrArray) found = NULL;
    g_autoptr (GDir) dir = NULL;
    const char *name;
    int scanned = 0;

    if (path == NULL)
    {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                                 "Folder previews are only available for local folders");
        return;
    }

    dir = g_dir_open (path, 0, NULL);

    if (dir == NULL)
    {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                                 "Could not open folder");
        return;
    }

    found = g_ptr_array_new_with_free_func ((GDestroyNotify) folder_thumbnail_entry_free);

    while ((name = g_dir_read_name (dir)) != NULL && scanned < FOLDER_THUMBNAIL_SCAN_LIMIT)
    {
        g_autofree char *child_path = NULL;
        FolderThumbnailEntry *entry;

        if (name[0] == '.')
        {
            continue;
        }

        child_path = g_build_filename (path, name, NULL);
        scanned++;

        if (g_file_test (child_path, G_FILE_TEST_IS_DIR))
        {
            entry = g_new0 (FolderThumbnailEntry, 1);
            entry->name = g_strdup (name);
            entry->is_dir = TRUE;
        }
        else if (g_file_test (child_path, G_FILE_TEST_IS_REGULAR))
        {
            entry = g_new0 (FolderThumbnailEntry, 1);
            entry->name = g_strdup (name);
            entry->pixbuf = load_pixbuf_for_file (child_path, name, size);
        }
        else
        {
            continue;
        }

        g_ptr_array_add (found, entry);
    }

    if (found->len == 0)
    {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                                 "Folder is empty");
        return;
    }

    g_ptr_array_sort (found, compare_entry_names);

    /* Keep the configured number of entries, preferring those which already
     * have a generated preview. */
    while (found->len > (guint) options->max_items)
    {
        gboolean removed = FALSE;

        for (guint i = found->len; i > 0; i--)
        {
            FolderThumbnailEntry *entry = found->pdata[i - 1];

            if (entry->pixbuf == NULL)
            {
                g_ptr_array_remove_index (found, i - 1);
                removed = TRUE;
                break;
            }
        }

        if (!removed)
        {
            g_ptr_array_remove_index (found, found->len - 1);
        }
    }

    FolderThumbnailResult *result = g_new0 (FolderThumbnailResult, 1);

    result->entries = g_ptr_array_ref (found);
    result->size = size;
    result->show_icons = options->show_icons;

    g_task_return_pointer (task, result, (GDestroyNotify) folder_thumbnail_result_free);
}

void
nautilus_folder_thumbnail_create_async (GFile               *location,
                                        int                  size,
                                        GCancellable        *cancellable,
                                        GAsyncReadyCallback  callback,
                                        gpointer             user_data)
{
    g_autoptr (GTask) task = g_task_new (location, cancellable, callback, user_data);
    FolderThumbnailOptions *options = g_new0 (FolderThumbnailOptions, 1);

    /* Always render at least twice the requested size so that the preview
     * stays sharp on high density displays. */
    options->size = CLAMP (size * 2, 128, 512);
    options->max_items = CLAMP (nautilus_preferences_get_int (NAUTILUS_PREFERENCES_FOLDER_PREVIEW_ITEMS,
                                                              4), 1, FOLDER_THUMBNAIL_MAX_ITEMS);
    options->show_icons = nautilus_preferences_get_boolean (NAUTILUS_PREFERENCES_FOLDER_PREVIEW_ICONS,
                                                            TRUE);

    g_task_set_task_data (task, options, (GDestroyNotify) folder_thumbnail_options_free);
    g_task_set_priority (task, G_PRIORITY_LOW);
    g_task_run_in_thread (task, folder_thumbnail_thread);
}

static GtkIconPaintable *
lookup_icon_paintable (GtkIconTheme *theme,
                       const char   *name,
                       int           size)
{
    GtkIconPaintable *paintable = gtk_icon_theme_lookup_icon (theme, name, NULL,
                                                              size, 1,
                                                              GTK_TEXT_DIR_NONE,
                                                              GTK_ICON_LOOKUP_FORCE_REGULAR);

    if (paintable == NULL)
    {
        paintable = gtk_icon_theme_lookup_icon (theme, name, NULL,
                                                size, 1,
                                                GTK_TEXT_DIR_NONE,
                                                0);
    }

    return paintable;
}

static char *
get_fallback_icon_name (const char *name,
                        gboolean    is_dir)
{
    g_autofree char *content_type = NULL;
    g_autoptr (GIcon) icon = NULL;

    if (is_dir)
    {
        return g_strdup ("folder");
    }

    content_type = g_content_type_guess (name, NULL, 0, NULL);
    icon = g_content_type_get_icon (content_type);

    if (G_IS_THEMED_ICON (icon))
    {
        const char * const *names = g_themed_icon_get_names (G_THEMED_ICON (icon));

        if (names != NULL && names[0] != NULL)
        {
            return g_strdup (names[0]);
        }
    }

    return g_strdup ("text-x-generic");
}

static GdkPixbuf *
scale_cover (GdkPixbuf *src,
             int        target_width,
             int        target_height)
{
    int width = gdk_pixbuf_get_width (src);
    int height = gdk_pixbuf_get_height (src);
    double scale;
    int scaled_width;
    int scaled_height;
    g_autoptr (GdkPixbuf) scaled = NULL;
    int x;
    int y;

    if (width <= 0 || height <= 0 || target_width <= 0 || target_height <= 0)
    {
        return NULL;
    }

    scale = MAX ((double) target_width / width, (double) target_height / height);
    scaled_width = MAX (1, (int) ceil (width * scale));
    scaled_height = MAX (1, (int) ceil (height * scale));

    scaled = gdk_pixbuf_scale_simple (src, scaled_width, scaled_height,
                                      GDK_INTERP_BILINEAR);

    if (scaled == NULL)
    {
        return NULL;
    }

    x = MAX (0, (scaled_width - target_width) / 2);
    y = MAX (0, (scaled_height - target_height) / 2);

    return gdk_pixbuf_new_subpixbuf (scaled, x, y,
                                     MIN (target_width, scaled_width),
                                     MIN (target_height, scaled_height));
}

static void
snapshot_paintable (GtkSnapshot *snap,
                    GdkPaintable *paintable,
                    int           x,
                    int           y,
                    int           width,
                    int           height)
{
    gtk_snapshot_save (snap);
    gtk_snapshot_translate (snap, &GRAPHENE_POINT_INIT (x, y));
    gdk_paintable_snapshot (paintable, GDK_SNAPSHOT (snap), width, height);
    gtk_snapshot_restore (snap);
}

static void
snapshot_entry (GtkSnapshot           *snap,
                GtkIconTheme          *theme,
                FolderThumbnailEntry  *entry,
                gboolean               show_icons,
                int                    x,
                int                    y,
                int                    width,
                int                    height)
{
    if (entry->pixbuf != NULL)
    {
        g_autoptr (GdkPixbuf) scaled = scale_cover (entry->pixbuf, width, height);

        if (scaled != NULL)
        {
            g_autoptr (GdkTexture) texture = gdk_texture_new_for_pixbuf (scaled);

            snapshot_paintable (snap, GDK_PAINTABLE (texture), x, y, width, height);

            return;
        }
    }

    if (!show_icons)
    {
        return;
    }

    g_autofree char *icon_name = get_fallback_icon_name (entry->name, entry->is_dir);
    g_autoptr (GtkIconPaintable) icon = lookup_icon_paintable (theme, icon_name,
                                                               MIN (width, height));

    if (icon == NULL)
    {
        return;
    }

    int icon_size = MIN (width, height);
    int icon_x = x + (width - icon_size) / 2;
    int icon_y = y + (height - icon_size) / 2;

    snapshot_paintable (snap, GDK_PAINTABLE (icon), icon_x, icon_y,
                        icon_size, icon_size);
}

static void
snapshot_default_folder (GtkSnapshot *snap,
                         int          size)
{
    GdkRGBA color = { 0.31, 0.55, 0.84, 1.0 };
    graphene_rect_t bounds = GRAPHENE_RECT_INIT (size * 0.08, size * 0.16,
                                                 size * 0.84, size * 0.72);

    gtk_snapshot_append_color (snap, &color, &bounds);
}

GdkTexture *
nautilus_folder_thumbnail_create_finish (GAsyncResult  *result,
                                         GError       **error)
{
    g_autoptr (FolderThumbnailResult) thumb_result =
        g_task_propagate_pointer (G_TASK (result), error);

    if (thumb_result == NULL)
    {
        return NULL;
    }

    GPtrArray *entries = thumb_result->entries;
    int size = thumb_result->size;
    g_autoptr (GtkSnapshot) snap = gtk_snapshot_new ();
    GtkIconTheme *theme = gtk_icon_theme_get_for_display (gdk_display_get_default ());
    g_autoptr (GtkIconPaintable) folder_icon = lookup_icon_paintable (theme, "folder", size);

    if (folder_icon != NULL)
    {
        snapshot_paintable (snap, GDK_PAINTABLE (folder_icon), 0, 0, size, size);
    }
    else
    {
        snapshot_default_folder (snap, size);
    }

    /* Keep the contents inside the body of the folder icon, like Dolphin. */
    int inner_x = round (size * 0.15);
    int inner_y = round (size * 0.31);
    int inner_width = size - 2 * inner_x;
    int inner_height = round (size * 0.83) - inner_y;
    int gap = MAX (1, size / 128);
    guint n = MIN (entries->len, FOLDER_THUMBNAIL_MAX_ITEMS);

    if (n == 0)
    {
        goto out;
    }

    int cell_width;
    int cell_height;
    int total_height;

    if (n == 1)
    {
        cell_width = inner_width;
        cell_height = inner_height;
        total_height = inner_height;
    }
    else if (n == 2)
    {
        cell_width = (inner_width - gap) / 2;
        cell_height = inner_height;
        total_height = inner_height;
    }
    else
    {
        cell_width = (inner_width - gap) / 2;
        cell_height = (inner_height - gap) / 2;
        total_height = 2 * cell_height + gap;
    }

    int base_y = inner_y + (inner_height - total_height) / 2;

    for (guint i = 0; i < n; i++)
    {
        FolderThumbnailEntry *entry = entries->pdata[i];
        int row;
        int column;
        int items_in_row;
        int row_width;

        if (n <= 2)
        {
            row = 0;
            column = i;
            items_in_row = n;
        }
        else
        {
            row = i / 2;
            column = i % 2;
            items_in_row = MIN (2, n - row * 2);
        }

        row_width = items_in_row * cell_width + (items_in_row - 1) * gap;

        int x = inner_x + (inner_width - row_width) / 2 + column * (cell_width + gap);
        int y = base_y + row * (cell_height + gap);

        snapshot_entry (snap, theme, entry, thumb_result->show_icons,
                        x, y, cell_width, cell_height);
    }

out:
    {
        graphene_rect_t bounds = GRAPHENE_RECT_INIT (0, 0, size, size);
        GskRenderNode *node = gtk_snapshot_free_to_node (g_steal_pointer (&snap));
        g_autoptr (GskRenderer) renderer = gsk_cairo_renderer_new ();

        gsk_renderer_realize_for_display (renderer, gdk_display_get_default (), NULL);

        GdkTexture *texture = gsk_renderer_render_texture (renderer, node, &bounds);

        gsk_renderer_unrealize (renderer);
        gsk_render_node_unref (node);

        return texture;
    }
}
