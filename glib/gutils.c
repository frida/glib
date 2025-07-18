/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1998  Peter Mattis, Spencer Kimball and Josh MacDonald
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

/* 
 * MT safe for the unix part, FIXME: make the win32 part MT safe as well.
 */

#include "config.h"

#include "gutils.h"
#include "gutilsprivate.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <ctype.h>		/* For tolower() */
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef G_OS_UNIX
#include <pwd.h>
#include <sys/utsname.h>
#include <unistd.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_CRT_EXTERNS_H 
#include <crt_externs.h> /* for _NSGetEnviron */
#endif
#ifdef HAVE_SYS_AUXV_H
#include <sys/auxv.h>
#endif

#include "glib-init.h"
#include "glib-private.h"
#include "genviron.h"
#include "gfileutils.h"
#include "ggettext.h"
#include "ghash.h"
#include "gthread.h"
#include "gtestutils.h"
#include "gunicode.h"
#include "gstrfuncs.h"
#include "garray.h"
#include "glibintl.h"
#include "gstdio.h"
#include "gquark.h"

#ifdef G_PLATFORM_WIN32
#include "gconvert.h"
#include "gwin32.h"
#endif


/**
 * SECTION:misc_utils
 * @title: Miscellaneous Utility Functions
 * @short_description: a selection of portable utility functions
 *
 * These are portable utility functions.
 */

#ifdef G_PLATFORM_WIN32
#  include <windows.h>
#  ifndef GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
#    define GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT 2
#    define GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS 4
#  endif
#  include <lmcons.h>		/* For UNLEN */
#endif /* G_PLATFORM_WIN32 */

#ifdef G_OS_WIN32
#  include <direct.h>
#  include <shlobj.h>
#  include <process.h>
#endif

#ifdef HAVE_CODESET
#include <langinfo.h>
#endif

/**
 * g_memmove: 
 * @dest: the destination address to copy the bytes to.
 * @src: the source address to copy the bytes from.
 * @len: the number of bytes to copy.
 *
 * Copies a block of memory @len bytes long, from @src to @dest.
 * The source and destination areas may overlap.
 *
 * Deprecated:2.40: Just use memmove().
 */

#ifdef G_OS_WIN32
#undef g_atexit
#endif

/**
 * g_atexit:
 * @func: (scope async): the function to call on normal program termination.
 * 
 * Specifies a function to be called at normal program termination.
 *
 * Since GLib 2.8.2, on Windows g_atexit() actually is a preprocessor
 * macro that maps to a call to the atexit() function in the C
 * library. This means that in case the code that calls g_atexit(),
 * i.e. atexit(), is in a DLL, the function will be called when the
 * DLL is detached from the program. This typically makes more sense
 * than that the function is called when the GLib DLL is detached,
 * which happened earlier when g_atexit() was a function in the GLib
 * DLL.
 *
 * The behaviour of atexit() in the context of dynamically loaded
 * modules is not formally specified and varies wildly.
 *
 * On POSIX systems, calling g_atexit() (or atexit()) in a dynamically
 * loaded module which is unloaded before the program terminates might
 * well cause a crash at program exit.
 *
 * Some POSIX systems implement atexit() like Windows, and have each
 * dynamically loaded module maintain an own atexit chain that is
 * called when the module is unloaded.
 *
 * On other POSIX systems, before a dynamically loaded module is
 * unloaded, the registered atexit functions (if any) residing in that
 * module are called, regardless where the code that registered them
 * resided. This is presumably the most robust approach.
 *
 * As can be seen from the above, for portability it's best to avoid
 * calling g_atexit() (or atexit()) except in the main executable of a
 * program.
 *
 * Deprecated:2.32: It is best to avoid g_atexit().
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
void
g_atexit (GVoidFunc func)
{
  gint result;
  int errsv;

  result = atexit ((void (*)(void)) func);
  errsv = errno;
  if (result)
    {
      g_error ("Could not register atexit() function: %s",
               g_strerror (errsv));
    }
}
G_GNUC_END_IGNORE_DEPRECATIONS

/* Based on execvp() from GNU Libc.
 * Some of this code is cut-and-pasted into gspawn.c
 */

static gchar*
my_strchrnul (const gchar *str, 
	      gchar        c)
{
  gchar *p = (gchar*)str;
  while (*p && (*p != c))
    ++p;

  return p;
}

#ifdef G_OS_WIN32

static gchar *inner_find_program_in_path (const gchar *program);

gchar*
g_find_program_in_path (const gchar *program)
{
  const gchar *last_dot = strrchr (program, '.');

  if (last_dot == NULL ||
      strchr (last_dot, '\\') != NULL ||
      strchr (last_dot, '/') != NULL)
    {
      const gint program_length = strlen (program);
      gchar *pathext = g_build_path (";",
				     ".exe;.cmd;.bat;.com",
				     g_getenv ("PATHEXT"),
				     NULL);
      gchar *p;
      gchar *decorated_program;
      gchar *retval;

      p = pathext;
      do
	{
	  gchar *q = my_strchrnul (p, ';');

	  decorated_program = g_malloc (program_length + (q-p) + 1);
	  memcpy (decorated_program, program, program_length);
	  memcpy (decorated_program+program_length, p, q-p);
	  decorated_program [program_length + (q-p)] = '\0';
	  
	  retval = inner_find_program_in_path (decorated_program);
	  g_free (decorated_program);

	  if (retval != NULL)
	    {
	      g_free (pathext);
	      return retval;
	    }
	  p = q;
	} while (*p++ != '\0');
      g_free (pathext);
      return NULL;
    }
  else
    return inner_find_program_in_path (program);
}

#endif

/**
 * g_find_program_in_path:
 * @program: (type filename): a program name in the GLib file name encoding
 * 
 * Locates the first executable named @program in the user's path, in the
 * same way that execvp() would locate it. Returns an allocated string
 * with the absolute path name, or %NULL if the program is not found in
 * the path. If @program is already an absolute path, returns a copy of
 * @program if @program exists and is executable, and %NULL otherwise.
 *  
 * On Windows, if @program does not have a file type suffix, tries
 * with the suffixes .exe, .cmd, .bat and .com, and the suffixes in
 * the `PATHEXT` environment variable. 
 * 
 * On Windows, it looks for the file in the same way as CreateProcess() 
 * would. This means first in the directory where the executing
 * program was loaded from, then in the current directory, then in the
 * Windows 32-bit system directory, then in the Windows directory, and
 * finally in the directories in the `PATH` environment variable. If
 * the program is found, the return value contains the full name
 * including the type suffix.
 *
 * Returns: (type filename) (transfer full) (nullable): a newly-allocated
 *   string with the absolute path, or %NULL
 **/
#ifdef G_OS_WIN32
static gchar *
inner_find_program_in_path (const gchar *program)
#else
gchar*
g_find_program_in_path (const gchar *program)
#endif
{
#ifdef G_OS_NONE
  return NULL;
#else
  const gchar *path, *p;
  gchar *name, *freeme;
#ifdef G_OS_WIN32
  const gchar *path_copy;
  gchar *filename = NULL, *appdir = NULL;
  gchar *sysdir = NULL, *windir = NULL;
  int n;
  wchar_t wfilename[MAXPATHLEN], wsysdir[MAXPATHLEN],
    wwindir[MAXPATHLEN];
#endif
  gsize len;
  gsize pathlen;

  g_return_val_if_fail (program != NULL, NULL);

  /* If it is an absolute path, or a relative path including subdirectories,
   * don't look in PATH.
   */
  if (g_path_is_absolute (program)
      || strchr (program, G_DIR_SEPARATOR) != NULL
#ifdef G_OS_WIN32
      || strchr (program, '/') != NULL
#endif
      )
    {
      if (g_file_test (program, G_FILE_TEST_IS_EXECUTABLE) &&
	  !g_file_test (program, G_FILE_TEST_IS_DIR))
        {
          gchar *out = NULL, *cwd = NULL;

          if (g_path_is_absolute (program))
            return g_strdup (program);

          cwd = g_get_current_dir ();
          out = g_build_filename (cwd, program, NULL);
          g_free (cwd);
          return g_steal_pointer (&out);
        }
      else
        return NULL;
    }
  
  path = g_getenv ("PATH");
#if defined(G_OS_UNIX)
  if (path == NULL)
    {
      /* There is no 'PATH' in the environment.  The default
       * search path in GNU libc is the current directory followed by
       * the path 'confstr' returns for '_CS_PATH'.
       */
      
      /* In GLib we put . last, for security, and don't use the
       * unportable confstr(); UNIX98 does not actually specify
       * what to search if PATH is unset. POSIX may, dunno.
       */
      
      path = "/bin:/usr/bin:.";
    }
#else
  n = GetModuleFileNameW (NULL, wfilename, MAXPATHLEN);
  if (n > 0 && n < MAXPATHLEN)
    filename = g_utf16_to_utf8 (wfilename, -1, NULL, NULL, NULL);
  
  n = GetSystemDirectoryW (wsysdir, MAXPATHLEN);
  if (n > 0 && n < MAXPATHLEN)
    sysdir = g_utf16_to_utf8 (wsysdir, -1, NULL, NULL, NULL);
  
  n = GetWindowsDirectoryW (wwindir, MAXPATHLEN);
  if (n > 0 && n < MAXPATHLEN)
    windir = g_utf16_to_utf8 (wwindir, -1, NULL, NULL, NULL);
  
  if (filename)
    {
      appdir = g_path_get_dirname (filename);
      g_free (filename);
    }
  
  path = g_strdup (path);

  if (windir)
    {
      const gchar *tem = path;
      path = g_strconcat (windir, ";", path, NULL);
      g_free ((gchar *) tem);
      g_free (windir);
    }
  
  if (sysdir)
    {
      const gchar *tem = path;
      path = g_strconcat (sysdir, ";", path, NULL);
      g_free ((gchar *) tem);
      g_free (sysdir);
    }
  
  {
    const gchar *tem = path;
    path = g_strconcat (".;", path, NULL);
    g_free ((gchar *) tem);
  }
  
  if (appdir)
    {
      const gchar *tem = path;
      path = g_strconcat (appdir, ";", path, NULL);
      g_free ((gchar *) tem);
      g_free (appdir);
    }

  path_copy = path;
#endif
  
  len = strlen (program) + 1;
  pathlen = strlen (path);
  freeme = name = g_malloc (pathlen + len + 1);
  
  /* Copy the file name at the top, including '\0'  */
  memcpy (name + pathlen + 1, program, len);
  name = name + pathlen;
  /* And add the slash before the filename  */
  *name = G_DIR_SEPARATOR;
  
  p = path;
  do
    {
      char *startp;

      path = p;
      p = my_strchrnul (path, G_SEARCHPATH_SEPARATOR);

      if (p == path)
        /* Two adjacent colons, or a colon at the beginning or the end
         * of 'PATH' means to search the current directory.
         */
        startp = name + 1;
      else
        startp = memcpy (name - (p - path), path, p - path);

      if (g_file_test (startp, G_FILE_TEST_IS_EXECUTABLE) &&
	  !g_file_test (startp, G_FILE_TEST_IS_DIR))
        {
          gchar *ret;
          if (g_path_is_absolute (startp)) {
            ret = g_strdup (startp);
          } else {
            gchar *cwd = NULL;
            cwd = g_get_current_dir ();
            ret = g_build_filename (cwd, startp, NULL);
            g_free (cwd);
          }
          g_free (freeme);
#ifdef G_OS_WIN32
	  g_free ((gchar *) path_copy);
#endif
          return ret;
        }
    }
  while (*p++ != '\0');
  
  g_free (freeme);
#ifdef G_OS_WIN32
  g_free ((gchar *) path_copy);
#endif

  return NULL;
#endif
}

/* The functions below are defined this way for compatibility reasons.
 * See the note in gutils.h.
 */

/**
 * g_bit_nth_lsf:
 * @mask: a #gulong containing flags
 * @nth_bit: the index of the bit to start the search from
 *
 * Find the position of the first bit set in @mask, searching
 * from (but not including) @nth_bit upwards. Bits are numbered
 * from 0 (least significant) to sizeof(#gulong) * 8 - 1 (31 or 63,
 * usually). To start searching from the 0th bit, set @nth_bit to -1.
 *
 * Returns: the index of the first bit set which is higher than @nth_bit, or -1
 *    if no higher bits are set
 */
gint
(g_bit_nth_lsf) (gulong mask,
                 gint   nth_bit)
{
  return g_bit_nth_lsf_impl (mask, nth_bit);
}

/**
 * g_bit_nth_msf:
 * @mask: a #gulong containing flags
 * @nth_bit: the index of the bit to start the search from
 *
 * Find the position of the first bit set in @mask, searching
 * from (but not including) @nth_bit downwards. Bits are numbered
 * from 0 (least significant) to sizeof(#gulong) * 8 - 1 (31 or 63,
 * usually). To start searching from the last bit, set @nth_bit to
 * -1 or GLIB_SIZEOF_LONG * 8.
 *
 * Returns: the index of the first bit set which is lower than @nth_bit, or -1
 *    if no lower bits are set
 */
gint
(g_bit_nth_msf) (gulong mask,
                 gint   nth_bit)
{
  return g_bit_nth_msf_impl (mask, nth_bit);
}


