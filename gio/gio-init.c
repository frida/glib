/*
 * Copyright © 2014 Ole André Vadla Ravnås <oleavr@gmail.com>
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

#include "gio-init.h"

#ifdef GIO_STATIC_COMPILATION

void
gio_init (void)
{
}

void
gio_shutdown (void)
{
  _g_dbus_shutdown ();
  _g_task_shutdown ();
}

void
gio_deinit (void)
{
  gio_shutdown ();

  _g_dbus_deinit ();
}

#else

void
gio_init (void)
{
}

void
gio_shutdown (void)
{
}

void
gio_deinit (void)
{
}

#endif
