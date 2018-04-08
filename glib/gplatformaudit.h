/*
 * Copyright © 2018 Ole André Vadla Ravnås <oleavr@nowsecure.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __G_PLATFORM_AUDIT_H__
#define __G_PLATFORM_AUDIT_H__

#if !defined (__GLIB_H_INSIDE__) && !defined (GLIB_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>

G_BEGIN_DECLS

typedef struct _GFDCallbacks GFDCallbacks;

struct _GFDCallbacks
{
  void (*on_fd_opened) (gint fd, const gchar *description);
  void (*on_fd_closed) (gint fd, const gchar *description);
};

GLIB_VAR
GFDCallbacks       *glib_fd_callbacks;
GLIB_AVAILABLE_IN_2_62
void                g_platform_audit_set_fd_callbacks (GFDCallbacks *callbacks);

G_END_DECLS

#endif /* __G_PLATFORM_AUDIT_H__ */
