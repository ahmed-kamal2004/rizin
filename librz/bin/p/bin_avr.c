// SPDX-FileCopyrightText: 2016-2017 pancake <pancake@nopcode.org>
// SPDX-License-Identifier: LGPL-3.0-only

#include <rz_bin.h>
#include <rz_lib.h>

#define CHECK4INSTR(b, instr, size) \
	if (!instr(b, 0) || \
		!instr((b), (size)) || \
		!instr((b), (size)*2) || \
		!instr((b), (size)*3)) { \
		return false; \
	}

#define CHECK3INSTR(b, instr, size) \
	if (!instr((b), (size)) || \
		!instr((b), (size)*2) || \
		!instr((b), (size)*3)) { \
		return false; \
	}

static bool rjmp(RzBuffer *b, ut64 addr) {
	ut8 tmp;
	if (!rz_buf_read8_at(b, addr + 1, &tmp)) {
		return false;
	}

	return (tmp & 0xf0) == 0xc0;
}

static bool jmp(RzBuffer *b, ut64 addr) {
	ut8 tmp;
	if (!rz_buf_read8_at(b, addr, &tmp)) {
		return false;
	}

	if (tmp != 0x0c) {
		return false;
	}

	return rz_buf_read8_at(b, addr + 1, &tmp) && tmp == 0x94;
}

static bool rjmp_dest(RzBuffer *b, ut64 addr, ut64 *result) {
	ut8 tmp;
	if (!rz_buf_read8_at(b, addr, &tmp)) {
		return false;
	}

	ut64 dst = 2 + addr + (ut64)tmp * 2;

	if (!rz_buf_read8_at(b, addr + 1, &tmp)) {
		return false;
	}

	*result = dst + ((((ut64)tmp & 0xf) * 2) << 8);

	return true;
}

static bool jmp_dest(RzBuffer *b, ut64 addr, ut64 *result) {
	ut8 tmp;
	if (!rz_buf_read8_at(b, addr + 2, &tmp)) {
		return false;
	}

	*result = tmp;

	if (!rz_buf_read8_at(b, addr + 3, &tmp)) {
		return false;
	}

	*result += (ut64)tmp << 8;
	*result *= 2;

	return true;
}

static bool check_buffer_rjmp(RzBuffer *b, RZ_OUT ut64 *entry) {
	CHECK3INSTR(b, rjmp, 4);
	ut64 dst;
	if (!rjmp_dest(b, 0, &dst)) {
		return false;
	}

	if (dst < 1 || dst > rz_buf_size(b)) {
		return false;
	}
	if (entry) {
		*entry = dst;
	}
	return true;
}

static bool check_buffer_jmp(RzBuffer *b, RZ_OUT ut64 *entry) {
	CHECK4INSTR(b, jmp, 4);
	ut64 dst;
	if (!jmp_dest(b, 0, &dst)) {
		return false;
	}

	if (dst < 1 || dst > rz_buf_size(b)) {
		return false;
	}
	if (entry) {
		*entry = dst;
	}
	return true;
}

static bool find_magic(RzBuffer *buf, RZ_OUT ut64 *entry) {
	if (rz_buf_size(buf) < 32) {
		return false;
	}
	if (!rjmp(buf, 0)) {
		return check_buffer_jmp(buf, entry);
	}
	return check_buffer_rjmp(buf, entry);
}

static bool check_buffer(RzBuffer *buf) {
	return find_magic(buf, NULL);
}

static bool load_buffer(RzBinFile *bf, RzBinObject *obj, RzBuffer *buf, Sdb *sdb) {
	ut64 *entry = RZ_NEW0(ut64);
	if (!entry) {
		return false;
	}
	*entry = UT64_MAX;
	if (find_magic(buf, entry)) {
		obj->bin_obj = entry;
		return true;
	}
	free(entry);
	return false;
}

static void destroy(RzBinFile *bf) {
	free(bf->o->bin_obj);
}

static RzBinInfo *info(RzBinFile *bf) {
	rz_return_val_if_fail(bf, NULL);
	RzBinInfo *bi = RZ_NEW0(RzBinInfo);
	if (bi) {
		bi->file = rz_str_dup(bf->file);
		bi->type = rz_str_dup("ROM");
		bi->machine = rz_str_dup("ATmel");
		bi->os = rz_str_dup("avr");
		bi->has_va = 0; // 1;
		bi->arch = rz_str_dup("avr");
		bi->bits = 8;
	}
	return bi;
}

static RzPVector /*<RzBinAddr *>*/ *entries(RzBinFile *bf) {
	RzPVector *ret;
	RzBinAddr *ptr = NULL;
	ut64 *entry = bf->o->bin_obj;
	if (*entry == UT64_MAX) {
		return false;
	}
	if (!(ret = rz_pvector_new(free))) {
		return NULL;
	}
	if ((ptr = RZ_NEW0(RzBinAddr))) {
		ut64 addr = *entry;
		ptr->vaddr = ptr->paddr = addr;
		rz_pvector_push(ret, ptr);
	}
	return ret;
}

static void addsym(RzPVector /*<RzBinSymbol *>*/ *ret, const char *name, ut64 addr) {
	RzBinSymbol *ptr = RZ_NEW0(RzBinSymbol);
	if (ptr) {
		ptr->name = rz_str_dup(name ? name : "");
		ptr->paddr = ptr->vaddr = addr;
		ptr->size = 0;
		ptr->ordinal = 0;
		rz_pvector_push(ret, ptr);
	}
}

static void addptr(RzPVector /*<RzBinSymbol *>*/ *ret, const char *name, ut64 addr, RzBuffer *b) {
	if (b && rjmp(b, 0)) {
		char tmpbuf[128];
		addsym(ret, rz_strf(tmpbuf, "vector.%s", name), addr);
		ut64 ptr_addr;
		if (rjmp_dest(b, addr, &ptr_addr)) {
			addsym(ret, rz_strf(tmpbuf, "syscall.%s", name), ptr_addr);
		}
	}
}

static RzPVector /*<RzBinSymbol *>*/ *symbols(RzBinFile *bf) {
	RzPVector *ret = NULL;
	RzBuffer *buf = bf->buf;

	if (!(ret = rz_pvector_new((RzPVectorFree)rz_bin_symbol_free))) {
		return NULL;
	}
	/* atmega8 */
	addptr(ret, "int0", 2, buf);
	addptr(ret, "int1", 4, buf);
	addptr(ret, "timer2cmp", 6, buf);
	addptr(ret, "timer2ovf", 8, buf);
	addptr(ret, "timer1capt", 10, buf);
	addptr(ret, "timer1cmpa", 12, buf);
	return ret;
}

static RzPVector /*<RzBinString *>*/ *strings(RzBinFile *bf) {
	// we dont want to find strings in avr bins because there are lot of false positives
	return rz_pvector_new((RzPVectorFree)rz_bin_string_free);
}

RzBinPlugin rz_bin_plugin_avr = {
	.name = "avr",
	.desc = "ATmel AVR MCUs",
	.license = "LGPL3",
	.author = "pancake",
	.load_buffer = &load_buffer,
	.destroy = &destroy,
	.entries = &entries,
	.symbols = &symbols,
	.check_buffer = &check_buffer,
	.info = &info,
	.strings = &strings,
};

#ifndef RZ_PLUGIN_INCORE
RZ_API RzLibStruct rizin_plugin = {
	.type = RZ_LIB_TYPE_BIN,
	.data = &rz_bin_plugin_avr,
	.version = RZ_VERSION
};
#endif
