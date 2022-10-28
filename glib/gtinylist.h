/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __G_TINYLIST_H__
#define __G_TINYLIST_H__

#include <glib/gmem.h>

G_BEGIN_DECLS

typedef struct _GTinyList GTinyList;

struct _GTinyList
{
  gpointer data;
  GTinyList *next;
};

G_GNUC_INTERNAL
void        g_tinylist_free     (GTinyList *list);

G_GNUC_INTERNAL
GTinyList*  g_tinylist_prepend  (GTinyList *list,
                                 gpointer   data);
G_GNUC_INTERNAL
GTinyList*  g_tinylist_remove   (GTinyList    *list,
                                 gconstpointer data);

G_GNUC_INTERNAL
void g_tinylist_foreach         (GTinyList *list,
                                 GFunc      func,
                                 gpointer   user_data);

G_END_DECLS

#endif /* __G_TINYLIST_H__ */
