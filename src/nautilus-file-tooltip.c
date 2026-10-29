/*
 * SPDX-FileCopyrightText: 2026 The Nautilus2 contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nautilus-file-tooltip.h"

#include <glib/gi18n.h>

#include "nautilus-global-preferences.h"
#include "nautilus-image.h"
#include "nautilus-view-cell.h"
#include "nautilus-view-item.h"

#define TOOLTIP_IMAGE_SIZE 96

static void
add_detail_row (GtkGrid    *grid,
                int         row,
                const char *title,
                const char *value)
{
    GtkWidget *title_label = gtk_label_new (title);
    GtkWidget *value_label = gtk_label_new (value != NULL ? value : "—");

    gtk_widget_add_css_class (title_label, "dim-label");
    gtk_label_set_xalign (GTK_LABEL (title_label), 0);
    gtk_label_set_xalign (GTK_LABEL (value_label), 0);
    gtk_label_set_selectable (GTK_LABEL (value_label), TRUE);
    gtk_label_set_ellipsize (GTK_LABEL (value_label), PANGO_ELLIPSIZE_END);
    gtk_label_set_max_width_chars (GTK_LABEL (value_label), 34);
    gtk_widget_set_hexpand (value_label, TRUE);

    gtk_grid_attach (grid, title_label, 0, row, 1, 1);
    gtk_grid_attach (grid, value_label, 1, row, 1, 1);
}

GtkWidget *
nautilus_file_tooltip_new (NautilusFile *file)
{
    g_return_val_if_fail (NAUTILUS_IS_FILE (file), NULL);

    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *details = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget *name_label = gtk_label_new (nautilus_file_get_display_name (file));
    GtkWidget *grid = gtk_grid_new ();

    gtk_widget_add_css_class (box, "file-tooltip");
    gtk_widget_set_margin_top (box, 6);
    gtk_widget_set_margin_bottom (box, 6);
    gtk_widget_set_margin_start (box, 6);
    gtk_widget_set_margin_end (box, 6);

    if (nautilus_preferences_get_boolean (NAUTILUS_PREFERENCES_TOOLTIP_THUMBNAILS, TRUE))
    {
        GtkWidget *image = GTK_WIDGET (nautilus_image_new ());
        gint scale = gtk_widget_get_scale_factor (box);
        g_autoptr (GdkPaintable) fallback =
            nautilus_file_get_icon_paintable (file, TOOLTIP_IMAGE_SIZE, scale,
                                              NAUTILUS_FILE_ICON_FLAGS_NONE);
        g_autoptr (GFile) location = nautilus_file_get_location (file);
        gboolean show_thumbnail = nautilus_file_should_show_thumbnail (file);

        nautilus_image_set_size (NAUTILUS_IMAGE (image), TOOLTIP_IMAGE_SIZE);
        nautilus_image_set_fallback (NAUTILUS_IMAGE (image), fallback);
        nautilus_image_set_source (NAUTILUS_IMAGE (image),
                                   show_thumbnail ? location : NULL);

        gtk_widget_set_size_request (image, TOOLTIP_IMAGE_SIZE, TOOLTIP_IMAGE_SIZE);
        gtk_widget_set_valign (image, GTK_ALIGN_START);
        gtk_box_append (GTK_BOX (box), image);
    }

    gtk_label_set_xalign (GTK_LABEL (name_label), 0);
    gtk_label_set_ellipsize (GTK_LABEL (name_label), PANGO_ELLIPSIZE_END);
    gtk_label_set_max_width_chars (GTK_LABEL (name_label), 34);
    gtk_widget_add_css_class (name_label, "heading");
    gtk_box_append (GTK_BOX (details), name_label);

    gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
    gtk_grid_set_column_spacing (GTK_GRID (grid), 12);

    g_autofree char *type = nautilus_file_get_string_attribute (file, "type");
    g_autofree char *size = nautilus_file_get_string_attribute (file, "size");
    g_autofree char *modified = nautilus_file_get_string_attribute (file, "date_modified");
    g_autofree char *accessed = nautilus_file_get_string_attribute (file, "date_accessed");
    g_autofree char *created = nautilus_file_get_string_attribute (file, "date_created");
    int row = 0;

    add_detail_row (GTK_GRID (grid), row++, _("Type"), type);
    add_detail_row (GTK_GRID (grid), row++, _("Size"), size);

    if (nautilus_preferences_get_boolean (NAUTILUS_PREFERENCES_TOOLTIP_DATE_MODIFIED, TRUE))
    {
        add_detail_row (GTK_GRID (grid), row++, _("Modified"), modified);
    }

    if (nautilus_preferences_get_boolean (NAUTILUS_PREFERENCES_TOOLTIP_DATE_ACCESSED, TRUE))
    {
        add_detail_row (GTK_GRID (grid), row++, _("Accessed"), accessed);
    }

    if (nautilus_preferences_get_boolean (NAUTILUS_PREFERENCES_TOOLTIP_DATE_CREATED, TRUE))
    {
        add_detail_row (GTK_GRID (grid), row++, _("Created"), created);
    }

    gtk_box_append (GTK_BOX (details), grid);
    gtk_box_append (GTK_BOX (box), details);

    return box;
}

gboolean
nautilus_file_tooltip_query (GtkWidget  *widget,
                             GtkTooltip *tooltip)
{
    g_autoptr (NautilusViewItem) item = NULL;
    NautilusFile *file;
    GtkWidget *custom;
    GtkWidget *cell;

    if (!nautilus_preferences_get_boolean (NAUTILUS_PREFERENCES_SHOW_FILE_TOOLTIPS, TRUE))
    {
        return FALSE;
    }

    /* Tooltips may be queried for child widgets such as the name label. */
    cell = NAUTILUS_IS_VIEW_CELL (widget)
           ? widget
           : gtk_widget_get_ancestor (widget, NAUTILUS_TYPE_VIEW_CELL);

    if (cell == NULL)
    {
        return FALSE;
    }

    item = nautilus_view_cell_get_item (NAUTILUS_VIEW_CELL (cell));

    if (item == NULL)
    {
        return FALSE;
    }

    file = nautilus_view_item_get_file (item);

    if (file == NULL)
    {
        return FALSE;
    }

    custom = nautilus_file_tooltip_new (file);

    if (custom == NULL)
    {
        return FALSE;
    }

    gtk_tooltip_set_custom (tooltip, custom);

    return TRUE;
}
