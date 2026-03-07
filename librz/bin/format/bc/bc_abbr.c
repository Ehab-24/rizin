// SPDX-License-Identifier: LGPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ehab-24 <ehabs1775@gmail.com>

#include "bc.h"

#define bc_abbr_op_new(x) \
	do { \
		BcAbbrOp *op = RZ_NEW0(BcAbbrOp); \
		if (op) { \
			op->type = x; \
		} \
		return op; \
	} while (0)

#define bc_abbr_op_new_val(x, y, z) \
	do { \
		BcAbbrOp *op = RZ_NEW0(BcAbbrOp); \
		if (op) { \
			op->type = x; \
			op->y = z; \
		} \
		return op; \
	} while (0)

RZ_IPI RZ_OWN BcAbbr *bc_abbr_new(RZ_OWN BcAbbrOps *ops) {
	BcAbbr *abbr = RZ_NEW0(BcAbbr);
	abbr->ops = ops;
	return abbr;
}

RZ_IPI void bc_abbr_free(BcAbbr *abbr) {
	if (abbr) {
		if (abbr->ops) {
			rz_pvector_free(abbr->ops);
		}
		free(abbr);
	}
}

RZ_IPI BcAbbrOp *bc_abbr_op_new_literal(ut64 val) {
	bc_abbr_op_new_val(ABBR_OP_LITERAL, literal, val);
}

RZ_IPI BcAbbrOp *bc_abbr_op_new_vbr(ut64 val) {
	bc_abbr_op_new_val(ABBR_OP_VBR, vbr, val);
}

RZ_IPI BcAbbrOp *bc_abbr_op_new_fixed(ut64 val) {
	bc_abbr_op_new_val(ABBR_OP_FIXED, fixed, val);
}

RZ_IPI BcAbbrOp *bc_abbr_op_new_array(RZ_NONNULL RZ_OWN BcAbbrOp *elem_type) {
	rz_return_val_if_fail(elem_type, NULL);
	bc_abbr_op_new_val(ABBR_OP_ARRAY, arr, elem_type);
}

RZ_IPI BcAbbrOp *bc_abbr_op_new_char6() {
	bc_abbr_op_new(ABBR_OP_CHAR6);
}

RZ_IPI BcAbbrOp *bc_abbr_op_new_blob() {
	bc_abbr_op_new(ABBR_OP_BLOB);
}

RZ_IPI void bc_abbr_op_free(BcAbbrOp *op) {
	if (op) {
		switch (op->type) {
		default:
			break;
		case ABBR_OP_ARRAY:
			bc_abbr_op_free(op->arr);
			break;
		}
		free(op);
	}
}
