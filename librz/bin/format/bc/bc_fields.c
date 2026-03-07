// SPDX-License-Identifier: LGPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ehab-24 <ehabs1775@gmail.com>

#include "bc.h"

RZ_IPI BcFields *bc_fields_new(size_t capacity) {
	BcFields *fields = rz_vector_new(sizeof(ut64), NULL, NULL);
	if (fields) {
		rz_vector_reserve(fields, capacity);
	}
	return fields;
}

RZ_IPI bool bc_fields_push(RZ_NONNULL BcFields *fields, ut64 field) {
	return rz_vector_push(fields, &field) != NULL;
}

RZ_IPI void bc_fields_free(BcFields *fields) {
	rz_vector_free(fields);
}
