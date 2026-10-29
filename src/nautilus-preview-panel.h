/*
 * SPDX-FileCopyrightText: 2026 The Nautilus2 contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

#include "nautilus-file.h"

#define NAUTILUS_TYPE_PREVIEW_PANEL (nautilus_preview_panel_get_type ())

G_DECLARE_FINAL_TYPE (NautilusPreviewPanel, nautilus_preview_panel,
                      NAUTILUS, PREVIEW_PANEL, GtkWidget)

NautilusPreviewPanel *nautilus_preview_panel_new      (void);
void                  nautilus_preview_panel_set_file (NautilusPreviewPanel *self,
                                                       NautilusFile         *file);
