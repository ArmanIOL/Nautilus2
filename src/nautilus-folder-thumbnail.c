/*
 * SPDX-FileCopyrightText: 2026 The Nautilus2 contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nautilus-folder-thumbnail.h"

#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <unistd.h>

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-desktop-thumbnail.h>

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
    GPtrArray *entries;
    int size;
} FolderThumbnailResult;

static void
folder_thumbnail_entry_free (FolderThumbnailEntry *entry)
{
    g_free (entry->name);
    g_clear_object (&entry->pixbuf);
    g_free (entry);
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
    int size = GPOINTER_TO_INT (task_data);
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

    /* Keep at most four entries, preferring those which already have a
     * generated preview. */
    while (found->len > FOLDER_THUMBNAIL_MAX_ITEMS)
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

    /* Always render at least twice the requested size so that the preview
     * stays sharp on high density displays. */
    size = CLAMP (size * 2, 128, 512);

    g_task_set_task_data (task, GINT_TO_POINTER (size), NULL);
    g_task_set_priority (task, G_PRIORITY_LOW);
    g_task_run_in_thread (task, folder_thumbnail_thread);
}

static GdkPixbuf *
scale_cover (GdkPixbuf *src,
             int        size)
{
    int width = gdk_pixbuf_get_width (src);
    int height = gdk_pixbuf_get_height (src);
    double scale;
    int scaled_width;
    int scaled_height;
    g_autoptr (GdkPixbuf) scaled = NULL;
    int x;
    int y;

    if (width <= 0 || height <= 0)
    {
        return NULL;
    }

    scale = MAX ((double) size / width, (double) size / height);
    scaled_width = MAX (1, (int) ceil (width * scale));
    scaled_height = MAX (1, (int) ceil (height * scale));

    scaled = gdk_pixbuf_scale_simple (src, scaled_width, scaled_height,
                                      GDK_INTERP_BILINEAR);

    if (scaled == NULL)
    {
        return NULL;
    }

    x = MAX (0, (scaled_width - size) / 2);
    y = MAX (0, (scaled_height - size) / 2);

    if (x == 0 && y == 0 && scaled_width == size && scaled_height == size)
    {
        return (GdkPixbuf *) g_object_ref (scaled);
    }

    return gdk_pixbuf_new_subpixbuf (scaled, x, y, MIN (size, scaled_width),
                                     MIN (size, scaled_height));
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
    int gap = MAX (2, size / 64);
    g_autoptr (GdkPixbuf) base = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, size, size);

    if (base == NULL)
    {
        g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                             "Could not allocate folder preview");
        return NULL;
    }

    gdk_pixbuf_fill (base, 0x00000000);

    guint n = MIN (entries->len, FOLDER_THUMBNAIL_MAX_ITEMS);
    int cell = (n == 1) ? size : (size - gap) / 2;
    GtkIconTheme *theme = gtk_icon_theme_get_for_display (gdk_display_get_default ());

    for (guint i = 0; i < n; i++)
    {
        FolderThumbnailEntry *entry = entries->pdata[i];
        g_autoptr (GdkPixbuf) scaled = NULL;
        int x;
        int y;

        if (n == 1)
        {
            x = 0;
            y = 0;
        }
        else if (n == 2)
        {
            x = i * (cell + gap);
            y = (size - cell) / 2;
        }
        else
        {
            x = (i % 2) * (cell + gap);
            y = (i / 2) * (cell + gap);
        }

        if (entry->pixbuf != NULL)
        {
            scaled = scale_cover (entry->pixbuf, cell);
        }

        if (scaled == NULL)
        {
            g_autofree char *icon_name = get_fallback_icon_name (entry->name, entry->is_dir);
            g_autoptr (GtkIconPaintable) icon_paintable =
                gtk_icon_theme_lookup_icon (theme, icon_name, NULL, cell, 1,
                                            GTK_TEXT_DIR_NONE, 0);

            if (icon_paintable != NULL)
            {
                g_autoptr (GFile) icon_file = gtk_icon_paintable_get_file (icon_paintable);

                if (icon_file != NULL)
                {
                    g_autofree char *icon_path = g_file_get_path (icon_file);

                    if (icon_path != NULL)
                    {
                        scaled = gdk_pixbuf_new_from_file_at_scale (icon_path, cell, cell,
                                                                    TRUE, NULL);
                    }
                }
            }
        }

        if (scaled == NULL)
        {
            continue;
        }

        gdk_pixbuf_composite (scaled, base, x, y,
                              gdk_pixbuf_get_width (scaled),
                              gdk_pixbuf_get_height (scaled),
                              0, 0, 1.0, 1.0,
                              GDK_INTERP_BILINEAR, 255);
    }

    return gdk_texture_new_for_pixbuf (base);
}
