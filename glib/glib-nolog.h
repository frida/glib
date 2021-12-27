/*
 * Copyright © 2021 Ole André Vadla Ravnås <oleavr@gmail.com>
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

#include "gtestutils.h"

#undef  g_error
#define g_error(...) g_assert_not_reached ()
#undef  g_critical
#define g_critical(...) g_assert_not_reached ()
#undef  g_warning
#define g_warning(...)
#undef  g_debug
#define g_debug(...)

#undef  g_warn_if_fail
#define g_warn_if_fail(...)
