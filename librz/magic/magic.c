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

static bool magic_load_file(RZ_NONNULL RZ_BORROW RzMagic *m, const char *file_path) {
	rz_return_val_if_fail(m, false);

	int result;
	FILE *file = fopen(file_path, "r");
	if (!file) {
		return false;
	}
	result = magic_load(m, file, file_path, m->flags);
	fclose(file);
	return result;
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

RZ_API void rz_magic_free(RZ_NULLABLE RZ_OWN RzMagic *m) {
	if (m) {
		free(m->path);
		rz_rbtree_free(m->magic_tree, magic_node_free_rb, NULL);
		rz_rbtree_free(m->magic_named_tree, magic_node_free_rb, NULL);
		free(m);
	}
}

RZ_API bool rz_magic_load(RZ_NONNULL RZ_BORROW RzMagic *m, const char *magic_path) {
	rz_return_val_if_fail(m, false);

	if (m->path) {
		free(m->path);
	}
	m->path = rz_str_dup(magic_path);

	if (rz_file_is_regular(magic_path)) {
		return magic_load_file(m, magic_path);
	} else if (rz_file_is_directory(magic_path)) {
		RzList *files = rz_sys_dir(magic_path);
		if (!files) {
			return false;
		}
		RzListIter *it;
		const char *subname;
		char *filepath = NULL;
		RzStrBuf subpath;
		rz_strbuf_init(&subpath);
		bool result = true;
		rz_list_foreach (files, it, subname) {
			if (*subname == '.') {
				continue;
			}
			if (!rz_str_cmp(subname, "..", -1)) {
				continue;
			}
			filepath = rz_file_path_join(magic_path, subname);
			result &= magic_load_file(m, filepath);
			free(filepath);
		}
		rz_list_free(files);
		return result;
	} else {
		return false;
	}
}

RZ_API char *rz_magic_buffer(RZ_NONNULL const RzMagic *m, const ut8 *buf, size_t nb) {
	rz_return_val_if_fail(m, NULL);

	if (nb == 0) {
		return NULL;
	}
	return magic_test(m, buf, nb, m->flags);
}

RZ_API void rz_magic_setflags(RzMagic *m, int flags) {
	if (m) {
		m->flags = flags;
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
	RzMagicLine *child;
	TAILQ_FOREACH(child, &ml->children, entry) {
		TAILQ_REMOVE(&ml->children, child, entry);
		rz_magic_line_free(child);
	}
	free(ml->type_string);
	free(ml->result);
	free(ml->mimetype);
	free(ml);
}