/**
 * g_bit_storage:
 * @number: a #guint
 *
 * Gets the number of bits used to hold @number,
 * e.g. if @number is 4, 3 bits are needed.
 *
 * Returns: the number of bits used to hold @number
 */
guint
(g_bit_storage) (gulong number)
{
  return g_bit_storage_impl (number);
}

G_LOCK_DEFINE_STATIC (g_utils_global);

typedef struct
{
  gchar *user_name;
  gchar *real_name;
  gchar *home_dir;
} UserDatabaseEntry;

/* These must all be read/written with @g_utils_global held. */
static  gchar   *g_user_data_dir = NULL;
static  gchar  **g_system_data_dirs = NULL;
static  gchar   *g_user_cache_dir = NULL;
static  gchar   *g_user_config_dir = NULL;
static  gchar   *g_user_state_dir = NULL;
static  gchar   *g_user_runtime_dir = NULL;
static  gchar  **g_system_config_dirs = NULL;
static  gchar  **g_user_special_dirs = NULL;

/* fifteen minutes of fame for everybody */
#define G_USER_DIRS_EXPIRE      15 * 60

#ifdef G_OS_WIN32

static gchar *
get_special_folder (int csidl)
{
  wchar_t path[MAX_PATH+1];
  HRESULT hr;
  LPITEMIDLIST pidl = NULL;
  BOOL b;
  gchar *retval = NULL;

  hr = SHGetSpecialFolderLocation (NULL, csidl, &pidl);
  if (hr == S_OK)
    {
      b = SHGetPathFromIDListW (pidl, path);
      if (b)
	retval = g_utf16_to_utf8 (path, -1, NULL, NULL, NULL);
      CoTaskMemFree (pidl);
    }
  return retval;
}

static char *
get_windows_directory_root (void)
{
  wchar_t wwindowsdir[MAX_PATH];

  if (GetWindowsDirectoryW (wwindowsdir, G_N_ELEMENTS (wwindowsdir)))
    {
      /* Usually X:\Windows, but in terminal server environments
       * might be an UNC path, AFAIK.
       */
      char *windowsdir = g_utf16_to_utf8 (wwindowsdir, -1, NULL, NULL, NULL);
      char *p;

      if (windowsdir == NULL)
	return g_strdup ("C:\\");

      p = (char *) g_path_skip_root (windowsdir);
      if (G_IS_DIR_SEPARATOR (p[-1]) && p[-2] != ':')
	p--;
      *p = '\0';
      return windowsdir;
    }
  else
    return g_strdup ("C:\\");
}

#endif

/* HOLDS: g_utils_global_lock */
static UserDatabaseEntry *
g_get_user_database_entry (void)
{
  static UserDatabaseEntry *entry;

  if (g_once_init_enter (&entry))
    {
      static UserDatabaseEntry e;

#ifdef G_OS_UNIX
      {
        struct passwd *pw = NULL;
        gpointer buffer = NULL;
        gint error;
        gchar *logname;

#  if defined (HAVE_GETPWUID_R)
        struct passwd pwd;
#    ifdef _SC_GETPW_R_SIZE_MAX
        /* This reurns the maximum length */
        glong bufsize = sysconf (_SC_GETPW_R_SIZE_MAX);

        if (bufsize < 0)
          bufsize = 64;
#    else /* _SC_GETPW_R_SIZE_MAX */
        glong bufsize = 64;
#    endif /* _SC_GETPW_R_SIZE_MAX */

        logname = (gchar *) g_getenv ("LOGNAME");

        do
          {
            g_free (buffer);
            /* we allocate 6 extra bytes to work around a bug in
             * Mac OS < 10.3. See #156446
             */
            buffer = g_malloc (bufsize + 6);
            errno = 0;

            if (logname) {
              error = getpwnam_r (logname, &pwd, buffer, bufsize, &pw);
              if (!pw || (pw->pw_uid != getuid ())) {
                /* LOGNAME is lying, fall back to looking up the uid */
                error = getpwuid_r (getuid (), &pwd, buffer, bufsize, &pw);
              }
            } else {
              error = getpwuid_r (getuid (), &pwd, buffer, bufsize, &pw);
            }
            error = error < 0 ? errno : error;

            if (!pw)
              {
                /* we bail out prematurely if the user id can't be found
                 * (should be pretty rare case actually), or if the buffer
                 * should be sufficiently big and lookups are still not
                 * successful.
                 */
                if (error == 0 || error == ENOENT)
                  {
                    g_warning ("getpwuid_r(): failed due to unknown user id (%lu)",
                               (gulong) getuid ());
                    break;
                  }
                if (bufsize > 32 * 1024)
                  {
                    g_warning ("getpwuid_r(): failed due to: %s.",
                               g_strerror (error));
                    break;
                  }

                bufsize *= 2;
              }
          }
        while (!pw);
#  endif /* HAVE_GETPWUID_R */

        if (!pw)
          {
            pw = getpwuid (getuid ());
          }
        if (pw)
          {
            e.user_name = g_strdup (pw->pw_name);

#ifndef __BIONIC__
            if (pw->pw_gecos && *pw->pw_gecos != '\0' && pw->pw_name)
              {
                gchar **gecos_fields;
                gchar **name_parts;
                gchar *uppercase_pw_name;

                /* split the gecos field and substitute '&' */
                gecos_fields = g_strsplit (pw->pw_gecos, ",", 0);
                name_parts = g_strsplit (gecos_fields[0], "&", 0);
                uppercase_pw_name = g_strdup (pw->pw_name);
                uppercase_pw_name[0] = g_ascii_toupper (uppercase_pw_name[0]);
                e.real_name = g_strjoinv (uppercase_pw_name, name_parts);
                g_strfreev (gecos_fields);
                g_strfreev (name_parts);
                g_free (uppercase_pw_name);
              }
#endif

            if (!e.home_dir)
              e.home_dir = g_strdup (pw->pw_dir);
          }
        g_free (buffer);
      }

#endif /* G_OS_UNIX */

#ifdef G_OS_WIN32
      {
        guint len = UNLEN+1;
        wchar_t buffer[UNLEN+1];

        if (GetUserNameW (buffer, (LPDWORD) &len))
          {
            e.user_name = g_utf16_to_utf8 (buffer, -1, NULL, NULL, NULL);
            e.real_name = g_strdup (e.user_name);
          }
      }
#endif /* G_OS_WIN32 */

      if (!e.user_name)
        e.user_name = g_strdup ("somebody");
      if (!e.real_name)
        e.real_name = g_strdup ("Unknown");

      g_once_init_leave (&entry, &e);
    }

  return entry;
}

/**
 * g_get_user_name:
 *
 * Gets the user name of the current user. The encoding of the returned
 * string is system-defined. On UNIX, it might be the preferred file name
 * encoding, or something else, and there is no guarantee that it is even
 * consistent on a machine. On Windows, it is always UTF-8.
 *
 * Returns: (type filename) (transfer none): the user name of the current user.
 */
const gchar *
g_get_user_name (void)
{
  UserDatabaseEntry *entry;

  entry = g_get_user_database_entry ();

  return entry->user_name;
}

/**
 * g_get_real_name:
 *
 * Gets the real name of the user. This usually comes from the user's
 * entry in the `passwd` file. The encoding of the returned string is
 * system-defined. (On Windows, it is, however, always UTF-8.) If the
 * real user name cannot be determined, the string "Unknown" is 
 * returned.
 *
 * Returns: (type filename) (transfer none): the user's real name.
 */
const gchar *
g_get_real_name (void)
{
  UserDatabaseEntry *entry;

  entry = g_get_user_database_entry ();

  return entry->real_name;
}

/* Protected by @g_utils_global_lock. */
static gchar *g_home_dir = NULL;  /* (owned) (nullable before initialised) */

static gchar *
g_build_home_dir (void)
{
  gchar *home_dir;

  /* We first check HOME and use it if it is set */
  home_dir = g_strdup (g_getenv ("HOME"));

#ifdef G_OS_WIN32
  /* Only believe HOME if it is an absolute path and exists.
   *
   * We only do this check on Windows for a couple of reasons.
   * Historically, we only did it there because we used to ignore $HOME
   * on UNIX.  There are concerns about enabling it now on UNIX because
   * of things like autofs.  In short, if the user has a bogus value in
   * $HOME then they get what they pay for...
   */
  if (home_dir != NULL)
    {
      if (!(g_path_is_absolute (home_dir) &&
            g_file_test (home_dir, G_FILE_TEST_IS_DIR)))
        g_clear_pointer (&home_dir, g_free);
    }

  /* In case HOME is Unix-style (it happens), convert it to
   * Windows style.
   */
  if (home_dir != NULL)
    {
      gchar *p;
      while ((p = strchr (home_dir, '/')) != NULL)
        *p = '\\';
    }

  if (home_dir == NULL)
    {
      /* USERPROFILE is probably the closest equivalent to $HOME? */
      if (g_getenv ("USERPROFILE") != NULL)
        home_dir = g_strdup (g_getenv ("USERPROFILE"));
    }

  if (home_dir == NULL)
    home_dir = get_special_folder (CSIDL_PROFILE);

  if (home_dir == NULL)
    home_dir = get_windows_directory_root ();
#endif /* G_OS_WIN32 */

  if (home_dir == NULL)
    {
      /* If we didn't get it from any of those methods, we will have
       * to read the user database entry.
       */
      UserDatabaseEntry *entry = g_get_user_database_entry ();
      home_dir = g_strdup (entry->home_dir);
    }

  /* If we have been denied access to /etc/passwd (for example, by an
   * overly-zealous LSM), make up a junk value. The return value at this
   * point is explicitly documented as ‘undefined’. */
  if (home_dir == NULL)
    {
      g_warning ("Could not find home directory: $HOME is not set, and "
                 "user database could not be read.");
      home_dir = g_strdup ("/");
    }

  return g_steal_pointer (&home_dir);
}

/**
 * g_get_home_dir:
 *
 * Gets the current user's home directory.
 *
 * As with most UNIX tools, this function will return the value of the
 * `HOME` environment variable if it is set to an existing absolute path
 * name, falling back to the `passwd` file in the case that it is unset.
 *
 * If the path given in `HOME` is non-absolute, does not exist, or is
 * not a directory, the result is undefined.
 *
 * Before version 2.36 this function would ignore the `HOME` environment
 * variable, taking the value from the `passwd` database instead. This was
 * changed to increase the compatibility of GLib with other programs (and
 * the XDG basedir specification) and to increase testability of programs
 * based on GLib (by making it easier to run them from test frameworks).
 *
 * If your program has a strong requirement for either the new or the
 * old behaviour (and if you don't wish to increase your GLib
 * dependency to ensure that the new behaviour is in effect) then you
 * should either directly check the `HOME` environment variable yourself
 * or unset it before calling any functions in GLib.
 *
 * Returns: (type filename) (transfer none): the current user's home directory
 */
const gchar *
g_get_home_dir (void)
{
  const gchar *home_dir;

  G_LOCK (g_utils_global);

  if (g_home_dir == NULL)
    g_home_dir = g_build_home_dir ();
  home_dir = g_home_dir;

  G_UNLOCK (g_utils_global);

  return home_dir;
}

/**
 * g_get_tmp_dir:
 *
 * Gets the directory to use for temporary files.
 *
 * On UNIX, this is taken from the `TMPDIR` environment variable.
 * If the variable is not set, `P_tmpdir` is
 * used, as defined by the system C library. Failing that, a
 * hard-coded default of "/tmp" is returned.
 *
 * On Windows, the `TEMP` environment variable is used, with the
 * root directory of the Windows installation (eg: "C:\") used
 * as a default.
 *
 * The encoding of the returned string is system-defined. On Windows,
 * it is always UTF-8. The return value is never %NULL or the empty
 * string.
 *
 * Returns: (type filename) (transfer none): the directory to use for temporary files.
 */
const gchar *
g_get_tmp_dir (void)
{
  static gchar *tmp_dir;

  if (g_once_init_enter (&tmp_dir))
    {
      gchar *tmp;

#ifdef G_OS_WIN32
      tmp = g_strdup (g_getenv ("TEMP"));

      if (tmp == NULL || *tmp == '\0')
        {
          g_free (tmp);
          tmp = get_windows_directory_root ();
        }
#else /* G_OS_WIN32 */
      tmp = g_strdup (g_getenv ("TMPDIR"));

#ifdef P_tmpdir
      if (tmp == NULL || *tmp == '\0')
        {
          gsize k;
          g_free (tmp);
          tmp = g_strdup (P_tmpdir);
          k = strlen (tmp);
          if (k > 1 && G_IS_DIR_SEPARATOR (tmp[k - 1]))
            tmp[k - 1] = '\0';
        }
#endif /* P_tmpdir */

      if (tmp == NULL || *tmp == '\0')
        {
          g_free (tmp);
          tmp = g_strdup ("/tmp");
        }
#endif /* !G_OS_WIN32 */

      g_once_init_leave (&tmp_dir, tmp);
    }

  return tmp_dir;
}

