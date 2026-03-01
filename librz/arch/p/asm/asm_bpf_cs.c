#include <rz_types.h>
#include <rz_asm.h>
#include <stdio.h>
#include "capstone.h"
#include "cs_helper.h"

CAPSTONE_DEFINE_PLUGIN_FUNCTIONS(bpf_asm)

static int bpf_disassemble(RzAsm *a, RzAsmOp *op, const ut8 *buf, int len) {
	CapstoneContext *ctx = (CapstoneContext *)a->plugin_data;
	int ret = -1;
	cs_mode mode = a->bits == 64 ? CS_MODE_BPF_EXTENDED : CS_MODE_BPF_CLASSIC;
	mode |= a->big_endian ? CS_MODE_BIG_ENDIAN : CS_MODE_LITTLE_ENDIAN;
	op->size = 8;

	if (ctx->omode != mode || ctx->obits != a->bits) {
		cs_close(&ctx->handle);
		ctx->omode = mode;
		ctx->obits = a->bits;
	}
	if (!ctx->handle) {
		ret = cs_open(CS_ARCH_BPF, mode, &ctx->handle);
		if (ret != CS_ERR_OK) {
			goto fini;
		}
	}

	cs_insn *insn = NULL;

	size_t n = cs_disasm(ctx->handle, buf, len, a->pc, 1, &insn);
	if (n < 1) {
		rz_asm_op_set_asm(op, "invalid");
		goto fini;
	}

	op->size = insn->size;
	rz_asm_op_setf_asm(op, "%s%s%s", insn->mnemonic, insn->op_str[0] ? " " : "", insn->op_str);
	cs_free(insn, n);
fini:
	return op->size;
}

RzAsmPlugin rz_asm_plugin_bpf = {
	.name = "bpf",
	.arch = "bpf",
	.license = "LGPL3",
	.desc = "Capstone based disassembler for cBPF and eBPF bytecode",
	.bits = 32 | 64,
	.init = &bpf_asm_init,
	.fini = &bpf_asm_fini,
	.mnemonics = &bpf_asm_mnemonics,
	.disassemble = &bpf_disassemble,
};

#ifndef RZ_PLUGIN_INCORE
RZ_API RzLibStruct rizin_plugin = {
	.type = RZ_LIB_TYPE_ASM,
	.data = rz_asm_plugin_bpf,
	.version = RZ_VERSION
};
#endif
