// SPDX-License-Identifier: LGPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ehab-24 <ehabs1775@gmail.com>

#include "bc.h"
#include "rz_vector.h"

/**
 * \brief The current (top-most) scope
 */
static BcScope *scope(RZ_NONNULL BcParser *p) {
	rz_return_val_if_fail(p && p->scopes && !rz_stack_is_empty(p->scopes), NULL);
	return rz_stack_peek(p->scopes);
}

static void free_blk_abbr(BcAbbr *abbr) {
	if (abbr->scope == ABBR_SCOPE_BLOCK) {
		bc_abbr_free(abbr);
	}
}

static void free_infoblk_abbr(BcAbbr *abbr) {
	if (abbr->scope == ABBR_SCOPE_BLOCKINFO_BLOCK) {
		bc_abbr_free(abbr);
	}
}

static bool abbr_op_is_valid_array_elem_type(RZ_NONNULL BcAbbrOp *op) {
	switch (op->type) {
	default: return false;
	case ABBR_OP_VBR:
	case ABBR_OP_FIXED:
	case ABBR_OP_CHAR6:
		return true;
	}
}

static char char6_to_ascii(ut64 val) {
	return "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._"[val];
}

static BcAbbrOp *parse_abbr_op_literal(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf) {
	ut64 val;
	if (!bc_curs_read_vbr(p->curs, buf, 8, &val)) {
		return NULL;
	}
	return bc_abbr_op_new_literal(val);
}

static BcAbbrOp *parse_abbr_op_fixed(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf) {
	ut64 val;
	if (!bc_curs_read_vbr(p->curs, buf, 5, &val)) {
		return NULL;
	}
	return bc_abbr_op_new_fixed(val);
}

static BcAbbrOp *parse_abbr_op_vbr(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf) {
	ut64 val;
	if (!bc_curs_read_vbr(p->curs, buf, 5, &val)) {
		return NULL;
	}
	return bc_abbr_op_new_vbr(val);
}

static BcAbbrOp *parse_abbr_op(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf, size_t op_idx, ut64 numops);

static BcAbbrOp *parse_abbr_op_encoded(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf, size_t op_idx, ut64 numops) {
	ut64 enc_type;
	if (!bc_curs_read(p->curs, buf, 3, &enc_type)) {
		return NULL;
	}
	switch (enc_type) {
	default: return NULL;
	case ABBR_OP_ENC_FIXED: return parse_abbr_op_fixed(p, buf);
	case ABBR_OP_ENC_VBR: return parse_abbr_op_vbr(p, buf);
	case ABBR_OP_ENC_ARRAY: {
		if (op_idx != numops - 2) {
			return NULL;
		}
		BcAbbrOp *elem_type = parse_abbr_op(p, buf, op_idx + 1, numops);
		if (!elem_type || !abbr_op_is_valid_array_elem_type(elem_type)) {
			return NULL;
		}
		return bc_abbr_op_new_array(elem_type);
	}
	case ABBR_OP_ENC_CHAR6:
		return bc_abbr_op_new_char6();
	case ABBR_OP_ENC_BLOB:
		if (op_idx != numops - 1) {
			return NULL;
		}
		return bc_abbr_op_new_blob();
	}
}

static BcAbbrOp *parse_abbr_op(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf, size_t op_idx, ut64 numops) {
	ut64 op_type;
	if (!bc_curs_read(p->curs, buf, 1, &op_type)) {
		return NULL;
	}
	switch (op_type) {
	default:
		return NULL;
	case ABBR_OP_TYPE_LITERAL: return parse_abbr_op_literal(p, buf);
	case ABBR_OP_TYPE_ENCODED: return parse_abbr_op_encoded(p, buf, op_idx, numops);
	}
}

static BcAbbr *parse_abbr(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf) {
	ut64 numops;
	if (!bc_curs_read_vbr(p->curs, buf, 5, &numops) || numops < 1) {
		return false;
	}
	BcAbbrOps *ops = rz_pvector_new((RzPVectorFree)bc_abbr_op_free);
	if (!ops) {
		return false;
	}
	for (size_t i = 0; i < numops; ++i) {
		BcAbbrOp *op = parse_abbr_op(p, buf, i, numops);
		if (!op || !rz_pvector_push(ops, op)) {
			return false;
		}
		/// Break early since array ops will also conusume the next (and the last) op
		if (op->type == ABBR_OP_ARRAY) {
			break;
		}
	}
	return bc_abbr_new(ops);
}

/**
 * \brief Parse a single operand in an abbreviated record.
 */