/**
 * g_get_host_name:
 *
 * Return a name for the machine. 
 *
 * The returned name is not necessarily a fully-qualified domain name,
 * or even present in DNS or some other name service at all. It need
 * not even be unique on your local network or site, but usually it
 * is. Callers should not rely on the return value having any specific
 * properties like uniqueness for security purposes. Even if the name
 * of the machine is changed while an application is running, the
 * return value from this function does not change. The returned
 * string is owned by GLib and should not be modified or freed. If no
 * name can be determined, a default fixed string "localhost" is
 * returned.
 *
 * The encoding of the returned string is UTF-8.
 *
 * Returns: (transfer none): the host name of the machine.
 *
 * Since: 2.8
 */
const gchar *
g_get_host_name (void)
{
  static gchar *hostname;

  if (g_once_init_enter (&hostname))
    {
      gboolean failed;
      gchar *utmp = NULL;

#ifndef G_OS_WIN32
      gsize size;
      /* The number 256 * 256 is taken from the value of _POSIX_HOST_NAME_MAX,
       * which is 255. Since we use _POSIX_HOST_NAME_MAX + 1 (= 256) in the
       * fallback case, we pick 256 * 256 as the size of the larger buffer here.
       * It should be large enough. It doesn't looks reasonable to name a host
       * with a string that is longer than 64 KiB.
       */
      const gsize size_large = (gsize) 256 * 256;
      gchar *tmp;

#ifdef _SC_HOST_NAME_MAX
      {
        glong max;

        max = sysconf (_SC_HOST_NAME_MAX);
        if (max > 0 && (gsize) max <= G_MAXSIZE - 1)
          size = (gsize) max + 1;
        else
#ifdef HOST_NAME_MAX
          size = HOST_NAME_MAX + 1;
#elif defined(_POSIX_HOST_NAME_MAX)
          size = _POSIX_HOST_NAME_MAX + 1;
#else
          size = 256;
#endif /* HOST_NAME_MAX */
      }
#else
      /* Fallback to some reasonable value */
      size = 256;
#endif /* _SC_HOST_NAME_MAX */
      tmp = g_malloc (size);
      failed = (gethostname (tmp, size) == -1);
      if (failed && size < size_large)
        {
          /* Try again with a larger buffer if 'size' may be too small. */
          g_free (tmp);
          tmp = g_malloc (size_large);
          failed = (gethostname (tmp, size_large) == -1);
        }

      if (failed)
        g_clear_pointer (&tmp, g_free);
      utmp = tmp;
#else
      wchar_t tmp[MAX_COMPUTERNAME_LENGTH + 1];
      DWORD size = sizeof (tmp) / sizeof (tmp[0]);
      failed = (!GetComputerNameW (tmp, &size));
      if (!failed)
        utmp = g_utf16_to_utf8 (tmp, size, NULL, NULL, NULL);
      if (utmp == NULL)
        failed = TRUE;
#endif

      g_once_init_leave (&hostname, failed ? g_strdup ("localhost") : utmp);
    }

  return hostname;
}

G_LOCK_DEFINE_STATIC (g_prgname);
static const gchar *g_prgname = NULL; /* always a quark */

/**
 * g_get_prgname:
 *
 * Gets the name of the program. This name should not be localized,
 * in contrast to g_get_application_name().
 *
 * If you are using #GApplication the program name is set in
 * g_application_run(). In case of GDK or GTK+ it is set in
 * gdk_init(), which is called by gtk_init() and the
 * #GtkApplication::startup handler. The program name is found by
 * taking the last component of @argv[0].
 *
 * Returns: (nullable) (transfer none): the name of the program,
 *   or %NULL if it has not been set yet. The returned string belongs
 *   to GLib and must not be modified or freed.
 */
const gchar*
g_get_prgname (void)
{
  const gchar* retval;

  G_LOCK (g_prgname);
  retval = g_prgname;
  G_UNLOCK (g_prgname);

  return retval;
}

/**
 * g_set_prgname:
 * @prgname: the name of the program.
 *
 * Sets the name of the program. This name should not be localized,
 * in contrast to g_set_application_name().
 *
 * If you are using #GApplication the program name is set in
 * g_application_run(). In case of GDK or GTK+ it is set in
 * gdk_init(), which is called by gtk_init() and the
 * #GtkApplication::startup handler. The program name is found by
 * taking the last component of @argv[0].
 *
 * Since GLib 2.72, this function can be called multiple times
 * and is fully thread safe. Prior to GLib 2.72, this function
 * could only be called once per process.
 */
void
g_set_prgname (const gchar *prgname)
{
  GQuark qprgname = g_quark_from_string (prgname);
  G_LOCK (g_prgname);
  g_prgname = g_quark_to_string (qprgname);
  G_UNLOCK (g_prgname);
}

G_LOCK_DEFINE_STATIC (g_application_name);
static gchar *g_application_name = NULL;

/**
 * g_get_application_name:
 * 
 * Gets a human-readable name for the application, as set by
 * g_set_application_name(). This name should be localized if
 * possible, and is intended for display to the user.  Contrast with
 * g_get_prgname(), which gets a non-localized name. If
 * g_set_application_name() has not been called, returns the result of
 * g_get_prgname() (which may be %NULL if g_set_prgname() has also not
 * been called).
 * 
 * Returns: (transfer none) (nullable): human-readable application
 *   name. May return %NULL
 *
 * Since: 2.2
 **/
const gchar *
g_get_application_name (void)
{
  gchar* retval;

  G_LOCK (g_application_name);
  retval = g_application_name;
  G_UNLOCK (g_application_name);

  if (retval == NULL)
    return g_get_prgname ();
  
  return retval;
}

/**
 * g_set_application_name:
 * @application_name: localized name of the application
 *
 * Sets a human-readable name for the application. This name should be
 * localized if possible, and is intended for display to the user.
 * Contrast with g_set_prgname(), which sets a non-localized name.
 * g_set_prgname() will be called automatically by gtk_init(),
 * but g_set_application_name() will not.
 *
 * Note that for thread safety reasons, this function can only
 * be called once.
 *
 * The application name will be used in contexts such as error messages,
 * or when displaying an application's name in the task list.
 * 
 * Since: 2.2
 **/
void
g_set_application_name (const gchar *application_name)
{
  gboolean already_set = FALSE;
	
  G_LOCK (g_application_name);
  if (g_application_name)
    already_set = TRUE;
  else
    g_application_name = g_strdup (application_name);
  G_UNLOCK (g_application_name);

  if (already_set)
    g_warning ("g_set_application_name() called multiple times");
}

#ifdef G_OS_WIN32
/* For the past versions we can just
 * hardcode all the names.
 */
static const struct winver
{
  gint major;
  gint minor;
  gint sp;
  const char *version;
  const char *spversion;
} versions[] =
{
  {6, 2, 0, "8", ""},
  {6, 1, 1, "7", " SP1"},
  {6, 1, 0, "7", ""},
  {6, 0, 2, "Vista", " SP2"},
  {6, 0, 1, "Vista", " SP1"},
  {6, 0, 0, "Vista", ""},
  {5, 1, 3, "XP", " SP3"},
  {5, 1, 2, "XP", " SP2"},
  {5, 1, 1, "XP", " SP1"},
  {5, 1, 0, "XP", ""},
  {0, 0, 0, NULL, NULL},
};

static gchar *
get_registry_str (HKEY root_key, const wchar_t *path, const wchar_t *value_name)
{
  HKEY key_handle;
  DWORD req_value_data_size;
  DWORD req_value_data_size2;
  LONG status;
  DWORD value_type_w;
  DWORD value_type_w2;
  char *req_value_data;
  gchar *result;

  status = RegOpenKeyExW (root_key, path, 0, KEY_READ, &key_handle);
  if (status != ERROR_SUCCESS)
    return NULL;

  req_value_data_size = 0;
  status = RegQueryValueExW (key_handle,
                             value_name,
                             NULL,
                             &value_type_w,
                             NULL,
                             &req_value_data_size);

  if (status != ERROR_MORE_DATA && status != ERROR_SUCCESS)
    {
      RegCloseKey (key_handle);

      return NULL;
    }

  req_value_data = g_malloc (req_value_data_size);
  req_value_data_size2 = req_value_data_size;

  status = RegQueryValueExW (key_handle,
                             value_name,
                             NULL,
                             &value_type_w2,
                             (gpointer) req_value_data,
                             &req_value_data_size2);

  result = NULL;

  if (status == ERROR_SUCCESS && value_type_w2 == REG_SZ)
    result = g_utf16_to_utf8 ((gunichar2 *) req_value_data,
                              req_value_data_size / sizeof (gunichar2),
                              NULL,
                              NULL,
                              NULL);

  g_free (req_value_data);
  RegCloseKey (key_handle);

  return result;
}

/* Windows 8.1 can be either plain or with Update 1,
 * depending on its build number (9200 or 9600).
 */
static gchar *
get_windows_8_1_update (void)
{
  gchar *current_build;
  gchar *result = NULL;

  current_build = get_registry_str (HKEY_LOCAL_MACHINE,
                                    L"SOFTWARE"
                                    L"\\Microsoft"
                                    L"\\Windows NT"
                                    L"\\CurrentVersion",
                                    L"CurrentBuild");

  if (current_build != NULL)
    {
      wchar_t *end;
      long build = wcstol ((const wchar_t *) current_build, &end, 10);

      if (build <= INT_MAX &&
          build >= INT_MIN &&
          errno == 0 &&
          *end == L'\0')
        {
          if (build >= 9600)
            result = g_strdup ("Update 1");
        }
    }

  g_clear_pointer (&current_build, g_free);

  return result;
}

static gchar *
get_windows_version (gboolean with_windows)
{
  GString *version = g_string_new (NULL);
  gboolean is_win_server = FALSE;

  if (g_win32_check_windows_version (10, 0, 0, G_WIN32_OS_ANY))
    {
      gchar *win10_release;
      gboolean is_win11 = FALSE;
      OSVERSIONINFOEXW osinfo;

      /* Are we on Windows 2016/2019/2022 Server? */
      is_win_server = g_win32_check_windows_version (10, 0, 0, G_WIN32_OS_SERVER);

      /*
       * This always succeeds if we get here, since the
       * g_win32_check_windows_version() already did this!
       * We want the OSVERSIONINFOEXW here for more even
       * fine-grained versioning items
       */
      _g_win32_call_rtl_version (&osinfo);

      if (!is_win_server)
        {
          /*
           * Windows 11 is actually Windows 10.0.22000+,
           * so look at the build number
           */
          is_win11 = (osinfo.dwBuildNumber >= 22000);
        }
      else
        {
          /*
           * Windows 2022 Server is actually Windows 10.0.20348+,
           * Windows 2019 Server is actually Windows 10.0.17763+,
           * Windows 2016 Server is actually Windows 10.0.14393+,
           * so look at the build number
           */
          g_string_append (version, "Server");
          if (osinfo.dwBuildNumber >= 20348)
            g_string_append (version, " 2022");
          else if (osinfo.dwBuildNumber >= 17763)
            g_string_append (version, " 2019");
          else
            g_string_append (version, " 2016");
        }

      if (is_win11)
        g_string_append (version, "11");
      else if (!is_win_server)
        g_string_append (version, "10");

      /* Windows 10/Server 2016+ is identified by its ReleaseId or
       * DisplayVersion (since 20H2), such as
       * 1511, 1607, 1703, 1709, 1803, 1809 or 1903 etc.
       * The first version of Windows 10 has no release number.
       */
      win10_release = get_registry_str (HKEY_LOCAL_MACHINE,
                                        L"SOFTWARE"
                                        L"\\Microsoft"
                                        L"\\Windows NT"
                                        L"\\CurrentVersion",
                                        L"ReleaseId");

      if (win10_release != NULL)
        {
          if (g_strcmp0 (win10_release, "2009") != 0)
            g_string_append_printf (version, " %s", win10_release);
          else
            {
              g_free (win10_release);

              win10_release = get_registry_str (HKEY_LOCAL_MACHINE,
                                                L"SOFTWARE"
                                                L"\\Microsoft"
                                                L"\\Windows NT"
                                                L"\\CurrentVersion",
                                                L"DisplayVersion");

              if (win10_release != NULL)
                g_string_append_printf (version, " %s", win10_release);
              else
                g_string_append_printf (version, " 2009");
            }
        }

      g_free (win10_release);
    }
  else if (g_win32_check_windows_version (6, 3, 0, G_WIN32_OS_ANY))
    {
      gchar *win81_update;

      if (g_win32_check_windows_version (6, 3, 0, G_WIN32_OS_WORKSTATION))
        g_string_append (version, "8.1");
      else
        g_string_append (version, "Server 2012 R2");

      win81_update = get_windows_8_1_update ();

      if (win81_update != NULL)
        g_string_append_printf (version, " %s", win81_update);

      g_free (win81_update);
    }
  else
    {
      gint i;

      for (i = 0; versions[i].major > 0; i++)
        {
          if (!g_win32_check_windows_version (versions[i].major, versions[i].minor, versions[i].sp, G_WIN32_OS_ANY))
            continue;

          g_string_append (version, versions[i].version);

          if (g_win32_check_windows_version (versions[i].major, versions[i].minor, versions[i].sp, G_WIN32_OS_SERVER))
            {
              /*
               * This condition should now always hold, since Windows
               * 7+/Server 2008 R2+ is now required
               */
              if (versions[i].major == 6)
                {
                  g_string_append (version, "Server");
                  if (versions[i].minor == 2)
                    g_string_append (version, " 2012");
                  else if (versions[i].minor == 1)
                    g_string_append (version, " 2008 R2");
                  else
                    g_string_append (version, " 2008");
                }
            }

          g_string_append (version, versions[i].spversion);
        }
    }

  if (version->len == 0)
    {
      g_string_free (version, TRUE);

      return NULL;
    }

  if (with_windows)
    g_string_prepend (version, "Windows ");

  return g_string_free (version, FALSE);
}
#endif

