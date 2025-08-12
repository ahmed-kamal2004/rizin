/* $OpenBSD: magic-common.c,v 1.3 2015/08/11 22:29:25 nicm Exp $ */

/*
 * Copyright (c) 2015 Nicholas Marriott <nicm@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <rz_magic.h>

RZ_API char *magic_strtoull(RZ_NONNULL const char *s, RZ_NONNULL ut64 *u) {
	rz_return_val_if_fail(s && u, NULL);

	char *endptr;

	if (*s == '-' || *s == '\0')
		return (NULL);

	errno = 0;
	*u = strtoull(s, &endptr, 0);
	if (endptr == s)
		*u = strtoull(s, &endptr, 16);
	if (errno == ERANGE && *u == ULLONG_MAX)
		return (NULL);
	if (*endptr == 'L')
		endptr++;
	return (endptr);
}

RZ_API char *magic_strtoll(RZ_NONNULL const char *s, RZ_NONNULL int64_t *i) {
	rz_return_val_if_fail(s && i, NULL);

	char *endptr;

	if (*s == '\0')
		return (NULL);

	errno = 0;
	*i = strtoll(s, &endptr, 0);
	if (endptr == s)
		*i = strtoll(s, &endptr, 16);
	if (errno == ERANGE && *i == LLONG_MAX)
		return (NULL);
	if (*endptr == 'L')
		endptr++;
	return (endptr);
}

RZ_API void magic_vwarnm(RZ_NONNULL const RzMagic *m, ut32 line, RZ_NONNULL const char *fmt, va_list ap) {
	rz_return_if_fail(m && fmt);

	char *msg;

	if (!m->flags)
		return;

	if (vasprintf(&msg, fmt, ap) == -1)
		return;
	fprintf(stderr, "%s:%u: %s\n", m->path, line, msg);
	free(msg);
}

RZ_API void magic_warnm(RZ_NONNULL const RzMagic *m, ut32 line, RZ_NONNULL const char *fmt, ...) {
	rz_return_if_fail(m && fmt);

	va_list ap;

	va_start(ap, fmt);
	magic_vwarnm(m, line, fmt, ap);
	va_end(ap);
}

RZ_API void magic_warn(RZ_NONNULL const RzMagicLine *ml, RZ_NONNULL const char *fmt, ...) {
	rz_return_if_fail(ml && fmt);

	va_list ap;

	va_start(ap, fmt);
	magic_vwarnm(ml->root, ml->line, fmt, ap);
	va_end(ap);
}
