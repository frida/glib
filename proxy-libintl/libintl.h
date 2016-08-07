/*
 * Copyright (C) 2008 Tor Lillqvist
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.txt.  If
 * not, write to the Free Software Foundation, Inc., 51 Franklin
 * Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _LIBINTL_H
#define _LIBINTL_H      1

#include <locale.h>

#ifndef LC_MESSAGES
# define LC_MESSAGES 1729       /* Use same value as in GNU gettext */
#endif

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
# define PROXY_LIBINTL_GNUC_FORMAT(arg_idx) __attribute__((__format_arg__(arg_idx)))
#else
# define PROXY_LIBINTL_GNUC_FORMAT(arg_idx)
#endif

#define gettext g_libintl_gettext
#define dgettext g_libintl_dgettext
#define dcgettext g_libintl_dcgettext
#define ngettext g_libintl_ngettext
#define dngettext g_libintl_dngettext
#define dcngettext g_libintl_dcngettext
#define textdomain g_libintl_textdomain
#define bindtextdomain g_libintl_bindtextdomain
#define bind_textdomain_codeset g_libintl_bind_textdomain_codeset

#ifdef __cplusplus
extern "C" {
#endif

extern char *g_libintl_gettext (const char *msgid) PROXY_LIBINTL_GNUC_FORMAT (1);

extern char *g_libintl_dgettext (const char *domainname,
				 const char *msgid) PROXY_LIBINTL_GNUC_FORMAT (2);

extern char *g_libintl_dcgettext (const char *domainname,
			const char *msgid,
			int         category) PROXY_LIBINTL_GNUC_FORMAT (2);

extern char *g_libintl_ngettext (const char       *msgid1,
				 const char       *msgid2,
				 unsigned long int n) PROXY_LIBINTL_GNUC_FORMAT (1) PROXY_LIBINTL_GNUC_FORMAT (2);

extern char *g_libintl_dngettext (const char       *domainname,
				  const char       *msgid1,
				  const char       *msgid2,
				  unsigned long int n) PROXY_LIBINTL_GNUC_FORMAT (2) PROXY_LIBINTL_GNUC_FORMAT (3);

extern char *g_libintl_dcngettext (const char       *domainname,
				   const char       *msgid1,
				   const char       *msgid2,
				   unsigned long int n,
				   int               category) PROXY_LIBINTL_GNUC_FORMAT (2) PROXY_LIBINTL_GNUC_FORMAT (3);

extern char *g_libintl_textdomain (const char *domainname);

extern char *g_libintl_bindtextdomain (const char *domainname,
				       const char *dirname);

extern char *g_libintl_bind_textdomain_codeset (const char *domainname,
						const char *codeset);

#ifdef __cplusplus
}
#endif

#endif /* _LIBINTL_H */
