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

#include "config.h"

#include "gplatformaudit.h"

#include "gtestutils.h"

static void
g_platform_audit_on_fd_opened (gint         fd,
                               const gchar *description)
{
}

static void
g_platform_audit_on_fd_closed (gint         fd,
                               const gchar *description)
{
}

static gboolean fd_callbacks_set = FALSE;
static GFDCallbacks fd_callbacks_storage = {
  g_platform_audit_on_fd_opened,
  g_platform_audit_on_fd_closed
};
GFDCallbacks *glib_fd_callbacks = &fd_callbacks_storage;

/**
 * g_platform_audit_set_fd_callbacks:
 * @callbacks: callbacks for keeping track of file descriptors.
 *
 * Sets the #GFDCallbacks to use for getting notified of GLib's file-descriptor
 * usage. You can use this to instrument your application's resource usage.
 *
 * Note that this function must be called before using any other GLib
 * functions.
 */
void
g_platform_audit_set_fd_callbacks (GFDCallbacks *callbacks)
{
  if (!fd_callbacks_set)
    {
      if (callbacks->on_fd_opened &&
          callbacks->on_fd_closed)
        {
          fd_callbacks_storage.on_fd_opened = callbacks->on_fd_opened;
          fd_callbacks_storage.on_fd_closed = callbacks->on_fd_closed;
          fd_callbacks_set = TRUE;
        }
      else
        g_warning (G_STRLOC ": FD callbacks are incomplete");
    }
  else
    g_warning (G_STRLOC ": FD callbacks can only be set once at startup");
}