#if defined (G_OS_UNIX) && !defined (G_OS_DARWIN)
static gchar *
get_os_info_from_os_release (const gchar *key_name,
                             const gchar *buffer)
{
  GStrv lines;
  gchar *prefix;
  size_t i;
  gchar *result = NULL;

  lines = g_strsplit (buffer, "\n", -1);
  prefix = g_strdup_printf ("%s=", key_name);
  for (i = 0; lines[i] != NULL; i++)
    {
      const gchar *line = lines[i];
      const gchar *value;

      if (g_str_has_prefix (line, prefix))
        {
          value = line + strlen (prefix);
          result = g_shell_unquote (value, NULL);
          if (result == NULL)
            result = g_strdup (value);
          break;
        }
    }
  g_strfreev (lines);
  g_free (prefix);

#ifdef __linux__
  /* Default values in spec */
  if (result == NULL)
    {
      if (g_str_equal (key_name, G_OS_INFO_KEY_NAME))
        return g_strdup ("Linux");
      if (g_str_equal (key_name, G_OS_INFO_KEY_ID))
        return g_strdup ("linux");
      if (g_str_equal (key_name, G_OS_INFO_KEY_PRETTY_NAME))
        return g_strdup ("Linux");
    }
#endif

  return g_steal_pointer (&result);
}

static gchar *
get_os_info_from_uname (const gchar *key_name)
{
  struct utsname info;

  if (uname (&info) == -1)
    return NULL;

  if (strcmp (key_name, G_OS_INFO_KEY_NAME) == 0)
    return g_strdup (info.sysname);
  else if (strcmp (key_name, G_OS_INFO_KEY_VERSION) == 0)
    return g_strdup (info.release);
  else if (strcmp (key_name, G_OS_INFO_KEY_PRETTY_NAME) == 0)
    return g_strdup_printf ("%s %s", info.sysname, info.release);
  else if (strcmp (key_name, G_OS_INFO_KEY_ID) == 0)
    {
      gchar *result = g_ascii_strdown (info.sysname, -1);

      g_strcanon (result, "abcdefghijklmnopqrstuvwxyz0123456789_-.", '_');
      return g_steal_pointer (&result);
    }
  else if (strcmp (key_name, G_OS_INFO_KEY_VERSION_ID) == 0)
    {
      /* We attempt to convert the version string to the format returned by
       * config.guess, which is the script used to generate target triplets
       * in GNU autotools. There are a lot of rules in the script. We only
       * implement a few rules which are easy to understand here.
       *
       * config.guess can be found at https://savannah.gnu.org/projects/config.
       */
      gchar *result;

      if (strcmp (info.sysname, "NetBSD") == 0)
        {
          /* sed -e 's,[-_].*,,' */
          gssize len = G_MAXSSIZE;
          const gchar *c;

          if ((c = strchr (info.release, '-')) != NULL)
            len = MIN (len, c - info.release);
          if ((c = strchr (info.release, '_')) != NULL)
            len = MIN (len, c - info.release);
          if (len == G_MAXSSIZE)
            len = -1;

          result = g_ascii_strdown (info.release, len);
        }
      else if (strcmp (info.sysname, "GNU") == 0)
        {
          /* sed -e 's,/.*$,,' */
          gssize len = -1;
          const gchar *c = strchr (info.release, '/');

          if (c != NULL)
            len = c - info.release;

          result = g_ascii_strdown (info.release, len);
        }
      else if (g_str_has_prefix (info.sysname, "GNU/") ||
               strcmp (info.sysname, "FreeBSD") == 0 ||
               strcmp (info.sysname, "DragonFly") == 0)
        {
          /* sed -e 's,[-(].*,,' */
          gssize len = G_MAXSSIZE;
          const gchar *c;

          if ((c = strchr (info.release, '-')) != NULL)
            len = MIN (len, c - info.release);
          if ((c = strchr (info.release, '(')) != NULL)
            len = MIN (len, c - info.release);
          if (len == G_MAXSSIZE)
            len = -1;

          result = g_ascii_strdown (info.release, len);
        }
      else
        result = g_ascii_strdown (info.release, -1);

      g_strcanon (result, "abcdefghijklmnopqrstuvwxyz0123456789_-.", '_');
      return g_steal_pointer (&result);
    }
  else
    return NULL;
}
#endif  /* defined (G_OS_UNIX) && !defined (G_OS_DARWIN) */

/**
 * g_get_os_info:
 * @key_name: a key for the OS info being requested, for example %G_OS_INFO_KEY_NAME.
 *
 * Get information about the operating system.
 *
 * On Linux this comes from the `/etc/os-release` file. On other systems, it may
 * come from a variety of sources. You can either use the standard key names
 * like %G_OS_INFO_KEY_NAME or pass any UTF-8 string key name. For example,
 * `/etc/os-release` provides a number of other less commonly used values that may
 * be useful. No key is guaranteed to be provided, so the caller should always
 * check if the result is %NULL.
 *
 * Returns: (nullable): The associated value for the requested key or %NULL if
 *   this information is not provided.
 *
 * Since: 2.64
 **/
gchar *
g_get_os_info (const gchar *key_name)
{
#if defined (G_OS_DARWIN)
  if (g_strcmp0 (key_name, G_OS_INFO_KEY_NAME) == 0)
    return g_strdup ("macOS");
  else
    return NULL;
#elif defined (G_OS_UNIX)
  const gchar * const os_release_files[] = { "/etc/os-release", "/usr/lib/os-release" };
  gsize i;
  gchar *buffer = NULL;
  gchar *result = NULL;

  g_return_val_if_fail (key_name != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (os_release_files); i++)
    {
      GError *error = NULL;
      gboolean file_missing;

      if (g_file_get_contents (os_release_files[i], &buffer, NULL, &error))
        break;

      file_missing = g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT);
      g_clear_error (&error);

      if (!file_missing)
        return NULL;
    }

  if (buffer != NULL)
    result = get_os_info_from_os_release (key_name, buffer);
  else
    result = get_os_info_from_uname (key_name);

  g_free (buffer);
  return g_steal_pointer (&result);
#elif defined (G_OS_WIN32)
  if (g_strcmp0 (key_name, G_OS_INFO_KEY_NAME) == 0)
    return g_strdup ("Windows");
  else if (g_strcmp0 (key_name, G_OS_INFO_KEY_ID) == 0)
    return g_strdup ("windows");
  else if (g_strcmp0 (key_name, G_OS_INFO_KEY_PRETTY_NAME) == 0)
    /* Windows XP SP2 or Windows 10 1903 or Windows 7 Server SP1 */
    return get_windows_version (TRUE);
  else if (g_strcmp0 (key_name, G_OS_INFO_KEY_VERSION) == 0)
    /* XP SP2 or 10 1903 or 7 Server SP1 */
    return get_windows_version (FALSE);
  else if (g_strcmp0 (key_name, G_OS_INFO_KEY_VERSION_ID) == 0)
    {
      /* xp_sp2 or 10_1903 or 7_server_sp1 */
      gchar *result;
      gchar *version = get_windows_version (FALSE);

      if (version == NULL)
        return NULL;

      result = g_ascii_strdown (version, -1);
      g_free (version);

      return g_strcanon (result, "abcdefghijklmnopqrstuvwxyz0123456789_-.", '_');
    }
  else if (g_strcmp0 (key_name, G_OS_INFO_KEY_HOME_URL) == 0)
    return g_strdup ("https://microsoft.com/windows/");
  else if (g_strcmp0 (key_name, G_OS_INFO_KEY_DOCUMENTATION_URL) == 0)
    return g_strdup ("https://docs.microsoft.com/");
  else if (g_strcmp0 (key_name, G_OS_INFO_KEY_SUPPORT_URL) == 0)
    return g_strdup ("https://support.microsoft.com/");
  else if (g_strcmp0 (key_name, G_OS_INFO_KEY_BUG_REPORT_URL) == 0)
    return g_strdup ("https://support.microsoft.com/contactus/");
  else if (g_strcmp0 (key_name, G_OS_INFO_KEY_PRIVACY_POLICY_URL) == 0)
    return g_strdup ("https://privacy.microsoft.com/");
  else
    return NULL;
#endif
}

/* Set @global_str to a copy of @new_value if it’s currently unset or has a
 * different value. If its current value matches @new_value, do nothing. If
 * replaced, we have to leak the old value as client code could still have
 * pointers to it. */
static void
set_str_if_different (gchar       **global_str,
                      const gchar  *type,
                      const gchar  *new_value)
{
  if (*global_str == NULL ||
      !g_str_equal (new_value, *global_str))
    {
      /* We have to leak the old value, as user code could be retaining pointers
       * to it. */
      g_ignore_leak (*global_str);
      *global_str = g_strdup (new_value);
    }
}

static void
set_strv_if_different (gchar                ***global_strv,
                       const gchar            *type,
                       const gchar  * const   *new_value)
{
  if (*global_strv == NULL ||
      !g_strv_equal (new_value, (const gchar * const *) *global_strv))
    {
      gchar *new_value_str = g_strjoinv (":", (gchar **) new_value);
      g_free (new_value_str);

      /* We have to leak the old value, as user code could be retaining pointers
       * to it. */
      g_ignore_strv_leak (*global_strv);
      *global_strv = g_strdupv ((gchar **) new_value);
    }
}

/*
 * g_set_user_dirs:
 * @first_dir_type: Type of the first directory to set
 * @...: Value to set the first directory to, followed by additional type/value
 *    pairs, followed by %NULL
 *
 * Set one or more ‘user’ directories to custom values. This is intended to be
 * used by test code (particularly with the %G_TEST_OPTION_ISOLATE_DIRS option)
 * to override the values returned by the following functions, so that test
 * code can be run without touching an installed system and user data:
 *
 *  - g_get_home_dir() — use type `HOME`, pass a string
 *  - g_get_user_cache_dir() — use type `XDG_CACHE_HOME`, pass a string
 *  - g_get_system_config_dirs() — use type `XDG_CONFIG_DIRS`, pass a
 *    %NULL-terminated string array
 *  - g_get_user_config_dir() — use type `XDG_CONFIG_HOME`, pass a string
 *  - g_get_system_data_dirs() — use type `XDG_DATA_DIRS`, pass a
 *    %NULL-terminated string array
 *  - g_get_user_data_dir() — use type `XDG_DATA_HOME`, pass a string
 *  - g_get_user_runtime_dir() — use type `XDG_RUNTIME_DIR`, pass a string
 *
 * The list must be terminated with a %NULL type. All of the values must be
 * non-%NULL — passing %NULL as a value won’t reset a directory. If a reference
 * to a directory from the calling environment needs to be kept, copy it before
 * the first call to g_set_user_dirs(). g_set_user_dirs() can be called multiple
 * times.
 *
 * Since: 2.60
 */
/*< private > */
void
g_set_user_dirs (const gchar *first_dir_type,
                 ...)
{
  va_list args;
  const gchar *dir_type;

  G_LOCK (g_utils_global);

  va_start (args, first_dir_type);

  for (dir_type = first_dir_type; dir_type != NULL; dir_type = va_arg (args, const gchar *))
    {
      gconstpointer dir_value = va_arg (args, gconstpointer);
      g_assert (dir_value != NULL);

      if (g_str_equal (dir_type, "HOME"))
        set_str_if_different (&g_home_dir, dir_type, dir_value);
      else if (g_str_equal (dir_type, "XDG_CACHE_HOME"))
        set_str_if_different (&g_user_cache_dir, dir_type, dir_value);
      else if (g_str_equal (dir_type, "XDG_CONFIG_DIRS"))
        set_strv_if_different (&g_system_config_dirs, dir_type, dir_value);
      else if (g_str_equal (dir_type, "XDG_CONFIG_HOME"))
        set_str_if_different (&g_user_config_dir, dir_type, dir_value);
      else if (g_str_equal (dir_type, "XDG_DATA_DIRS"))
        set_strv_if_different (&g_system_data_dirs, dir_type, dir_value);
      else if (g_str_equal (dir_type, "XDG_DATA_HOME"))
        set_str_if_different (&g_user_data_dir, dir_type, dir_value);
      else if (g_str_equal (dir_type, "XDG_STATE_HOME"))
        set_str_if_different (&g_user_state_dir, dir_type, dir_value);
      else if (g_str_equal (dir_type, "XDG_RUNTIME_DIR"))
        set_str_if_different (&g_user_runtime_dir, dir_type, dir_value);
      else
        g_assert_not_reached ();
    }

  va_end (args);

  G_UNLOCK (g_utils_global);
}

static gchar *
g_build_user_data_dir (void)
{
  gchar *data_dir = NULL;
  const gchar *data_dir_env = g_getenv ("XDG_DATA_HOME");

  if (data_dir_env && data_dir_env[0])
    data_dir = g_strdup (data_dir_env);
#ifdef G_OS_WIN32
  else
    data_dir = get_special_folder (CSIDL_LOCAL_APPDATA);
#endif
  if (!data_dir || !data_dir[0])
    {
      gchar *home_dir = g_build_home_dir ();
      data_dir = g_build_filename (home_dir, ".local", "share", NULL);
      g_free (home_dir);
    }

  return g_steal_pointer (&data_dir);
}

