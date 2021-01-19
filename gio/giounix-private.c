/*
 * Copyright © 2021 Ole André Vadla Ravnås
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

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "giounix-private.h"

#define G_TEMP_FAILURE_RETRY(expression)      \
  ({                                          \
    gssize __result;                          \
                                              \
    do                                        \
      __result = (gssize) (expression);       \
    while (__result == -1 && errno == EINTR); \
                                              \
    __result;                                 \
  })

gboolean
_g_fd_is_pollable (int fd)
{
  struct stat st;
  static gsize initialized;
  static gboolean have_nulldev;
  static struct stat ndst;

  if (G_TEMP_FAILURE_RETRY (fstat (fd, &st)) == -1)
    return TRUE;

  if (S_ISREG (st.st_mode))
    return FALSE;

  if (g_once_init_enter (&initialized))
    {
      int ndfd;

      ndfd = G_TEMP_FAILURE_RETRY (open ("/dev/null", O_RDONLY, 0));

      if (ndfd != -1)
        {
          have_nulldev = G_TEMP_FAILURE_RETRY (fstat (ndfd, &ndst)) != -1;

          G_TEMP_FAILURE_RETRY (close (ndfd));
        }

      g_once_init_leave (&initialized, TRUE);
    }

  if (have_nulldev && st.st_dev == ndst.st_dev && st.st_ino == ndst.st_ino)
    return FALSE;

  return TRUE;
}
