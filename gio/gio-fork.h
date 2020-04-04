/*
 * Copyright © 2018 Ole André Vadla Ravnås <oleavr@gmail.com>
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

#ifndef __GIO_FORK_H__
#define __GIO_FORK_H__

#include "gio.h"

G_GNUC_INTERNAL void _g_dbus_prepare_to_fork (void);
G_GNUC_INTERNAL void _g_dbus_recover_from_fork_in_parent (void);
G_GNUC_INTERNAL void _g_dbus_recover_from_fork_in_child (void);

#endif
