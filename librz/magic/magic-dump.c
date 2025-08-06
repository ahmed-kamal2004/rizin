/* $OpenBSD: magic-dump.c,v 1.2 2016/05/01 10:56:03 nicm Exp $ */

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

#include <sys/types.h>
#include <rz_magic.h>
#include <rz_util.h>

#include <stdio.h>

static void
magic_dump_line(RZ_NONNULL const RzMagicLine *ml, ut32 depth) {
	rz_return_if_fail(ml);

	RzMagicLine *child;
	ut32 i;

	printf("%u", ml->line);
	for (i = 0; i < depth; i++)
		printf(">");
	if (ml->name != NULL)
		printf(" %s %s\n", ml->type_string, ml->name);
	else {
		printf(" %s/%s%s%s%s [%u]%s\n", ml->type_string,
			ml->result == NULL ? "" : ml->result,
			ml->mimetype == NULL ? "" : " (",
			ml->mimetype == NULL ? "" : ml->mimetype,
			ml->mimetype == NULL ? "" : ")",
			ml->strength, ml->text ? " (text)" : "");
	}

	TAILQ_FOREACH(child, &ml->children, entry)
	magic_dump_line(child, depth + 1);
}

void magic_dump(RZ_NONNULL const RzMagic *m) {
	rz_return_if_fail(m);

	RBIter it;
	RzMagicLine *ml;

	rz_rbtree_foreach (m->magic_tree, it, ml, RzMagicLine, rb) {
		magic_dump_line(ml, 0);
	}
	rz_rbtree_foreach (m->magic_named_tree, it, ml, RzMagicLine, rb) {
		magic_dump_line(ml, 0);
	}
}
