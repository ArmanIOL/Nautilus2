/*
 * SPDX-FileCopyrightText: 2026 The Nautilus2 contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <gdk/gdk.h>
#include <gio/gio.h>

/* Builds a Dolphin-style preview for a directory by compositing up to four
 * child items into a single texture. The result is cached by the caller. */
void        nautilus_folder_thumbnail_create_async  (GFile               *location,
                                                     int                  size,
                                                     GCancellable        *cancellable,
                                                     GAsyncReadyCallback  callback,
                                                     gpointer             user_data);

GdkTexture *nautilus_folder_thumbnail_create_finish (GAsyncResult        *result,
                                                     GError             **error);
