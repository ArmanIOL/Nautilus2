/*
 * SPDX-FileCopyrightText: 2026 The Nautilus2 contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

#include "nautilus-file.h"

GtkWidget *nautilus_file_tooltip_new   (NautilusFile *file);
gboolean   nautilus_file_tooltip_query (GtkWidget    *widget,
                                        GtkTooltip   *tooltip);
