// SPDX-License-Identifier: LGPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ehab-24 <ehabs1775@gmail.com>

#include "bc.h"

#define STENT_NEW(x) \
	BcStreamEntry *x = RZ_NEW0(BcStreamEntry); \
	if (!x) { \
		return NULL; \
	}

RZ_IPI RZ_OWN BcStreamEntry *bc_stent_new_endblk() {
	STENT_NEW(stent);
	stent->type = STREAM_ENTRY_ENDBLOCK;
	return stent;
}

RZ_IPI RZ_OWN BcStreamEntry *bc_stent_new_subblk(ut64 blk_id, ut64 blk_len) {
	STENT_NEW(stent);
	stent->type = STREAM_ENTRY_SUBBLOCK;
	stent->block.id = blk_id;
	stent->block.len = blk_len;
	return stent;
}

RZ_IPI RZ_OWN BcStreamEntry *bc_stent_new_record_abbr(ut64 abbr_id, ut64 code, RZ_NONNULL RZ_OWN RzVector * /* <ut64> */ fields) {
	STENT_NEW(stent);
	stent->type = STREAM_ENTRY_RECORD;
	stent->record.abbr_id = abbr_id;
	stent->record.code = code;
	stent->record.fields = fields;
	return stent;
}

RZ_IPI RZ_OWN BcStreamEntry *bc_stent_new_record_unabbr(ut64 code, RZ_NONNULL RZ_OWN RzVector * /* <ut64> */ fields) {
	STENT_NEW(stent);
	stent->type = STREAM_ENTRY_RECORD;
	stent->record.abbr_id = UT64_MAX;
	stent->record.code = code;
	stent->record.fields = fields;
	return stent;
}

RZ_IPI void bc_stent_free(BcStreamEntry *stent) {
	if (!stent) {
		return;
	}
	switch (stent->type) {
	default:
		break;
	case STREAM_ENTRY_RECORD:
		rz_vector_free(stent->record.fields);
		break;
	}
	free(stent);
}
