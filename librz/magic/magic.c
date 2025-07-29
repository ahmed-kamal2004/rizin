// SPDX-FileCopyrightText: 2011-2019 pancake <pancake@nopcode.org>
// SPDX-License-Identifier: LGPL-3.0-only
/* $OpenBSD: magic.c,v 1.8 2009/10/27 23:59:37 deraadt Exp $ */

#include <rz_userconf.h>
#include <rz_magic.h>

RZ_LIB_VERSION(rz_magic);

#ifdef _MSC_VER
#include <io.h>
#include <sys\stat.h>
#define S_ISREG(m)  (((m)&S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m)&S_IFMT) == S_IFDIR)
#define S_IFIFO     (-1)
#define S_ISFIFO(m) (((m)&S_IFIFO) == S_IFIFO)
#define MAXPATHLEN  255
#endif

static void magic_node_free_rb(RBNode *node, void *user) {
	RzMagicLine *ml = (RzMagicLine *)container_of(node, RzMagicLine, rb);
	free(ml);
}

/* API */

// TODO: reinitialize all the time
RZ_API RzMagic *rz_magic_new(int flags) {
	RzMagic *ms = RZ_NEW0(RzMagic);
	if (!ms) {
		return NULL;
	}
	rz_magic_setflags(ms, flags);
	return ms;
}

RZ_API void rz_magic_free(RzMagic *ms) {
	if (ms) {
		free(ms->path);
		rz_rbtree_free(ms->magic_tree, magic_node_free_rb, NULL);
		free(ms);
	}
}

RZ_API bool rz_magic_load(RzMagic *ms, const char *magicfile) {
	FILE *file = open(magicfile, "r");
	if (magic_load(ms, file, ms->path, ms->flags)) {
		fclose(file);
		return true;
	}
	return false;
}

RZ_API const char *rz_magic_buffer(RzMagic *ms, const ut8 *buf, size_t nb) {
	// if (file_reset(ms) == -1) {
	// 	return NULL;
	// }
	// if (file_buffer(ms, -1, NULL, buf, nb) == -1) {
	// 	return NULL;
	// }
	// return file_getbuffer(ms);
}

RZ_API void rz_magic_setflags(RzMagic *ms, int flags) {
	if (ms) {
		ms->flags = flags;
	}
}