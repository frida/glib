/* gstdio.h - GFilename wrappers for C library functions
 *
 * Copyright 2004 Tor Lillqvist
 *
 * GLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * GLib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GLib; see the file COPYING.LIB.  If not,
 * see <http://www.gnu.org/licenses/>.
 */

#ifndef __G_STDIO_H__
#define __G_STDIO_H__

#include <glib/gprintf.h>

#include <sys/stat.h>

G_BEGIN_DECLS

#ifdef G_OS_WIN32
/* We want 64-bit file size support */
typedef struct _stati64 GStatBuf;
#else
typedef struct stat GStatBuf;
#endif

#if defined(G_OS_UNIX) && !defined(G_STDIO_NO_WRAP_ON_UNIX)

/* Just pass on to the system functions, so there's no potential for data
 * format mismatches, especially with large file interfaces. 
 * A few functions can't be handled in this way, since they are not defined
 * in a portable system header that we could include here.
 */

#ifndef __GTK_DOC_IGNORE__
#define g_chmod   chmod
#define g_open    open
#define g_creat   creat
#define g_rename  rename
#define g_mkdir   mkdir
#define g_stat    stat
#define g_lstat   lstat
#define g_remove  remove
#define g_fopen   fopen
#define g_freopen freopen
#define g_utime   utime
#endif

GLIB_AVAILABLE_IN_ALL
int g_access (const gchar *filename,
	      int          mode);

GLIB_AVAILABLE_IN_ALL
int g_chdir  (const gchar *path);

GLIB_AVAILABLE_IN_ALL
int g_unlink (const gchar *filename);

GLIB_AVAILABLE_IN_ALL
int g_rmdir  (const gchar *filename);

#else /* ! G_OS_UNIX */

/* Wrappers for C library functions that take pathname arguments. On
 * Unix, the pathname is a file name as it literally is in the file
 * system. On well-maintained systems with consistent users who know
 * what they are doing and no exchange of files with others this would
 * be a well-defined encoding, preferably UTF-8. On Windows, the
 * pathname is always in UTF-8, even if that is not the on-disk
 * encoding, and not the encoding accepted by the C library or Win32
 * API.
 */

GLIB_AVAILABLE_IN_ALL
int g_access    (const gchar *filename,
		 int          mode);

GLIB_AVAILABLE_IN_ALL
int g_chmod     (const gchar *filename,
		 int          mode);

GLIB_AVAILABLE_IN_ALL
int g_open      (const gchar *filename,
                 int          flags,
                 int          mode);

GLIB_AVAILABLE_IN_ALL
int g_creat     (const gchar *filename,
                 int          mode);

GLIB_AVAILABLE_IN_ALL
int g_rename    (const gchar *oldfilename,
                 const gchar *newfilename);

GLIB_AVAILABLE_IN_ALL
int g_mkdir     (const gchar *filename,
                 int          mode);

GLIB_AVAILABLE_IN_ALL
int g_chdir     (const gchar *path);

GLIB_AVAILABLE_IN_ALL
int g_stat      (const gchar *filename,
                 GStatBuf    *buf);

GLIB_AVAILABLE_IN_ALL
int g_lstat     (const gchar *filename,
                 GStatBuf    *buf);

GLIB_AVAILABLE_IN_ALL
int g_unlink    (const gchar *filename);

GLIB_AVAILABLE_IN_ALL
int g_remove    (const gchar *filename);

GLIB_AVAILABLE_IN_ALL
int g_rmdir     (const gchar *filename);

GLIB_AVAILABLE_IN_ALL
FILE *g_fopen   (const gchar *filename,
                 const gchar *mode);

GLIB_AVAILABLE_IN_ALL
FILE *g_freopen (const gchar *filename,
                 const gchar *mode,
                 FILE        *stream);

struct utimbuf;			/* Don't need the real definition of struct utimbuf when just
				 * including this header.
				 */

GLIB_AVAILABLE_IN_ALL
int g_utime     (const gchar    *filename,
		 struct utimbuf *utb);

#endif /* G_OS_UNIX */

GLIB_AVAILABLE_IN_2_36
gboolean g_close (gint       fd,
                  GError   **error);

G_END_DECLS

#endif /* __G_STDIO_H__ */
