/*
 * SPDX-FileCopyrightText: 2026 The Nautilus2 contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nautilus-preview-panel.h"

#include <glib/gi18n.h>

#include "nautilus-file.h"
#include "nautilus-image.h"

#define PREVIEW_IMAGE_SIZE 256

struct _NautilusPreviewPanel
{
    GtkWidget parent_instance;

    GtkWidget *image;
    GtkLabel *name_label;
    GtkLabel *type_label;
    GtkLabel *size_label;
    GtkLabel *modified_label;
    GtkLabel *location_label;
    GtkWidget *empty_state;

    NautilusFile *file;
};

G_DEFINE_FINAL_TYPE (NautilusPreviewPanel, nautilus_preview_panel, GTK_TYPE_WIDGET)

static GtkWidget *
add_detail_row (GtkGrid    *grid,
                int         row,
                const char *title,
                GtkLabel  **value_out)
{
    GtkWidget *title_label = gtk_label_new (title);
    GtkWidget *value_label = gtk_label_new (NULL);

    gtk_widget_add_css_class (title_label, "dim-label");
    gtk_label_set_xalign (GTK_LABEL (title_label), 0);
    gtk_label_set_xalign (GTK_LABEL (value_label), 0);
    gtk_label_set_wrap (GTK_LABEL (value_label), TRUE);
    gtk_label_set_wrap_mode (GTK_LABEL (value_label), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_selectable (GTK_LABEL (value_label), TRUE);
    gtk_label_set_ellipsize (GTK_LABEL (value_label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_widget_set_hexpand (value_label, TRUE);

    gtk_grid_attach (grid, title_label, 0, row, 1, 1);
    gtk_grid_attach (grid, value_label, 1, row, 1, 1);

    *value_out = GTK_LABEL (value_label);

    return value_label;
}

static void
nautilus_preview_panel_init (NautilusPreviewPanel *self)
{
    gtk_widget_add_css_class (GTK_WIDGET (self), "preview-panel");

    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 18);
    GtkWidget *scrolled = gtk_scrolled_window_new ();
    GtkWidget *picture_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
    GtkWidget *grid = gtk_grid_new ();

    gtk_widget_set_margin_top (box, 18);
    gtk_widget_set_margin_bottom (box, 18);
    gtk_widget_set_margin_start (box, 18);
    gtk_widget_set_margin_end (box, 18);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled), box);
    gtk_widget_set_hexpand (scrolled, TRUE);
    gtk_widget_set_vexpand (scrolled, TRUE);

    self->image = GTK_WIDGET (nautilus_image_new ());
    nautilus_image_set_size (NAUTILUS_IMAGE (self->image), PREVIEW_IMAGE_SIZE);
    gtk_widget_set_halign (self->image, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request (self->image, PREVIEW_IMAGE_SIZE, PREVIEW_IMAGE_SIZE);
    gtk_widget_add_css_class (self->image, "preview-image");
    gtk_box_append (GTK_BOX (picture_box), self->image);

    self->name_label = GTK_LABEL (gtk_label_new (NULL));
    gtk_label_set_wrap (self->name_label, TRUE);
    gtk_label_set_wrap_mode (self->name_label, PANGO_WRAP_WORD_CHAR);
    gtk_label_set_justify (self->name_label, GTK_JUSTIFY_CENTER);
    gtk_label_set_lines (self->name_label, 3);
    gtk_label_set_ellipsize (self->name_label, PANGO_ELLIPSIZE_MIDDLE);
    gtk_widget_add_css_class (GTK_WIDGET (self->name_label), "title-3");
    gtk_box_append (GTK_BOX (picture_box), GTK_WIDGET (self->name_label));

    gtk_box_append (GTK_BOX (box), picture_box);

    gtk_grid_set_row_spacing (GTK_GRID (grid), 8);
    gtk_grid_set_column_spacing (GTK_GRID (grid), 12);

    add_detail_row (GTK_GRID (grid), 0, _("Type"), &self->type_label);
    add_detail_row (GTK_GRID (grid), 1, _("Size"), &self->size_label);
    add_detail_row (GTK_GRID (grid), 2, _("Modified"), &self->modified_label);
    add_detail_row (GTK_GRID (grid), 3, _("Location"), &self->location_label);

    gtk_box_append (GTK_BOX (box), grid);

    self->empty_state = gtk_label_new (_("Select a file to preview it"));
    gtk_widget_add_css_class (self->empty_state, "dim-label");
    gtk_label_set_wrap (GTK_LABEL (self->empty_state), TRUE);
    gtk_label_set_justify (GTK_LABEL (self->empty_state), GTK_JUSTIFY_CENTER);
    gtk_widget_set_valign (self->empty_state, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand (self->empty_state, TRUE);
    gtk_box_append (GTK_BOX (box), self->empty_state);

    gtk_widget_set_parent (scrolled, GTK_WIDGET (self));
}

static void
nautilus_preview_panel_dispose (GObject *object)
{
    NautilusPreviewPanel *self = NAUTILUS_PREVIEW_PANEL (object);

    g_clear_object (&self->file);

    GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self));

    if (child != NULL)
    {
        gtk_widget_unparent (child);
    }

    G_OBJECT_CLASS (nautilus_preview_panel_parent_class)->dispose (object);
}

static void
update_location (NautilusPreviewPanel *self)
{
    g_autoptr (GFile) parent = NULL;
    g_autofree char *location = NULL;

    if (self->file == NULL)
    {
        gtk_label_set_text (self->location_label, "");
        return;
    }

    parent = nautilus_file_get_parent_location (self->file);

    if (parent != NULL)
    {
        location = g_file_get_parse_name (parent);
    }

    gtk_label_set_text (self->location_label, location != NULL ? location : "");
}

static void
update_preview (NautilusPreviewPanel *self)
{
    g_autoptr (GdkPaintable) fallback = NULL;
    g_autoptr (GFile) location = NULL;
    g_autofree char *type = NULL;
    g_autofree char *size = NULL;
    g_autofree char *modified = NULL;
    gint scale;
    gboolean show_thumbnail;

    if (self->file == NULL)
    {
        gtk_widget_set_visible (self->empty_state, TRUE);
        gtk_widget_set_visible (self->image, FALSE);
        gtk_label_set_text (self->name_label, "");
        gtk_label_set_text (self->type_label, "");
        gtk_label_set_text (self->size_label, "");
        gtk_label_set_text (self->modified_label, "");
        gtk_label_set_text (self->location_label, "");

        return;
    }

    gtk_widget_set_visible (self->empty_state, FALSE);
    gtk_widget_set_visible (self->image, TRUE);

    scale = gtk_widget_get_scale_factor (GTK_WIDGET (self));
    fallback = nautilus_file_get_icon_paintable (self->file, PREVIEW_IMAGE_SIZE, scale,
                                                 NAUTILUS_FILE_ICON_FLAGS_NONE);
    nautilus_image_set_fallback (NAUTILUS_IMAGE (self->image), fallback);

    show_thumbnail = nautilus_file_should_show_thumbnail (self->file);
    location = nautilus_file_get_location (self->file);

    nautilus_image_set_source (NAUTILUS_IMAGE (self->image),
                               show_thumbnail ? location : NULL);

    gtk_label_set_text (self->name_label, nautilus_file_get_display_name (self->file));

    type = nautilus_file_get_string_attribute (self->file, "type");
    size = nautilus_file_get_string_attribute (self->file, "size");
    modified = nautilus_file_get_string_attribute (self->file, "date_modified");

    gtk_label_set_text (self->type_label, type != NULL ? type : "");
    gtk_label_set_text (self->size_label, size != NULL ? size : "");
    gtk_label_set_text (self->modified_label, modified != NULL ? modified : "");

    update_location (self);
}

void
nautilus_preview_panel_set_file (NautilusPreviewPanel *self,
                                 NautilusFile         *file)
{
    g_return_if_fail (NAUTILUS_IS_PREVIEW_PANEL (self));
    g_return_if_fail (file == NULL || NAUTILUS_IS_FILE (file));

    if (self->file == file)
    {
        return;
    }

    g_set_object (&self->file, file);
    update_preview (self);
}

NautilusPreviewPanel *
nautilus_preview_panel_new (void)
{
    return g_object_new (NAUTILUS_TYPE_PREVIEW_PANEL, NULL);
}

static void
nautilus_preview_panel_class_init (NautilusPreviewPanelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->dispose = nautilus_preview_panel_dispose;

    g_type_ensure (NAUTILUS_TYPE_IMAGE);

    gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}