/**
 * g_get_user_data_dir:
 * 
 * Returns a base directory in which to access application data such
 * as icons that is customized for a particular user.  
 *
 * On UNIX platforms this is determined using the mechanisms described
 * in the
 * [XDG Base Directory Specification](http://www.freedesktop.org/Standards/basedir-spec).
 * In this case the directory retrieved will be `XDG_DATA_HOME`.
 *
 * On Windows it follows XDG Base Directory Specification if `XDG_DATA_HOME`
 * is defined. If `XDG_DATA_HOME` is undefined, the folder to use for local (as
 * opposed to roaming) application data is used instead. See the
 * [documentation for `CSIDL_LOCAL_APPDATA`](https://msdn.microsoft.com/en-us/library/windows/desktop/bb762494%28v=vs.85%29.aspx#csidl_local_appdata).
 * Note that in this case on Windows it will be the same
 * as what g_get_user_config_dir() returns.
 *
 * The return value is cached and modifying it at runtime is not supported, as
 * it’s not thread-safe to modify environment variables at runtime.
 *
 * Returns: (type filename) (transfer none): a string owned by GLib that must
 *   not be modified or freed.
 *
 * Since: 2.6
 **/
const gchar *
g_get_user_data_dir (void)
{
  const gchar *user_data_dir;

  G_LOCK (g_utils_global);

  if (g_user_data_dir == NULL)
    g_user_data_dir = g_build_user_data_dir ();
  user_data_dir = g_user_data_dir;

  G_UNLOCK (g_utils_global);

  return user_data_dir;
}

static gchar *
g_build_user_config_dir (void)
{
  gchar *config_dir = NULL;
  const gchar *config_dir_env = g_getenv ("XDG_CONFIG_HOME");

  if (config_dir_env && config_dir_env[0])
    config_dir = g_strdup (config_dir_env);
#ifdef G_OS_WIN32
  else
    config_dir = get_special_folder (CSIDL_LOCAL_APPDATA);
#endif
  if (!config_dir || !config_dir[0])
    {
      gchar *home_dir = g_build_home_dir ();
      config_dir = g_build_filename (home_dir, ".config", NULL);
      g_free (home_dir);
    }

  return g_steal_pointer (&config_dir);
}

/**
 * g_get_user_config_dir:
 * 
 * Returns a base directory in which to store user-specific application 
 * configuration information such as user preferences and settings. 
 *
 * On UNIX platforms this is determined using the mechanisms described
 * in the
 * [XDG Base Directory Specification](http://www.freedesktop.org/Standards/basedir-spec).
 * In this case the directory retrieved will be `XDG_CONFIG_HOME`.
 *
 * On Windows it follows XDG Base Directory Specification if `XDG_CONFIG_HOME` is defined.
 * If `XDG_CONFIG_HOME` is undefined, the folder to use for local (as opposed
 * to roaming) application data is used instead. See the
 * [documentation for `CSIDL_LOCAL_APPDATA`](https://msdn.microsoft.com/en-us/library/windows/desktop/bb762494%28v=vs.85%29.aspx#csidl_local_appdata).
 * Note that in this case on Windows it will be  the same
 * as what g_get_user_data_dir() returns.
 *
 * The return value is cached and modifying it at runtime is not supported, as
 * it’s not thread-safe to modify environment variables at runtime.
 *
 * Returns: (type filename) (transfer none): a string owned by GLib that
 *   must not be modified or freed.
 * Since: 2.6
 **/
const gchar *
g_get_user_config_dir (void)
{
  const gchar *user_config_dir;

  G_LOCK (g_utils_global);

  if (g_user_config_dir == NULL)
    g_user_config_dir = g_build_user_config_dir ();
  user_config_dir = g_user_config_dir;

  G_UNLOCK (g_utils_global);

  return user_config_dir;
}

static gchar *
g_build_user_cache_dir (void)
{
  gchar *cache_dir = NULL;
  const gchar *cache_dir_env = g_getenv ("XDG_CACHE_HOME");

  if (cache_dir_env && cache_dir_env[0])
    cache_dir = g_strdup (cache_dir_env);
#ifdef G_OS_WIN32
  else
    cache_dir = get_special_folder (CSIDL_INTERNET_CACHE);
#endif
  if (!cache_dir || !cache_dir[0])
    {
      gchar *home_dir = g_build_home_dir ();
      cache_dir = g_build_filename (home_dir, ".cache", NULL);
      g_free (home_dir);
    }

  return g_steal_pointer (&cache_dir);
}

/**
 * g_get_user_cache_dir:
 * 
 * Returns a base directory in which to store non-essential, cached
 * data specific to particular user.
 *
 * On UNIX platforms this is determined using the mechanisms described
 * in the
 * [XDG Base Directory Specification](http://www.freedesktop.org/Standards/basedir-spec).
 * In this case the directory retrieved will be `XDG_CACHE_HOME`.
 *
 * On Windows it follows XDG Base Directory Specification if `XDG_CACHE_HOME` is defined.
 * If `XDG_CACHE_HOME` is undefined, the directory that serves as a common
 * repository for temporary Internet files is used instead. A typical path is
 * `C:\Documents and Settings\username\Local Settings\Temporary Internet Files`.
 * See the [documentation for `CSIDL_INTERNET_CACHE`](https://msdn.microsoft.com/en-us/library/windows/desktop/bb762494%28v=vs.85%29.aspx#csidl_internet_cache).
 *
 * The return value is cached and modifying it at runtime is not supported, as
 * it’s not thread-safe to modify environment variables at runtime.
 *
 * Returns: (type filename) (transfer none): a string owned by GLib that
 *   must not be modified or freed.
 * Since: 2.6
 **/
const gchar *
g_get_user_cache_dir (void)
{
  const gchar *user_cache_dir;

  G_LOCK (g_utils_global);

  if (g_user_cache_dir == NULL)
    g_user_cache_dir = g_build_user_cache_dir ();
  user_cache_dir = g_user_cache_dir;

  G_UNLOCK (g_utils_global);

  return user_cache_dir;
}

static gchar *
g_build_user_state_dir (void)
{
  gchar *state_dir = NULL;
  const gchar *state_dir_env = g_getenv ("XDG_STATE_HOME");

  if (state_dir_env && state_dir_env[0])
    state_dir = g_strdup (state_dir_env);
#ifdef G_OS_WIN32
  else
    state_dir = get_special_folder (CSIDL_LOCAL_APPDATA);
#endif
  if (!state_dir || !state_dir[0])
    {
      gchar *home_dir = g_build_home_dir ();
      state_dir = g_build_filename (home_dir, ".local/state", NULL);
      g_free (home_dir);
    }

  return g_steal_pointer (&state_dir);
}

/**
 * g_get_user_state_dir:
 *
 * Returns a base directory in which to store state files specific to
 * particular user.
 *
 * On UNIX platforms this is determined using the mechanisms described
 * in the
 * [XDG Base Directory Specification](http://www.freedesktop.org/Standards/basedir-spec).
 * In this case the directory retrieved will be `XDG_STATE_HOME`.
 *
 * On Windows it follows XDG Base Directory Specification if `XDG_STATE_HOME` is defined.
 * If `XDG_STATE_HOME` is undefined, the folder to use for local (as opposed
 * to roaming) application data is used instead. See the
 * [documentation for `FOLDERID_LocalAppData`](https://docs.microsoft.com/en-us/windows/win32/shell/knownfolderid).
 * Note that in this case on Windows it will be the same
 * as what g_get_user_data_dir() returns.
 *
 * The return value is cached and modifying it at runtime is not supported, as
 * it’s not thread-safe to modify environment variables at runtime.
 *
 * Returns: (type filename) (transfer none): a string owned by GLib that
 *   must not be modified or freed.
 *
 * Since: 2.72
 **/
const gchar *
g_get_user_state_dir (void)
{
  const gchar *user_state_dir;

  G_LOCK (g_utils_global);

  if (g_user_state_dir == NULL)
    g_user_state_dir = g_build_user_state_dir ();
  user_state_dir = g_user_state_dir;

  G_UNLOCK (g_utils_global);

  return user_state_dir;
}

static gchar *
g_build_user_runtime_dir (void)
{
  gchar *runtime_dir = NULL;
  const gchar *runtime_dir_env = g_getenv ("XDG_RUNTIME_DIR");

  if (runtime_dir_env && runtime_dir_env[0])
    {
      runtime_dir = g_strdup (runtime_dir_env);

      /* If the XDG_RUNTIME_DIR environment variable is set, we are being told by
       * the OS that this directory exists and is appropriately configured
       * already.
       */
    }
  else
    {
      runtime_dir = g_build_user_cache_dir ();

      /* Fallback case: the directory may not yet exist.
       *
       * The user should be able to rely on the directory existing
       * when the function returns.  Probably it already does, but
       * let's make sure.  Just do mkdir() directly since it will be
       * no more expensive than a stat() in the case that the
       * directory already exists and is a lot easier.
       *
       * $XDG_CACHE_HOME is probably ~/.cache/ so as long as $HOME
       * exists this will work.  If the user changed $XDG_CACHE_HOME
       * then they can make sure that it exists...
       */
      (void) g_mkdir (runtime_dir, 0700);
    }

  return g_steal_pointer (&runtime_dir);
}

/**
 * g_get_user_runtime_dir:
 *
 * Returns a directory that is unique to the current user on the local
 * system.
 *
 * This is determined using the mechanisms described
 * in the 
 * [XDG Base Directory Specification](http://www.freedesktop.org/Standards/basedir-spec).
 * This is the directory
 * specified in the `XDG_RUNTIME_DIR` environment variable.
 * In the case that this variable is not set, we return the value of
 * g_get_user_cache_dir(), after verifying that it exists.
 *
 * The return value is cached and modifying it at runtime is not supported, as
 * it’s not thread-safe to modify environment variables at runtime.
 *
 * Returns: (type filename): a string owned by GLib that must not be
 *     modified or freed.
 *
 * Since: 2.28
 **/
const gchar *
g_get_user_runtime_dir (void)
{
  const gchar *user_runtime_dir;

  G_LOCK (g_utils_global);

  if (g_user_runtime_dir == NULL)
    g_user_runtime_dir = g_build_user_runtime_dir ();
  user_runtime_dir = g_user_runtime_dir;

  G_UNLOCK (g_utils_global);

  return user_runtime_dir;
}

#ifdef G_OS_DARWIN

/* Implemented in gutils-macos.m */
void load_user_special_dirs_macos (gchar **table);

static void
load_user_special_dirs (void)
{
#ifdef HAVE_COCOA
  load_user_special_dirs_macos (g_user_special_dirs);
#endif
}

#elif defined(G_OS_WIN32)

static void
load_user_special_dirs (void)
{
  typedef HRESULT (WINAPI *t_SHGetKnownFolderPath) (const GUID *rfid,
						    DWORD dwFlags,
						    HANDLE hToken,
						    PWSTR *ppszPath);
  t_SHGetKnownFolderPath p_SHGetKnownFolderPath;
  wchar_t *wcp;

  p_SHGetKnownFolderPath = (t_SHGetKnownFolderPath) GetProcAddress (GetModuleHandleW (L"shell32.dll"),
								    "SHGetKnownFolderPath");

  g_user_special_dirs[G_USER_DIRECTORY_DESKTOP] = get_special_folder (CSIDL_DESKTOPDIRECTORY);
  g_user_special_dirs[G_USER_DIRECTORY_DOCUMENTS] = get_special_folder (CSIDL_PERSONAL);

  if (p_SHGetKnownFolderPath == NULL)
    {
      g_user_special_dirs[G_USER_DIRECTORY_DOWNLOAD] = get_special_folder (CSIDL_DESKTOPDIRECTORY);
    }
  else
    {
      wcp = NULL;
      (*p_SHGetKnownFolderPath) (&FOLDERID_Downloads, 0, NULL, &wcp);
      if (wcp)
        {
          g_user_special_dirs[G_USER_DIRECTORY_DOWNLOAD] = g_utf16_to_utf8 (wcp, -1, NULL, NULL, NULL);
          if (g_user_special_dirs[G_USER_DIRECTORY_DOWNLOAD] == NULL)
              g_user_special_dirs[G_USER_DIRECTORY_DOWNLOAD] = get_special_folder (CSIDL_DESKTOPDIRECTORY);
          CoTaskMemFree (wcp);
        }
      else
          g_user_special_dirs[G_USER_DIRECTORY_DOWNLOAD] = get_special_folder (CSIDL_DESKTOPDIRECTORY);
    }

  g_user_special_dirs[G_USER_DIRECTORY_MUSIC] = get_special_folder (CSIDL_MYMUSIC);
  g_user_special_dirs[G_USER_DIRECTORY_PICTURES] = get_special_folder (CSIDL_MYPICTURES);

  if (p_SHGetKnownFolderPath == NULL)
    {
      /* XXX */
      g_user_special_dirs[G_USER_DIRECTORY_PUBLIC_SHARE] = get_special_folder (CSIDL_COMMON_DOCUMENTS);
    }
  else
    {
      wcp = NULL;
      (*p_SHGetKnownFolderPath) (&FOLDERID_Public, 0, NULL, &wcp);
      if (wcp)
        {
          g_user_special_dirs[G_USER_DIRECTORY_PUBLIC_SHARE] = g_utf16_to_utf8 (wcp, -1, NULL, NULL, NULL);
          if (g_user_special_dirs[G_USER_DIRECTORY_PUBLIC_SHARE] == NULL)
              g_user_special_dirs[G_USER_DIRECTORY_PUBLIC_SHARE] = get_special_folder (CSIDL_COMMON_DOCUMENTS);
          CoTaskMemFree (wcp);
        }
      else
          g_user_special_dirs[G_USER_DIRECTORY_PUBLIC_SHARE] = get_special_folder (CSIDL_COMMON_DOCUMENTS);
    }
  
  g_user_special_dirs[G_USER_DIRECTORY_TEMPLATES] = get_special_folder (CSIDL_TEMPLATES);
  g_user_special_dirs[G_USER_DIRECTORY_VIDEOS] = get_special_folder (CSIDL_MYVIDEO);
}