static bool parse_abbr_record_op(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf, RZ_NONNULL const BcAbbrOp *op, RZ_NONNULL BcFields *fields) {
	rz_return_val_if_fail(p && buf && op && fields, false);
	switch (op->type) {
	default: return false;
	case ABBR_OP_LITERAL:
		if (!bc_fields_push(fields, op->literal)) {
			return false;
		}
		break;
	case ABBR_OP_VBR: {
		ut64 val;
		if (!bc_curs_read_vbr(p->curs, buf, op->vbr, &val)) {
			return false;
		}
		if (!bc_fields_push(fields, val)) {
			return false;
		}
		break;
	}
	case ABBR_OP_FIXED: {
		ut64 val;
		if (!bc_curs_read(p->curs, buf, op->fixed, &val)) {
			return false;
		}
		if (!bc_fields_push(fields, val)) {
			return false;
		}
		break;
	}
	case ABBR_OP_ARRAY: {
		if (!op->arr) {
			return false;
		}
		ut64 len;
		if (!bc_curs_read_vbr(p->curs, buf, 6, &len)) {
			return false;
		}
		for (size_t i = 0; i < len; ++i) {
			if (!parse_abbr_record_op(p, buf, op->arr, fields)) {
				return false;
			}
		}
		break;
	}
	case ABBR_OP_CHAR6: {
		ut64 val;
		if (!bc_curs_read(p->curs, buf, 6, &val)) {
			return false;
		}
		val = char6_to_ascii(val);
		if (!bc_fields_push(fields, val)) {
			return false;
		}
		break;
	}
	case ABBR_OP_BLOB: {
		ut64 len;
		if (!bc_curs_read_vbr(p->curs, buf, 6, &len)) {
			return false;
		}
		bc_curs_align(p->curs, 32);
		for (size_t i = 0; i < len; ++i) {
			ut8 byte;
			if (!bc_curs_read_8(p->curs, buf, &byte)) {
				return false;
			}
			if (!bc_fields_push(fields, (ut64)byte)) {
				return false;
			}
		}
		bc_curs_align(p->curs, 32);
		break;
	}
	}
	return true;
}

/**
 * \brief Parse an abbreviated record
 */
static bool parse_abbr_record(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf, ut64 abbr_id, RZ_OUT BcStreamEntry **ent) {
	const BcAbbr *abbr = bc_scopes_get_abbr(p->scopes, abbr_id);
	if (!abbr) {
		return false;
	}
	BcFields *fields = bc_fields_new(0);
	if (!fields) {
		return NULL;
	}
	void **it;
	rz_pvector_foreach (abbr->ops, it) {
		const BcAbbrOp *op = *it;
		if (!op) {
			return false;
		}
		if (!parse_abbr_record_op(p, buf, op, fields)) {
			return false;
		}
	}
	ut64 code;
	rz_vector_remove_at(fields, 0, &code);
	BcStreamEntry *stent = bc_stent_new_record_abbr(abbr_id, code, fields);
	if (!stent) {
		return false;
	}
	*ent = stent;
	return true;
}

/**
 * \return false on error
 */
static bool exit_blk(RZ_NONNULL BcParser *p, RZ_OUT BcStreamEntry **ent) {
	bc_curs_align(p->curs, BLOCK_ALIGNMENT);
	BcScope *s = bc_scopes_pop(p->scopes);
	if (!s) {
		return false;
	}
	if (bc_scope_is_infoblk(s)) {
		*ent = NULL;
	} else {
		*ent = bc_stent_new_endblk();
	}
	bc_scope_free(s);
	return true;
}

static bool enter_blk(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf, RZ_OUT BcStreamEntry **ent) {
	ut64 blk_id;
	if (!bc_curs_read_vbr(p->curs, buf, 8, &blk_id)) {
		return false;
	}
	ut64 id_width;
	if (!bc_curs_read_vbr(p->curs, buf, 4, &id_width)) {
		return false;
	}
	if (id_width < 1) {
		return false;
	}
	bc_curs_align(p->curs, BLOCK_ALIGNMENT);
	ut32 blk_wlen; ///< block length in 4-byte words
	if (!bc_curs_read_32(p->curs, buf, &blk_wlen)) {
		return false;
	}
	if (!bc_scopes_push(p->scopes, blk_id, id_width)) {
		return false;
	}

	bool found = false;
	const RzPVector *abbrs = ht_up_find(p->info_blks, blk_id, &found);
	if (found) {
		void **it;
		BcAbbr *abbr;
		rz_pvector_foreach (abbrs, it) {
			abbr = *it;
			if (!bc_scopes_add_abbr(p->scopes, abbr)) {
				return false;
			}
		}
	}

	if (bc_scopes_is_infoblk(p->scopes)) {
		*ent = NULL;
	} else {
		*ent = bc_stent_new_subblk(blk_id, blk_wlen * 4);
	}
	return true;
}

