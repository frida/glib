/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gthread.c: posix thread system implementation
 * Copyright 1998 Sebastian Wilhelmi; University of Karlsruhe
 * Copyright 2025 Ole André Vadla Ravnås <oleavr@frida.re>
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

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/* The GMutex, GCond and GPrivate implementations in this file are some
 * of the lowest-level code in GLib.  All other parts of GLib (messages,
 * memory, slices, etc) assume that they can freely use these facilities
 * without risking recursion.
 *
 * As such, these functions are NOT permitted to call any other part of
 * GLib.
 *
 * The thread manipulation functions (create, exit, join, etc.) have
 * more freedom -- they can do as they please.
 */

#include "config.h"

#include "gthread.h"

#include "glib-init.h"
#include "gslice.h"
#include "gstrfuncs.h"
#include "gthreadprivate.h"

#include <stdio.h>

/* {{{1 GMutex */

G_GNUC_WEAK void
g_mutex_init (GMutex *mutex)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
g_mutex_clear (GMutex *mutex)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
g_mutex_lock (GMutex *mutex)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
g_mutex_unlock (GMutex *mutex)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK gboolean
g_mutex_trylock (GMutex *mutex)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
  return FALSE;
}

/* {{{1 GRecMutex */

G_GNUC_WEAK void
g_rec_mutex_init (GRecMutex *rec_mutex)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
g_rec_mutex_clear (GRecMutex *rec_mutex)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
g_rec_mutex_lock (GRecMutex *mutex)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
g_rec_mutex_unlock (GRecMutex *rec_mutex)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK gboolean
g_rec_mutex_trylock (GRecMutex *rec_mutex)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
  return FALSE;
}

/* {{{1 GRWLock */

G_GNUC_WEAK void
g_rw_lock_init (GRWLock *rw_lock)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
g_rw_lock_clear (GRWLock *rw_lock)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
g_rw_lock_writer_lock (GRWLock *rw_lock)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK gboolean
g_rw_lock_writer_trylock (GRWLock *rw_lock)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
  return FALSE;
}

G_GNUC_WEAK void
g_rw_lock_writer_unlock (GRWLock *rw_lock)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
g_rw_lock_reader_lock (GRWLock *rw_lock)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK gboolean
g_rw_lock_reader_trylock (GRWLock *rw_lock)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
  return FALSE;
}

G_GNUC_WEAK void
g_rw_lock_reader_unlock (GRWLock *rw_lock)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

/* {{{1 GCond */

G_GNUC_WEAK void
g_cond_init (GCond *cond)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
g_cond_clear (GCond *cond)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
g_cond_wait (GCond  *cond,
             GMutex *mutex)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
g_cond_signal (GCond *cond)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
g_cond_broadcast (GCond *cond)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK gboolean
g_cond_wait_until (GCond  *cond,
                   GMutex *mutex,
                   gint64  end_time)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
  return FALSE;
}

/* {{{1 GPrivate */

G_GNUC_WEAK gpointer
g_private_get (GPrivate *key)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
  return NULL;
}

G_GNUC_WEAK void
g_private_set (GPrivate *key,
               gpointer  value)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
g_private_replace (GPrivate *key,
                   gpointer  value)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

/* {{{1 GThread */

typedef struct
{
  GRealThread thread;

  GSystemThread *system_thread;
  gboolean       joined;
  GMutex         lock;
} GThreadExternal;

void
g_system_thread_free (GRealThread *thread)
{
  GThreadExternal *pt = (GThreadExternal *) thread;

  if (!pt->joined)
    _g_system_thread_detach (pt->system_thread);

  g_mutex_clear (&pt->lock);

  g_slice_free (GThreadExternal, pt);
}

gboolean
g_system_thread_get_scheduler_settings (GThreadSchedulerSettings *scheduler_settings)
{
  return FALSE;
}

GRealThread *
g_system_thread_new (GThreadFunc proxy,
                     gulong stack_size,
                     const GThreadSchedulerSettings *scheduler_settings,
                     const char *name,
                     GThreadFunc func,
                     gpointer data,
                     GError **error)
{
  GThreadExternal *thread;
  GRealThread *base_thread;

  thread = g_slice_new0 (GThreadExternal);
  base_thread = (GRealThread*)thread;
  base_thread->ref_count = 2;
  base_thread->ours = TRUE;
  base_thread->thread.joinable = TRUE;
  base_thread->thread.func = func;
  base_thread->thread.data = data;
  base_thread->name = g_strdup (name);
  base_thread->pending_garbage = g_hash_table_new (NULL, NULL);

  thread->system_thread =
      _g_system_thread_create (stack_size, name, proxy, thread);

  if (thread->system_thread == NULL)
    {
      g_set_error (error, G_THREAD_ERROR, G_THREAD_ERROR_AGAIN,
                   "Error creating thread");
      g_hash_table_unref (thread->thread.pending_garbage);
      g_free (thread->thread.name);
      g_slice_free (GThreadExternal, thread);
      return NULL;
    }

  g_mutex_init (&thread->lock);

  return (GRealThread *) thread;
}

G_GNUC_WEAK void
g_thread_yield (void)
{
}

void
g_system_thread_wait (GRealThread *thread)
{
  GThreadExternal *pt = (GThreadExternal *) thread;

  g_mutex_lock (&pt->lock);

  if (!pt->joined)
    {
      _g_system_thread_wait (pt->system_thread);
      pt->joined = TRUE;
    }

  g_mutex_unlock (&pt->lock);
}

void
g_system_thread_exit (void)
{
  _g_system_thread_exit ();
}

void
g_system_thread_set_name (const gchar *name)
{
  _g_system_thread_set_name (name);
}

G_GNUC_WEAK GSystemThread *
_g_system_thread_create (gulong       stack_size,
                         const char  *name,
                         GThreadFunc  func,
                         gpointer     data)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
  return NULL;
}

G_GNUC_WEAK void
_g_system_thread_detach (GSystemThread *thread)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
_g_system_thread_wait (GSystemThread *thread)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
_g_system_thread_exit (void)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK void
_g_system_thread_set_name (const gchar *name)
{
}

G_GNUC_WEAK GThreadBeacon *
g_thread_lifetime_beacon_new (void)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
  return NULL;
}

G_GNUC_WEAK void
g_thread_lifetime_beacon_free (GThreadBeacon *beacon)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
}

G_GNUC_WEAK gboolean
g_thread_lifetime_beacon_check (GThreadBeacon *beacon)
{
  G_PANIC_MISSING_IMPLEMENTATION ();
  return FALSE;
}

G_GNUC_WEAK void
_g_thread_init (void)
{
}

G_GNUC_WEAK void
_g_thread_deinit (void)
{
}

  /* {{{1 Epilogue */
/* vim:set foldmethod=marker: */
