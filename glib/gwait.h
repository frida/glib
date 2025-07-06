/*
 * Copyright © 2025 Ole André Vadla Ravnås <oleavr@frida.re>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __G_WAIT_H__
#define __G_WAIT_H__

#if !defined (__GLIB_H_INSIDE__) && !defined (GLIB_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>

#define G_WAIT_INFINITE (-1)

G_BEGIN_DECLS

GLIB_AVAILABLE_IN_2_68
void            g_wait_sleep            (gpointer token, gint64 timeout_us);
GLIB_AVAILABLE_IN_2_68
void            g_wait_wake             (gpointer token);
GLIB_AVAILABLE_IN_2_68
gboolean        g_wait_is_set           (gpointer token);

G_END_DECLS

#endif