static bool define_abbr(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf) {
	BcAbbr *abbr = parse_abbr(p, buf);
	if (!abbr) {
		return NULL;
	}
	if (bc_scopes_is_infoblk(p->scopes)) {
		ut64 blk_id;
		if (!bc_scopes_blkinfo_id(p->scopes, &blk_id)) {
			return false;
		}

		HtUPKv *kv;
		RzPVector *abbrs = rz_pvector_new((RzPVectorFree)free_infoblk_abbr);
		HtRetCode ret = ht_up_insert_ex(p->info_blks, blk_id, abbrs, &kv);
		if (ret != HT_RC_INSERTED) {
			rz_pvector_free(abbrs);
		}
		return rz_pvector_push(kv->value, abbr) != NULL;
	}
	return bc_scopes_add_abbr(p->scopes, abbr);
}

static bool parse_unabbr(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf, RZ_OUT BcStreamEntry **ent) {
	if (bc_scopes_is_initial(p->scopes)) {
		return false;
	}
	ut64 code;
	if (!bc_curs_read_vbr(p->curs, buf, 6, &code)) {
		return false;
	}
	ut64 numops;
	if (!bc_curs_read_vbr(p->curs, buf, 6, &numops)) {
		return false;
	}
	BcFields *fields = bc_fields_new(numops);
	for (size_t i = 0; i < numops; ++i) {
		ut64 op;
		if (!bc_curs_read_vbr(p->curs, buf, 6, &op)) {
			return false;
		}
		if (!rz_vector_push(fields, &op)) {
			return false;
		}
	}

	if (bc_scopes_is_infoblk(p->scopes)) {
		switch (code) {
		default: return false;
		case BLOCKINFO_CODE_BLOCKNAME:
		case BLOCKINFO_CODE_SETRECORDNAME:
			/* skip */
			break;
		case BLOCKINFO_CODE_SETBID: {
			const ut64 *blk_id = rz_vector_index_ptr(fields, 0);
			if (!blk_id) {
				return false;
			}
			bc_scopes_set_infoblk_id(p->scopes, *blk_id);
			break;
		}
		}
		*ent = NULL;
	} else {
		*ent = bc_stent_new_record_unabbr(code, fields);
	}
	return true;
}

RZ_IPI RZ_OWN BcParser *bc_parser_new() {
	BcParser *p = RZ_NEW0(BcParser);
	p->curs = bc_curs_new();
	p->scopes = rz_stack_newf(4, (RzStackFree)free_blk_abbr);
	p->info_blks = ht_up_new(NULL, (HtUPFreeValue)rz_pvector_free);
	if (!bc_scopes_push(p->scopes, 0, INITIAL_ABBR_ID_WIDTH)) {
		bc_parser_free(p);
		return NULL;
	}
	return p;
}

RZ_IPI void bc_parser_free(BcParser *p) {
	if (p) {
		if (p->curs) {
			bc_curs_free(p->curs);
		}
		if (p->scopes) {
			rz_stack_free(p->scopes);
		}
		if (p->info_blks) {
			ht_up_free(p->info_blks);
		}
		free(p);
	}
}

RZ_IPI bool bc_parser_is_finished(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf) {
	return bc_curs_has_consumed(p->curs, rz_buf_size(buf) - 4);
}

/**
 * \return NULL on error
 */
RZ_IPI RZ_OWN BcStreamEntry *bc_parser_next(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf) {
	rz_return_val_if_fail(p && buf, NULL);

	if (bc_parser_is_finished(p, buf)) {
		return NULL;
	}
	BcScope *s = scope(p);
	if (!s) {
		return NULL;
	}
	ut64 id;
	if (!bc_curs_read(p->curs, buf, s->abbr_id_width, &id)) {
		return NULL;
	}

	BcStreamEntry *ent = NULL;
	switch (id) {
	case ABBR_ID_ENDBLOCK:
		if (!exit_blk(p, &ent)) {
			return NULL;
		}
		break;
	case ABBR_ID_ENTER_SUBBLOCK:
		if (!enter_blk(p, buf, &ent)) {
			return NULL;
		}
		break;
	case ABBR_ID_DEFINE_ABBR:
		if (!define_abbr(p, buf)) {
			return NULL;
		}
		break;
	case ABBR_ID_UNABBR_RECORD:
		if (!parse_unabbr(p, buf, &ent)) {
			return NULL;
		}
		break;
	default:
		if (!parse_abbr_record(p, buf, id, &ent)) {
			return NULL;
		}
		break;
	}
	return ent ? ent : bc_parser_next(p, buf);
}
