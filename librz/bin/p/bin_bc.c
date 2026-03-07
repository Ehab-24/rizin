// SPDX-License-Identifier: LGPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ehab-24 <ehabs1775@gmail.com>

#include <rz_bin.h>
#include <rz_lib.h>
#include "bc/bc.h"
#include "rz_util/rz_buf.h"

static bool check_buffer(RzBuffer *buf) {
	ut32 magic;
	if (!rz_buf_read_le32(buf, &magic)) {
		return false;
	}
	return magic == LLVM_BITCODE_MAGIC;
}

static void print_stream_entry(RZ_NONNULL const BcStreamEntry *stent, RZ_INOUT size_t *scope_depth) {
	switch (stent->type) {
	case STREAM_ENTRY_ENDBLOCK:
		*scope_depth -= 1;
		for (size_t i = 0; i < *scope_depth; ++i) {
			printf("\t");
		}
		printf("}\n");
		break;
	case STREAM_ENTRY_SUBBLOCK:
		for (size_t i = 0; i < *scope_depth; ++i) {
			printf("\t");
		}
		printf("BLOCK %llu {\n", stent->block.id);
		*scope_depth += 1;
		break;
	case STREAM_ENTRY_RECORD: {
		for (size_t i = 0; i < *scope_depth; ++i) {
			printf("\t");
		}
		printf("RECORD { code: %llu, fields: [", stent->record.code);
		void *it;
		size_t i = 0;
		rz_vector_enumerate (stent->record.fields, it, i) {
			ut64 val = *(ut64 *)it;
			printf("%llu", val);
			if (i < rz_vector_len(stent->record.fields) - 1) {
				printf(", ");
			}
		}
		printf("] }\n");
		break;
	}
	}
}

static bool load_buffer(RzBinFile *bf, RzBinObject *obj, RzBuffer *buf, Sdb *sdb) {
	BcParser *p = bc_parser_new();
	if (!p) {
		return false;
	}
	ut32 magic;
	if (!bc_curs_read_32(p->curs, buf, &magic)) {
		return false;
	}

	size_t scope_depth = 0;
	while (true) {
		BcStreamEntry *stent = bc_parser_next(p, buf);
		if (!stent) {
			break;
		}
		print_stream_entry(stent, &scope_depth);
		bc_stent_free(stent);
	}

	const bool ret = bc_parser_is_finished(p, buf);
	bc_parser_free(p);
	return ret;
}

static void destroy(RzBinFile *bf) {
	/* empty */
}

RzBinPlugin rz_bin_plugin_bc = {
	.name = "llvm-bc",
	.desc = "LLVM Bitcode",
	.license = "LGPL3",
	.author = "Ehab-24",
	.load_buffer = &load_buffer,
	.destroy = &destroy,
	.check_buffer = &check_buffer,
};

#ifndef RZ_PLUGIN_INCORE
RZ_API RzLibStruct rizin_plugin = {
	.type = RZ_LIB_TYPE_BIN,
	.data = &rz_bin_plugin_bc,
	.version = RZ_VERSION
};
#endif