#else /* default is unix */

/* adapted from xdg-user-dir-lookup.c
 *
 * Copyright (C) 2007 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions: 
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software. 
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
static void
load_user_special_dirs (void)
{
  gchar *config_dir = NULL;
  gchar *config_file;
  gchar *data;
  gchar **lines;
  gint n_lines, i;
  
  config_dir = g_build_user_config_dir ();
  config_file = g_build_filename (config_dir,
                                  "user-dirs.dirs",
                                  NULL);
  g_free (config_dir);

  if (!g_file_get_contents (config_file, &data, NULL, NULL))
    {
      g_free (config_file);
      return;
    }

  lines = g_strsplit (data, "\n", -1);
  n_lines = g_strv_length (lines);
  g_free (data);
  
  for (i = 0; i < n_lines; i++)
    {
      gchar *buffer = lines[i];
      gchar *d, *p;
      gint len;
      gboolean is_relative = FALSE;
      GUserDirectory directory;

      /* Remove newline at end */
      len = strlen (buffer);
      if (len > 0 && buffer[len - 1] == '\n')
	buffer[len - 1] = 0;
      
      p = buffer;
      while (*p == ' ' || *p == '\t')
	p++;
      
      if (strncmp (p, "XDG_DESKTOP_DIR", strlen ("XDG_DESKTOP_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_DESKTOP;
          p += strlen ("XDG_DESKTOP_DIR");
        }
      else if (strncmp (p, "XDG_DOCUMENTS_DIR", strlen ("XDG_DOCUMENTS_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_DOCUMENTS;
          p += strlen ("XDG_DOCUMENTS_DIR");
        }
      else if (strncmp (p, "XDG_DOWNLOAD_DIR", strlen ("XDG_DOWNLOAD_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_DOWNLOAD;
          p += strlen ("XDG_DOWNLOAD_DIR");
        }
      else if (strncmp (p, "XDG_MUSIC_DIR", strlen ("XDG_MUSIC_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_MUSIC;
          p += strlen ("XDG_MUSIC_DIR");
        }
      else if (strncmp (p, "XDG_PICTURES_DIR", strlen ("XDG_PICTURES_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_PICTURES;
          p += strlen ("XDG_PICTURES_DIR");
        }
      else if (strncmp (p, "XDG_PUBLICSHARE_DIR", strlen ("XDG_PUBLICSHARE_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_PUBLIC_SHARE;
          p += strlen ("XDG_PUBLICSHARE_DIR");
        }
      else if (strncmp (p, "XDG_TEMPLATES_DIR", strlen ("XDG_TEMPLATES_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_TEMPLATES;
          p += strlen ("XDG_TEMPLATES_DIR");
        }
      else if (strncmp (p, "XDG_VIDEOS_DIR", strlen ("XDG_VIDEOS_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_VIDEOS;
          p += strlen ("XDG_VIDEOS_DIR");
        }
      else
	continue;

      while (*p == ' ' || *p == '\t')
	p++;

      if (*p != '=')
	continue;
      p++;

      while (*p == ' ' || *p == '\t')
	p++;

      if (*p != '"')
	continue;
      p++;

      if (strncmp (p, "$HOME", 5) == 0)
	{
	  p += 5;
	  is_relative = TRUE;
	}
      else if (*p != '/')
	continue;

      d = strrchr (p, '"');
      if (!d)
        continue;
      *d = 0;

      d = p;
      
      /* remove trailing slashes */
      len = strlen (d);
      if (d[len - 1] == '/')
        d[len - 1] = 0;
      
      if (is_relative)
        {
          gchar *home_dir = g_build_home_dir ();
          g_user_special_dirs[directory] = g_build_filename (home_dir, d, NULL);
          g_free (home_dir);
        }
      else
	g_user_special_dirs[directory] = g_strdup (d);
    }

  g_strfreev (lines);
  g_free (config_file);
}

#endif /* platform-specific load_user_special_dirs implementations */


/**
 * g_reload_user_special_dirs_cache:
 *
 * Resets the cache used for g_get_user_special_dir(), so
 * that the latest on-disk version is used. Call this only
 * if you just changed the data on disk yourself.
 *
 * Due to thread safety issues this may cause leaking of strings
 * that were previously returned from g_get_user_special_dir()
 * that can't be freed. We ensure to only leak the data for
 * the directories that actually changed value though.
 *
 * Since: 2.22
 */
void
g_reload_user_special_dirs_cache (void)
{
  int i;

  G_LOCK (g_utils_global);

  if (g_user_special_dirs != NULL)
    {
      /* save a copy of the pointer, to check if some memory can be preserved */
      char **old_g_user_special_dirs = g_user_special_dirs;
      char *old_val;

      /* recreate and reload our cache */
      g_user_special_dirs = g_new0 (gchar *, G_USER_N_DIRECTORIES);
      load_user_special_dirs ();

      /* only leak changed directories */
      for (i = 0; i < G_USER_N_DIRECTORIES; i++)
        {
          old_val = old_g_user_special_dirs[i];
          if (g_user_special_dirs[i] == NULL)
            {
              g_user_special_dirs[i] = old_val;
            }
          else if (g_strcmp0 (old_val, g_user_special_dirs[i]) == 0)
            {
              /* don't leak */
              g_free (g_user_special_dirs[i]);
              g_user_special_dirs[i] = old_val;
            }
          else
            g_free (old_val);
        }

      /* free the old array */
      g_free (old_g_user_special_dirs);
    }

  G_UNLOCK (g_utils_global);
}

/**
 * g_get_user_special_dir:
 * @directory: the logical id of special directory
 *
 * Returns the full path of a special directory using its logical id.
 *
 * On UNIX this is done using the XDG special user directories.
 * For compatibility with existing practise, %G_USER_DIRECTORY_DESKTOP
 * falls back to `$HOME/Desktop` when XDG special user directories have
 * not been set up. 
 *
 * Depending on the platform, the user might be able to change the path
 * of the special directory without requiring the session to restart; GLib
 * will not reflect any change once the special directories are loaded.
 *
 * Returns: (type filename) (nullable): the path to the specified special
 *   directory, or %NULL if the logical id was not found. The returned string is
 *   owned by GLib and should not be modified or freed.
 *
 * Since: 2.14
 */
const gchar *
g_get_user_special_dir (GUserDirectory directory)
{
  const gchar *user_special_dir;

  g_return_val_if_fail (directory >= G_USER_DIRECTORY_DESKTOP &&
                        directory < G_USER_N_DIRECTORIES, NULL);

  G_LOCK (g_utils_global);

  if (G_UNLIKELY (g_user_special_dirs == NULL))
    {
      g_user_special_dirs = g_new0 (gchar *, G_USER_N_DIRECTORIES);

      load_user_special_dirs ();

      /* Special-case desktop for historical compatibility */
      if (g_user_special_dirs[G_USER_DIRECTORY_DESKTOP] == NULL)
        {
          gchar *home_dir = g_build_home_dir ();
          g_user_special_dirs[G_USER_DIRECTORY_DESKTOP] = g_build_filename (home_dir, "Desktop", NULL);
          g_free (home_dir);
        }
    }
  user_special_dir = g_user_special_dirs[directory];

  G_UNLOCK (g_utils_global);

  return user_special_dir;
}

#ifdef G_OS_WIN32

#undef g_get_system_data_dirs

static HMODULE
get_module_for_address (gconstpointer address)
{
  /* Holds the g_utils_global lock */

  HMODULE hmodule = NULL;

  if (!address)
    return NULL;

  if (!GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
			   GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
			   address, &hmodule))
    {
      MEMORY_BASIC_INFORMATION mbi;
      VirtualQuery (address, &mbi, sizeof (mbi));
      hmodule = (HMODULE) mbi.AllocationBase;
    }

  return hmodule;
}

static gchar *
get_module_share_dir (gconstpointer address)
{
  HMODULE hmodule;
  gchar *filename;
  gchar *retval;

  hmodule = get_module_for_address (address);
  if (hmodule == NULL)
    return NULL;

  filename = g_win32_get_package_installation_directory_of_module (hmodule);
  retval = g_build_filename (filename, "share", NULL);
  g_free (filename);

  return retval;
}

static const gchar * const *
g_win32_get_system_data_dirs_for_module_real (void (*address_of_function)(void))
{
  GArray *data_dirs;
  HMODULE hmodule;
  static GHashTable *per_module_data_dirs = NULL;
  gchar **retval;
  gchar *p;
  gchar *exe_root;

  hmodule = NULL;
  if (address_of_function)
    {
      G_LOCK (g_utils_global);
      hmodule = get_module_for_address (address_of_function);
      if (hmodule != NULL)
	{
	  if (per_module_data_dirs == NULL)
	    per_module_data_dirs = g_hash_table_new (NULL, NULL);
	  else
	    {
	      retval = g_hash_table_lookup (per_module_data_dirs, hmodule);
	      
	      if (retval != NULL)
		{
		  G_UNLOCK (g_utils_global);
		  return (const gchar * const *) retval;
		}
	    }
	}
    }

  data_dirs = g_array_new (TRUE, TRUE, sizeof (char *));

  /* Documents and Settings\All Users\Application Data */
  p = get_special_folder (CSIDL_COMMON_APPDATA);
  if (p)
    g_array_append_val (data_dirs, p);
  
  /* Documents and Settings\All Users\Documents */
  p = get_special_folder (CSIDL_COMMON_DOCUMENTS);
  if (p)
    g_array_append_val (data_dirs, p);
	
  /* Using the above subfolders of Documents and Settings perhaps
   * makes sense from a Windows perspective.
   *
   * But looking at the actual use cases of this function in GTK+
   * and GNOME software, what we really want is the "share"
   * subdirectory of the installation directory for the package
   * our caller is a part of.
   *
   * The address_of_function parameter, if non-NULL, points to a
   * function in the calling module. Use that to determine that
   * module's installation folder, and use its "share" subfolder.
   *
   * Additionally, also use the "share" subfolder of the installation
   * locations of GLib and the .exe file being run.
   *
   * To guard against none of the above being what is really wanted,
   * callers of this function should have Win32-specific code to look
   * up their installation folder themselves, and handle a subfolder
   * "share" of it in the same way as the folders returned from this
   * function.
   */

  p = get_module_share_dir (address_of_function);
  if (p)
    g_array_append_val (data_dirs, p);
    
  if (glib_dll != NULL)
    {
      gchar *glib_root = g_win32_get_package_installation_directory_of_module (glib_dll);
      p = g_build_filename (glib_root, "share", NULL);
      if (p)
	g_array_append_val (data_dirs, p);
      g_free (glib_root);
    }
  
  exe_root = g_win32_get_package_installation_directory_of_module (NULL);
  p = g_build_filename (exe_root, "share", NULL);
  if (p)
    g_array_append_val (data_dirs, p);
  g_free (exe_root);

  retval = (gchar **) g_array_free (data_dirs, FALSE);

  if (address_of_function)
    {
      if (hmodule != NULL)
	g_hash_table_insert (per_module_data_dirs, hmodule, retval);
      G_UNLOCK (g_utils_global);
    }

  return (const gchar * const *) retval;
}

const gchar * const *
g_win32_get_system_data_dirs_for_module (void (*address_of_function)(void))
{
  gboolean should_call_g_get_system_data_dirs;

  should_call_g_get_system_data_dirs = TRUE;
  /* These checks are the same as the ones that g_build_system_data_dirs() does.
   * Please keep them in sync.
   */
  G_LOCK (g_utils_global);

  if (!g_system_data_dirs)
    {
      const gchar *data_dirs = g_getenv ("XDG_DATA_DIRS");

      if (!data_dirs || !data_dirs[0])
        should_call_g_get_system_data_dirs = FALSE;
    }

  G_UNLOCK (g_utils_global);

  /* There is a subtle difference between g_win32_get_system_data_dirs_for_module (NULL),
   * which is what GLib code can normally call,
   * and g_win32_get_system_data_dirs_for_module (&_g_win32_get_system_data_dirs),
   * which is what the inline function used by non-GLib code calls.
   * The former gets prefix relative to currently-running executable,
   * the latter - relative to the module that calls _g_win32_get_system_data_dirs()
   * (disguised as g_get_system_data_dirs()), which could be an executable or
   * a DLL that is located somewhere else.
   * This is why that inline function in gutils.h exists, and why we can't just
   * call g_get_system_data_dirs() from there - because we need to get the address
   * local to the non-GLib caller-module.
   */

  /*
   * g_get_system_data_dirs() will fall back to calling
   * g_win32_get_system_data_dirs_for_module_real(NULL) if XDG_DATA_DIRS is NULL
   * or an empty string. The checks above ensure that we do not call it in such
   * cases and use the address_of_function that we've been given by the inline function.
   * The reason we're calling g_get_system_data_dirs /at all/ is to give
   * XDG_DATA_DIRS precedence (if it is set).
   */
  if (should_call_g_get_system_data_dirs)
    return g_get_system_data_dirs ();

  return g_win32_get_system_data_dirs_for_module_real (address_of_function);
}

#endif

static gchar **
g_build_system_data_dirs (void)
{
  gchar **data_dir_vector = NULL;
  gchar *data_dirs = (gchar *) g_getenv ("XDG_DATA_DIRS");

  /* These checks are the same as the ones that g_win32_get_system_data_dirs_for_module()
   * does. Please keep them in sync.
   */
#ifndef G_OS_WIN32
  if (!data_dirs || !data_dirs[0])
    data_dirs = "/usr/local/share/:/usr/share/";

  data_dir_vector = g_strsplit (data_dirs, G_SEARCHPATH_SEPARATOR_S, 0);
#else
  if (!data_dirs || !data_dirs[0])
    data_dir_vector = g_strdupv ((gchar **) g_win32_get_system_data_dirs_for_module_real (NULL));
  else
    data_dir_vector = g_strsplit (data_dirs, G_SEARCHPATH_SEPARATOR_S, 0);
#endif

  return g_steal_pointer (&data_dir_vector);
}

/**
 * g_get_system_data_dirs:
 * 
 * Returns an ordered list of base directories in which to access 
 * system-wide application data.
 *
 * On UNIX platforms this is determined using the mechanisms described
 * in the
 * [XDG Base Directory Specification](http://www.freedesktop.org/Standards/basedir-spec)
 * In this case the list of directories retrieved will be `XDG_DATA_DIRS`.
 *
 * On Windows it follows XDG Base Directory Specification if `XDG_DATA_DIRS` is defined.
 * If `XDG_DATA_DIRS` is undefined,
 * the first elements in the list are the Application Data
 * and Documents folders for All Users. (These can be determined only
 * on Windows 2000 or later and are not present in the list on other
 * Windows versions.) See documentation for CSIDL_COMMON_APPDATA and
 * CSIDL_COMMON_DOCUMENTS.
 *
 * Then follows the "share" subfolder in the installation folder for
 * the package containing the DLL that calls this function, if it can
 * be determined.
 * 
 * Finally the list contains the "share" subfolder in the installation
 * folder for GLib, and in the installation folder for the package the
 * application's .exe file belongs to.
 *
 * The installation folders above are determined by looking up the
 * folder where the module (DLL or EXE) in question is located. If the
 * folder's name is "bin", its parent is used, otherwise the folder
 * itself.
 *
 * Note that on Windows the returned list can vary depending on where
 * this function is called.
 *
 * The return value is cached and modifying it at runtime is not supported, as
 * it’s not thread-safe to modify environment variables at runtime.
 *
 * Returns: (array zero-terminated=1) (element-type filename) (transfer none):
 *     a %NULL-terminated array of strings owned by GLib that must not be
 *     modified or freed.
 * 
 * Since: 2.6
 **/
const gchar * const * 
g_get_system_data_dirs (void)
{
  const gchar * const *system_data_dirs;

  G_LOCK (g_utils_global);

  if (g_system_data_dirs == NULL)
    g_system_data_dirs = g_build_system_data_dirs ();
  system_data_dirs = (const gchar * const *) g_system_data_dirs;

  G_UNLOCK (g_utils_global);

  return system_data_dirs;
}

static gchar **
g_build_system_config_dirs (void)
{
  gchar **conf_dir_vector = NULL;
  const gchar *conf_dirs = g_getenv ("XDG_CONFIG_DIRS");
#ifdef G_OS_WIN32
  if (conf_dirs)
    {
      conf_dir_vector = g_strsplit (conf_dirs, G_SEARCHPATH_SEPARATOR_S, 0);
    }
  else
    {
      gchar *special_conf_dirs = get_special_folder (CSIDL_COMMON_APPDATA);

      if (special_conf_dirs)
        conf_dir_vector = g_strsplit (special_conf_dirs, G_SEARCHPATH_SEPARATOR_S, 0);
      else
        /* Return empty list */
        conf_dir_vector = g_strsplit ("", G_SEARCHPATH_SEPARATOR_S, 0);

      g_free (special_conf_dirs);
    }
#else
  if (!conf_dirs || !conf_dirs[0])
    conf_dirs = "/etc/xdg";

  conf_dir_vector = g_strsplit (conf_dirs, G_SEARCHPATH_SEPARATOR_S, 0);
#endif

  return g_steal_pointer (&conf_dir_vector);
}

/**
 * g_get_system_config_dirs:
 * 
 * Returns an ordered list of base directories in which to access 
 * system-wide configuration information.
 *
 * On UNIX platforms this is determined using the mechanisms described
 * in the
 * [XDG Base Directory Specification](http://www.freedesktop.org/Standards/basedir-spec).
 * In this case the list of directories retrieved will be `XDG_CONFIG_DIRS`.
 *
 * On Windows it follows XDG Base Directory Specification if `XDG_CONFIG_DIRS` is defined.
 * If `XDG_CONFIG_DIRS` is undefined, the directory that contains application
 * data for all users is used instead. A typical path is
 * `C:\Documents and Settings\All Users\Application Data`.
 * This folder is used for application data
 * that is not user specific. For example, an application can store
 * a spell-check dictionary, a database of clip art, or a log file in the
 * CSIDL_COMMON_APPDATA folder. This information will not roam and is available
 * to anyone using the computer.
 *
 * The return value is cached and modifying it at runtime is not supported, as
 * it’s not thread-safe to modify environment variables at runtime.
 *
 * Returns: (array zero-terminated=1) (element-type filename) (transfer none):
 *     a %NULL-terminated array of strings owned by GLib that must not be
 *     modified or freed.
 * 
 * Since: 2.6
 **/
const gchar * const *
g_get_system_config_dirs (void)
{
  const gchar * const *system_config_dirs;

  G_LOCK (g_utils_global);

  if (g_system_config_dirs == NULL)
    g_system_config_dirs = g_build_system_config_dirs ();
  system_config_dirs = (const gchar * const *) g_system_config_dirs;

  G_UNLOCK (g_utils_global);

  return system_config_dirs;
}

/**
 * g_nullify_pointer:
 * @nullify_location: (not nullable): the memory address of the pointer.
 *
 * Set the pointer at the specified location to %NULL.
 **/
void
g_nullify_pointer (gpointer *nullify_location)
{
  g_return_if_fail (nullify_location != NULL);

  *nullify_location = NULL;
}

#define KILOBYTE_FACTOR (G_GOFFSET_CONSTANT (1000))
#define MEGABYTE_FACTOR (KILOBYTE_FACTOR * KILOBYTE_FACTOR)
#define GIGABYTE_FACTOR (MEGABYTE_FACTOR * KILOBYTE_FACTOR)
#define TERABYTE_FACTOR (GIGABYTE_FACTOR * KILOBYTE_FACTOR)
#define PETABYTE_FACTOR (TERABYTE_FACTOR * KILOBYTE_FACTOR)
#define EXABYTE_FACTOR  (PETABYTE_FACTOR * KILOBYTE_FACTOR)

#define KIBIBYTE_FACTOR (G_GOFFSET_CONSTANT (1024))
#define MEBIBYTE_FACTOR (KIBIBYTE_FACTOR * KIBIBYTE_FACTOR)
#define GIBIBYTE_FACTOR (MEBIBYTE_FACTOR * KIBIBYTE_FACTOR)
#define TEBIBYTE_FACTOR (GIBIBYTE_FACTOR * KIBIBYTE_FACTOR)
#define PEBIBYTE_FACTOR (TEBIBYTE_FACTOR * KIBIBYTE_FACTOR)
#define EXBIBYTE_FACTOR (PEBIBYTE_FACTOR * KIBIBYTE_FACTOR)

/**
 * g_format_size:
 * @size: a size in bytes
 *
 * Formats a size (for example the size of a file) into a human readable
 * string.  Sizes are rounded to the nearest size prefix (kB, MB, GB)
 * and are displayed rounded to the nearest tenth. E.g. the file size
 * 3292528 bytes will be converted into the string "3.2 MB". The returned string
 * is UTF-8, and may use a non-breaking space to separate the number and units,
 * to ensure they aren’t separated when line wrapped.
 *
 * The prefix units base is 1000 (i.e. 1 kB is 1000 bytes).
 *
 * This string should be freed with g_free() when not needed any longer.
 *
 * See g_format_size_full() for more options about how the size might be
 * formatted.
 *
 * Returns: (transfer full): a newly-allocated formatted string containing
 *   a human readable file size
 *
 * Since: 2.30
 */
gchar *
g_format_size (guint64 size)
{
  return g_format_size_full (size, G_FORMAT_SIZE_DEFAULT);
}

/**
 * GFormatSizeFlags:
 * @G_FORMAT_SIZE_DEFAULT: behave the same as g_format_size()
 * @G_FORMAT_SIZE_LONG_FORMAT: include the exact number of bytes as part
 *     of the returned string.  For example, "45.6 kB (45,612 bytes)".
 * @G_FORMAT_SIZE_IEC_UNITS: use IEC (base 1024) units with "KiB"-style
 *     suffixes. IEC units should only be used for reporting things with
 *     a strong "power of 2" basis, like RAM sizes or RAID stripe sizes.
 *     Network and storage sizes should be reported in the normal SI units.
 * @G_FORMAT_SIZE_BITS: set the size as a quantity in bits, rather than
 *     bytes, and return units in bits. For example, ‘Mb’ rather than ‘MB’.
 * @G_FORMAT_SIZE_ONLY_VALUE: return only value, without unit; this should
 *     not be used together with @G_FORMAT_SIZE_LONG_FORMAT
 *     nor @G_FORMAT_SIZE_ONLY_UNIT. Since: 2.74
 * @G_FORMAT_SIZE_ONLY_UNIT: return only unit, without value; this should
 *     not be used together with @G_FORMAT_SIZE_LONG_FORMAT
 *     nor @G_FORMAT_SIZE_ONLY_VALUE. Since: 2.74
 *
 * Flags to modify the format of the string returned by g_format_size_full().
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

/**
 * g_format_size_full:
 * @size: a size in bytes
 * @flags: #GFormatSizeFlags to modify the output
 *
 * Formats a size.
 *
 * This function is similar to g_format_size() but allows for flags
 * that modify the output. See #GFormatSizeFlags.
 *
 * Returns: (transfer full): a newly-allocated formatted string
 *   containing a human readable file size
 *
 * Since: 2.30
 */
gchar *
g_format_size_full (guint64          size,
                    GFormatSizeFlags flags)
{
  struct Format
  {
    guint64 factor;
    char string[10];
  };

  typedef enum
  {
    FORMAT_BYTES,
    FORMAT_BYTES_IEC,
    FORMAT_BITS,
    FORMAT_BITS_IEC
  } FormatIndex;

  const struct Format formats[4][6] = {
    {
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 kB" */
      { KILOBYTE_FACTOR, N_("kB") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 MB" */
      { MEGABYTE_FACTOR, N_("MB") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 GB" */
      { GIGABYTE_FACTOR, N_("GB") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 TB" */
      { TERABYTE_FACTOR, N_("TB") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 PB" */
      { PETABYTE_FACTOR, N_("PB") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 EB" */
      { EXABYTE_FACTOR,  N_("EB") }
    },
    {
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 KiB" */
      { KIBIBYTE_FACTOR, N_("KiB") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 MiB" */
      { MEBIBYTE_FACTOR, N_("MiB") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 GiB" */
      { GIBIBYTE_FACTOR, N_("GiB") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 TiB" */
      { TEBIBYTE_FACTOR, N_("TiB") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 PiB" */
      { PEBIBYTE_FACTOR, N_("PiB") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 EiB" */
      { EXBIBYTE_FACTOR, N_("EiB") }
    },
    {
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 kb" */
      { KILOBYTE_FACTOR, N_("kb") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 Mb" */
      { MEGABYTE_FACTOR, N_("Mb") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 Gb" */
      { GIGABYTE_FACTOR, N_("Gb") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 Tb" */
      { TERABYTE_FACTOR, N_("Tb") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 Pb" */
      { PETABYTE_FACTOR, N_("Pb") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 Eb" */
      { EXABYTE_FACTOR,  N_("Eb") }
    },
    {
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 Kib" */
      { KIBIBYTE_FACTOR, N_("Kib") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 Mib" */
      { MEBIBYTE_FACTOR, N_("Mib") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 Gib" */
      { GIBIBYTE_FACTOR, N_("Gib") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 Tib" */
      { TEBIBYTE_FACTOR, N_("Tib") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 Pib" */
      { PEBIBYTE_FACTOR, N_("Pib") },
      /* Translators: A unit symbol for size formatting, showing for example: "13.0 Eib" */
      { EXBIBYTE_FACTOR, N_("Eib") }
    }
  };

  GString *string;
  FormatIndex index;

  g_return_val_if_fail ((flags & (G_FORMAT_SIZE_LONG_FORMAT | G_FORMAT_SIZE_ONLY_VALUE)) != (G_FORMAT_SIZE_LONG_FORMAT | G_FORMAT_SIZE_ONLY_VALUE), NULL);
  g_return_val_if_fail ((flags & (G_FORMAT_SIZE_LONG_FORMAT | G_FORMAT_SIZE_ONLY_UNIT)) != (G_FORMAT_SIZE_LONG_FORMAT | G_FORMAT_SIZE_ONLY_UNIT), NULL);
  g_return_val_if_fail ((flags & (G_FORMAT_SIZE_ONLY_VALUE | G_FORMAT_SIZE_ONLY_UNIT)) != (G_FORMAT_SIZE_ONLY_VALUE | G_FORMAT_SIZE_ONLY_UNIT), NULL);

  string = g_string_new (NULL);

  switch (flags & ~(G_FORMAT_SIZE_LONG_FORMAT | G_FORMAT_SIZE_ONLY_VALUE | G_FORMAT_SIZE_ONLY_UNIT))
    {
    case G_FORMAT_SIZE_DEFAULT:
      index = FORMAT_BYTES;
      break;
    case (G_FORMAT_SIZE_DEFAULT | G_FORMAT_SIZE_IEC_UNITS):
      index = FORMAT_BYTES_IEC;
      break;
    case G_FORMAT_SIZE_BITS:
      index = FORMAT_BITS;
      break;
    case (G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS):
      index = FORMAT_BITS_IEC;
      break;
    default:
      g_assert_not_reached ();
    }


  if (size < formats[index][0].factor)
    {
      const char * units;

      if (index == FORMAT_BYTES || index == FORMAT_BYTES_IEC)
        {
          units = g_dngettext (GETTEXT_PACKAGE, "byte", "bytes", (guint) size);
        }
      else
        {
          units = g_dngettext (GETTEXT_PACKAGE, "bit", "bits", (guint) size);
        }

      if ((flags & G_FORMAT_SIZE_ONLY_UNIT) != 0)
        g_string_append (string, units);
      else if ((flags & G_FORMAT_SIZE_ONLY_VALUE) != 0)
        /* Translators: The "%u" is replaced with the size value, like "13"; it could
         * be part of "13 bytes", but only the number is requested this time. */
        g_string_printf (string, C_("format-size", "%u"), (guint) size);
      else
        {
          /* Translators: The first "%u" is replaced with the value, the "%s" with a unit of the value.
           * The order can be changed with "%$2s %$1u". An example: "13 bytes" */
          g_string_printf (string, C_("format-size", "%u %s"), (guint) size, units);
        }

      flags &= ~G_FORMAT_SIZE_LONG_FORMAT;
    }
  else
    {
      const gsize n = G_N_ELEMENTS (formats[index]);
      const gchar * units;
      gdouble value;
      gsize i;

      /*
       * Point the last format (the highest unit) by default
       * and then then scan all formats, starting with the 2nd one
       * because the 1st is already managed by with the plural form
       */
      const struct Format * f = &formats[index][n - 1];

      for (i = 1; i < n; i++)
        {
          if (size < formats[index][i].factor)
            {
              f = &formats[index][i - 1];
              break;
            }
        }

      units = _(f->string);
      value = (gdouble) size / (gdouble) f->factor;

      if ((flags & G_FORMAT_SIZE_ONLY_UNIT) != 0)
        g_string_append (string, units);
      else if ((flags & G_FORMAT_SIZE_ONLY_VALUE) != 0)
        /* Translators: The "%.1f" is replaced with the size value, like "13.0"; it could
         * be part of "13.0 MB", but only the number is requested this time. */
        g_string_printf (string, C_("format-size", "%.1f"), value);
      else
        {
          /* Translators: The first "%.1f" is replaced with the value, the "%s" with a unit of the value.
           * The order can be changed with "%$2s %$1.1f". Keep the no-break space between the value and
           * the unit symbol. An example: "13.0 MB" */
          g_string_printf (string, C_("format-size", "%.1f %s"), value, units);
        }
    }

  if (flags & G_FORMAT_SIZE_LONG_FORMAT)
    {
      /* First problem: we need to use the number of bytes to decide on
       * the plural form that is used for display, but the number of
       * bytes potentially exceeds the size of a guint (which is what
       * ngettext() takes).
       *
       * From a pragmatic standpoint, it seems that all known languages
       * base plural forms on one or both of the following:
       *
       *   - the lowest digits of the number
       *
       *   - if the number if greater than some small value
       *
       * Here's how we fake it:  Draw an arbitrary line at one thousand.
       * If the number is below that, then fine.  If it is above it,
       * then we take the modulus of the number by one thousand (in
       * order to keep the lowest digits) and add one thousand to that
       * (in order to ensure that 1001 is not treated the same as 1).
       */
      guint plural_form = size < 1000 ? size : size % 1000 + 1000;

      /* Second problem: we need to translate the string "%u byte/bit" and
       * "%u bytes/bits" for pluralisation, but the correct number format to
       * use for a gsize is different depending on which architecture
       * we're on.
       *
       * Solution: format the number separately and use "%s bytes/bits" on
       * all platforms.
       */
      const gchar *translated_format;
      gchar *formatted_number;

      if (index == FORMAT_BYTES || index == FORMAT_BYTES_IEC)
        {
          /* Translators: the %s in "%s bytes" will always be replaced by a number. */
          translated_format = g_dngettext (GETTEXT_PACKAGE, "%s byte", "%s bytes", plural_form);
        }
      else
        {
          /* Translators: the %s in "%s bits" will always be replaced by a number. */
          translated_format = g_dngettext (GETTEXT_PACKAGE, "%s bit", "%s bits", plural_form);
        }
      formatted_number = g_strdup_printf ("%'"G_GUINT64_FORMAT, size);

      g_string_append (string, " (");
      g_string_append_printf (string, translated_format, formatted_number);
      g_free (formatted_number);
      g_string_append (string, ")");
    }

  return g_string_free (string, FALSE);
}

#pragma GCC diagnostic pop

/**
 * g_format_size_for_display:
 * @size: a size in bytes
 *
 * Formats a size (for example the size of a file) into a human
 * readable string. Sizes are rounded to the nearest size prefix
 * (KB, MB, GB) and are displayed rounded to the nearest tenth.
 * E.g. the file size 3292528 bytes will be converted into the
 * string "3.1 MB".
 *
 * The prefix units base is 1024 (i.e. 1 KB is 1024 bytes).
 *
 * This string should be freed with g_free() when not needed any longer.
 *
 * Returns: (transfer full): a newly-allocated formatted string
 *   containing a human readable file size
 *
 * Since: 2.16
 *
 * Deprecated:2.30: This function is broken due to its use of SI
 *     suffixes to denote IEC units. Use g_format_size() instead.
 */
gchar *
g_format_size_for_display (goffset size)
{
  if (size < (goffset) KIBIBYTE_FACTOR)
    return g_strdup_printf (g_dngettext(GETTEXT_PACKAGE, "%u byte", "%u bytes",(guint) size), (guint) size);
  else
    {
      gdouble displayed_size;

      if (size < (goffset) MEBIBYTE_FACTOR)
        {
          displayed_size = (gdouble) size / (gdouble) KIBIBYTE_FACTOR;
          /* Translators: this is from the deprecated function g_format_size_for_display() which uses 'KB' to
           * mean 1024 bytes.  I am aware that 'KB' is not correct, but it has been preserved for reasons of
           * compatibility.  Users will not see this string unless a program is using this deprecated function.
           * Please translate as literally as possible.
           */
          return g_strdup_printf (_("%.1f KB"), displayed_size);
        }
      else if (size < (goffset) GIBIBYTE_FACTOR)
        {
          displayed_size = (gdouble) size / (gdouble) MEBIBYTE_FACTOR;
          return g_strdup_printf (_("%.1f MB"), displayed_size);
        }
      else if (size < (goffset) TEBIBYTE_FACTOR)
        {
          displayed_size = (gdouble) size / (gdouble) GIBIBYTE_FACTOR;
          return g_strdup_printf (_("%.1f GB"), displayed_size);
        }
      else if (size < (goffset) PEBIBYTE_FACTOR)
        {
          displayed_size = (gdouble) size / (gdouble) TEBIBYTE_FACTOR;
          return g_strdup_printf (_("%.1f TB"), displayed_size);
        }
      else if (size < (goffset) EXBIBYTE_FACTOR)
        {
          displayed_size = (gdouble) size / (gdouble) PEBIBYTE_FACTOR;
          return g_strdup_printf (_("%.1f PB"), displayed_size);
        }
      else
        {
          displayed_size = (gdouble) size / (gdouble) EXBIBYTE_FACTOR;
          return g_strdup_printf (_("%.1f EB"), displayed_size);
        }
    }
}

#if defined (G_OS_WIN32) && !defined (_WIN64)

/* Binary compatibility versions. Not for newly compiled code. */

_GLIB_EXTERN const gchar *g_get_user_name_utf8        (void);
_GLIB_EXTERN const gchar *g_get_real_name_utf8        (void);
_GLIB_EXTERN const gchar *g_get_home_dir_utf8         (void);
_GLIB_EXTERN const gchar *g_get_tmp_dir_utf8          (void);
_GLIB_EXTERN gchar       *g_find_program_in_path_utf8 (const gchar *program);

gchar *
g_find_program_in_path_utf8 (const gchar *program)
{
  return g_find_program_in_path (program);
}

const gchar *g_get_user_name_utf8 (void) { return g_get_user_name (); }
const gchar *g_get_real_name_utf8 (void) { return g_get_real_name (); }
const gchar *g_get_home_dir_utf8 (void) { return g_get_home_dir (); }
const gchar *g_get_tmp_dir_utf8 (void) { return g_get_tmp_dir (); }

#endif

/* Private API:
 *
 * Returns %TRUE if the current process was executed as setuid
 */ 
gboolean
g_check_setuid (void)
{
#if defined(HAVE_SYS_AUXV_H) && defined(HAVE_GETAUXVAL) && defined(AT_SECURE)
  unsigned long value;
  int errsv;

  errno = 0;
  value = getauxval (AT_SECURE);
  errsv = errno;

  /*
   * When running 32-bit user applications on a 64-bit kernel, reading of the
   * auxilliary vector can be unreliable, likely as a result of the vector not
   * being architecture agnostic.
   *
   * Whilst qemu-user appears to correct these structures depending on the
   * target architecture, the glibc based loader for armhf (ld-2.27.so) used by
   * the default toolchain included in the package respositories on Ubuntu
   * 18.04 does not appear to do so (presumably as the same binary is used on
   * both 32-bit and 64-bit kernels).
   *
   * Since an error results in everything stopping, we instead return TRUE
   * (indicating that the application was setuid). Hence we assume the worst
   * case scenario.
   */
  if (errsv)
    return TRUE;

  return value;
#elif defined(HAVE_ISSETUGID) && !defined(__BIONIC__)
  /* BSD: http://www.freebsd.org/cgi/man.cgi?query=issetugid&sektion=2 */

  /* Android had it in older versions but the new 64 bit ABI does not
   * have it anymore, and some versions of the 32 bit ABI neither.
   * https://code.google.com/p/android-developer-preview/issues/detail?id=168
   */
  return issetugid ();
#elif defined(G_OS_UNIX)
  uid_t ruid, euid, suid; /* Real, effective and saved user ID's */
  gid_t rgid, egid, sgid; /* Real, effective and saved group ID's */

  static gsize check_setuid_initialised;
  static gboolean is_setuid;

  if (g_once_init_enter (&check_setuid_initialised))
    {
#ifdef HAVE_GETRESUID
      /* These aren't in the header files, so we prototype them here.
       */
      int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
      int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);
      
      if (getresuid (&ruid, &euid, &suid) != 0 ||
          getresgid (&rgid, &egid, &sgid) != 0)
#endif /* HAVE_GETRESUID */
        {
          suid = ruid = getuid ();
          sgid = rgid = getgid ();
          euid = geteuid ();
          egid = getegid ();
        }

      is_setuid = (ruid != euid || ruid != suid ||
                   rgid != egid || rgid != sgid);

      g_once_init_leave (&check_setuid_initialised, 1);
    }
  return is_setuid;
#else
  return FALSE;
#endif
}

#ifdef G_OS_WIN32
/**
 * g_abort:
 *
 * A wrapper for the POSIX abort() function.
 *
 * On Windows it is a function that makes extra effort (including a call
 * to abort()) to ensure that a debugger-catchable exception is thrown
 * before the program terminates.
 *
 * See your C library manual for more details about abort().
 *
 * Since: 2.50
 */
void
g_abort (void)
{
  /* One call to break the debugger
   * We check if a debugger is actually attached to
   * avoid a windows error reporting popup window
   * when run in a test harness / on CI
   */
  if (IsDebuggerPresent ())
    DebugBreak ();
  /* One call in case CRT changes its abort() behaviour */
  abort ();
  /* And one call to bind them all and terminate the program for sure */
  ExitProcess (127);
}
#endif
