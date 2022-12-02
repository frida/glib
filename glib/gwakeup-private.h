/* gwakeup-private.h
 *
 * Copyright (C) 2022 Håvard Sørbø <havard@hsorbo.no>
 * Copyright (C) 2022 Ole André Vadla Ravnås <oleavr@nowsecure.com>
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

#ifndef __G_WAKEUP_PRIVATE_H__
#define __G_WAKEUP_PRIVATE_H__

#include "gwakeup.h"

G_BEGIN_DECLS

void _g_wakeup_kqueue_realize   (GWakeup *wakeup, gint kq);
void _g_wakeup_kqueue_unrealize (GWakeup *wakeup);

G_END_DECLS

#endif
