
#include <rz_types.h>
#include <rz_analysis.h>

static int bpf_analysis_op(RzAnalysis *a, RzAnalysisOp *op, ut64 addr, const ut8 *data, int len, RzAnalysisOpMask mask) {
	return 0;
}

int bpf_archinfo(RzAnalysis *a, RzAnalysisInfoType query) {
	const bool is_cbpf = a && a->bits == 32;

	switch (query) {
	case RZ_ANALYSIS_ARCHINFO_MIN_OP_SIZE:
		return 8;
	case RZ_ANALYSIS_ARCHINFO_MAX_OP_SIZE:
		return is_cbpf ? 8 : 16;
	case RZ_ANALYSIS_ARCHINFO_TEXT_ALIGN:
		return 8;
	case RZ_ANALYSIS_ARCHINFO_DATA_ALIGN:
		return is_cbpf ? 1 : 8;
	case RZ_ANALYSIS_ARCHINFO_CAN_USE_POINTERS:
		return is_cbpf ? false : true;
	default:
		return -1;
	}
}

RzAnalysisPlugin rz_analysis_plugin_bpf = {
	.name = "bpf",
	.desc = "cBPF & eBPF code analysis plugin",
	.license = "LGPL3",
	.arch = "bpf",
	.esil = false,
	.bits = 64,
	// .address_bits = address_bits,
	.op = &bpf_analysis_op,
	.archinfo = &bpf_archinfo
	// .get_reg_profile = &get_reg_profile,
	// .esil_init = rz_avr_esil_init,
	// .esil_fini = rz_avr_esil_fini,
	// .il_config = rz_avr_il_config,
	// .analysis_mask = analysis_mask_avr,
};

#ifndef RZ_PLUGIN_INCORE
RZ_API RzLibStruct rizin_plugin = {
	.type = RZ_LIB_TYPE_ANALYSIS,
	.data = &rz_analysis_plugin_bpf,
	.version = RZ_VERSION,
};
#endif
