// SPDX-FileCopyrightText: 2009-2014 nibble <nibble.ds@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-only

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <rz_types.h>
#include <rz_lib.h>
#include <rz_util.h>
#include <rz_asm.h>
#include <common_gnu/disas-asm.h>
#include <common_gnu/mybfd.h>

typedef struct {
	struct disassemble_info disasm_obj;
	unsigned long Offset;
	RzStrBuf *buf_global;
	unsigned char bytes[4];
} SparcContext;

static int sparc_buffer_read_memory(bfd_vma memaddr, bfd_byte *myaddr, unsigned int length, struct disassemble_info *info, void *data) {
	SparcContext *ctx = (SparcContext *)data;
	int delta = (memaddr - ctx->Offset);
	if (delta < 0) {
		return -1; // disable backward reads
	}
	if ((delta + length) > 4) {
		return -1;
	}
	memcpy(myaddr, ctx->bytes, length);
	return 0;
}

static int symbol_at_address(bfd_vma addr, struct disassemble_info *info) {
	return 0;
}

static void memory_error_func(int status, bfd_vma memaddr, struct disassemble_info *info) {
	//--
}

static void generic_print_address_func(bfd_vma address, void *data, struct disassemble_info *info) {
	SparcContext *ctx = (SparcContext *)data;
	if (!ctx->buf_global) {
		return;
	}
	rz_strbuf_appendf(ctx->buf_global, "0x%08" PFMT64x, (ut64)address);
}

static int generic_fprintf_func(void *stream, void *data, const char *format, ...) {
	SparcContext *ctx = (SparcContext *)data;
	int ret;
	va_list ap;
	if (!ctx->buf_global || !format) {
		return 0;
	}
	va_start(ap, format);
	ret = rz_strbuf_vappendf(ctx->buf_global, format, ap);
	va_end(ap);
	return ret;
}

static int disassemble(RzAsm *a, RzAsmOp *op, const ut8 *buf, int len) {
	SparcContext *ctx = (SparcContext *)a->plugin_data;
	if (len < 4) {
		return -1;
	}
	ctx->buf_global = &op->buf_asm;
	ctx->Offset = a->pc;
	// disasm inverted
	ut32 newbuf = rz_swap_ut32(*(ut32 *)buf);
	memcpy(ctx->bytes, &newbuf, 4); // TODO handle thumb

	rz_strbuf_set(&op->buf_asm, "");
	/* prepare disassembler */
	memset(&ctx->disasm_obj, '\0', sizeof(struct disassemble_info));
	ctx->disasm_obj.buffer = ctx->bytes;
	ctx->disasm_obj.read_memory_func = &sparc_buffer_read_memory;
	ctx->disasm_obj.symbol_at_address_func = &symbol_at_address;
	ctx->disasm_obj.memory_error_func = &memory_error_func;
	ctx->disasm_obj.print_address_func = &generic_print_address_func;
	ctx->disasm_obj.endian = a->big_endian;
	ctx->disasm_obj.fprintf_func = &generic_fprintf_func;
	ctx->disasm_obj.stream = stdout;
	ctx->disasm_obj.mach = ((a->bits == 64)
			? bfd_mach_sparc_v9b
			: 0);

	op->size = print_insn_sparc((bfd_vma)ctx->Offset, &ctx->disasm_obj, ctx);

	if (!strncmp(rz_strbuf_get(&op->buf_asm), "unknown", 7)) {
		rz_asm_op_set_asm(op, "invalid");
	}
	if (op->size == -1) {
		rz_asm_op_set_asm(op, "(data)");
	}
	return op->size;
}

static bool init(void **user) {
	SparcContext *ctx = RZ_NEW0(SparcContext);
	rz_return_val_if_fail(ctx, false);
	*user = ctx;
	return true;
}

static bool fini(void *user) {
	SparcContext *ctx = (SparcContext *)user;
	if (ctx) {
		RZ_FREE(ctx);
	}
	return true;
}

RzAsmPlugin rz_asm_plugin_sparc_gnu = {
	.name = "sparc.gnu",
	.arch = "sparc",
	.bits = 32 | 64,
	.endian = RZ_SYS_ENDIAN_BIG | RZ_SYS_ENDIAN_LITTLE,
	.license = "GPL3",
	.desc = "Sun SPARC disassembler",
	.disassemble = &disassemble,
	.init = &init,
	.fini = &fini
};
