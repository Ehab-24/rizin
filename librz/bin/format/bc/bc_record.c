// SPDX-License-Identifier: LGPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ehab-24 <ehabs1775@gmail.com>

#include "bc.h"

#define RECORD_NEW(x) \
	BcRecord *x = RZ_NEW0(BcRecord); \
	if (!x) { \
		return NULL; \
	}

RZ_IPI RZ_OWN BcRecord *bc_record_new_unabbr(ut64 code, RZ_NONNULL RZ_OWN RzVector /* <ut64> */ *fields) {
	RECORD_NEW(record);
	record->abbr_id = UT64_MAX;
	record->code = code;
	record->fields = fields;
	return record;
}

RZ_IPI RZ_OWN BcRecord *bc_record_new_abbr(ut64 abbr_id, ut64 code, RZ_NONNULL RZ_OWN RzVector /* <ut64> */ *fields) {
	RECORD_NEW(record);
	record->abbr_id = abbr_id;
	record->code = code;
	record->fields = fields;
	return record;
}

RZ_IPI void bc_record_free(BcRecord *record) {
	if (record) {
		rz_vector_free(record->fields);
		free(record);
	}
}
