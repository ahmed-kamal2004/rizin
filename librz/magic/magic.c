// SPDX-FileCopyrightText: 2011-2019 pancake <pancake@nopcode.org>
// SPDX-License-Identifier: LGPL-3.0-only
/* $OpenBSD: magic.c,v 1.8 2009/10/27 23:59:37 deraadt Exp $ */

#include <rz_userconf.h>
#include <rz_magic.h>
#include <rz_util.h>

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
	RzMagicLine *ml = container_of(node, RzMagicLine, rb);
	rz_magic_line_free(ml);
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

RZ_API bool rz_magic_load(RzMagic *ms, const char *magic_path) {
	DIR *dir = opendir(magic_path);
	bool result = true;
	struct dirent *entry;
	char *filepath = NULL;

	if (ms->path) {
		free(ms->path);
	}
	ms->path = rz_str_dup(magic_path);

	while ((entry = readdir(dir)) != NULL && result) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		filepath = rz_str_newf("%s/%s", magic_path, entry->d_name);

		FILE *file = fopen(filepath, "r");

		if (magic_load(ms, file, filepath, ms->flags)) {
			result &= true;
		} else {
			result &= false;
		}

		fclose(file);
	}
	free(filepath);
	return result;
}

RZ_API const char *rz_magic_buffer(RzMagic *ms, const ut8 *buf, size_t nb) {
	if (nb == 0) {
		return NULL;
	}
	return magic_test(ms, buf, nb, ms->flags);
}

RZ_API void rz_magic_setflags(RzMagic *ms, int flags) {
	if (ms) {
		ms->flags = flags;
	}
}

RZ_API RZ_OWN RzMagicLine *rz_magic_line_new(void) {
	RzMagicLine *ml = RZ_NEW0(RzMagicLine);
	return ml;
}

RZ_API void rz_magic_line_free(RZ_OWN RZ_NULLABLE RzMagicLine *ml) {
	if (!ml) {
		return;
	}
	free(ml->type_string);
	free(ml->result);
	free(ml->mimetype);
	free(ml);
}