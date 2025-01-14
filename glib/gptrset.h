/*
 * Copyright © 2024 Ole André Vadla Ravnås <oleavr@frida.re>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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

#ifndef __G_PTR_SET_H__
#define __G_PTR_SET_H__

#include <glib/gtypes.h>

G_BEGIN_DECLS

typedef struct _GPtrSet      GPtrSet;
typedef struct _GPtrIndexMap GPtrIndexMap;

struct _GPtrSet
{
  gpointer     *items;
  gsize         size;
  gsize         capacity;
  GPtrIndexMap *index_map;
};

G_GNUC_INTERNAL
GPtrSet *   g_ptr_set_new      (void);
G_GNUC_INTERNAL
void        g_ptr_set_free     (GPtrSet  *pset);
G_GNUC_INTERNAL
void        g_ptr_set_add      (GPtrSet  *pset,
                                gpointer  ptr);
G_GNUC_INTERNAL
void        g_ptr_set_remove   (GPtrSet  *pset,
                                gpointer  ptr);
G_GNUC_INTERNAL
void        g_ptr_set_foreach  (GPtrSet  *pset,
                                GFunc     func,
                                gpointer  user_data);

G_END_DECLS

#endif
