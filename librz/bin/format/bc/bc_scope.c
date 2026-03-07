// SPDX-License-Identifier: LGPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ehab-24 <ehabs1775@gmail.com>

#include "bc.h"
#include "rz_vector.h"

#define SCOPE(x) \
	BcScope *x = rz_stack_peek(s); \
	if (!x) { \
		return false; \
	}

RZ_IPI bool bc_scope_is_infoblk(RZ_NONNULL BcScope *scp) {
	return !scp->is_initial && scp->blk_id == BLOCK_ID_BLOCKINFO_ID;
}

RZ_IPI bool bc_scopes_push(RZ_NONNULL BcScopeStack *s, ut64 blk_id, ut64 abbr_id_width) {
	BcScope *scp = bc_scope_new(blk_id, abbr_id_width);
	if (scp) {
		rz_stack_push(s, scp);
		return true;
	}
	return false;
}

RZ_IPI BcScope *bc_scopes_pop(RZ_NONNULL BcScopeStack *s) {
	if (rz_stack_size(s) > 1) {
		BcScope *scp = rz_stack_pop(s); /// Do not allow to pop the initial scope
		return scp;
	}
	return NULL;
}

RZ_IPI bool bc_scopes_blkinfo_id(RZ_NONNULL BcScopeStack *s, RZ_OUT ut64 *id) {
	SCOPE(scp);
	if (!scp->is_initial && scp->info_blk_id != UT64_MAX) {
		*id = scp->info_blk_id;
		return true;
	}
	return false;
}

RZ_IPI bool bc_scopes_is_infoblk(RZ_NONNULL BcScopeStack *s) {
	SCOPE(scp);
	return bc_scope_is_infoblk(scp);
}

RZ_IPI bool bc_scopes_is_initial(RZ_NONNULL BcScopeStack *s) {
	SCOPE(scp);
	return scp->is_initial;
}

RZ_IPI bool bc_scopes_set_infoblk_id(RZ_NONNULL BcScopeStack *s, ut64 id) {
	SCOPE(scp);
	if (!scp->is_initial) {
		scp->info_blk_id = id;
		return true;
	}
	return false;
}

RZ_IPI bool bc_scopes_find_abbr(RZ_NONNULL BcScopeStack *s, ut64 id, RZ_OUT BcAbbr **abbr) {
	SCOPE(scp);
	if (scp->is_initial) {
		return false;
	}
	const size_t idx = id - FIRST_APPLICATION_ABBR_ID;
	BcAbbr *elem = rz_pvector_at(scp->abbrs, idx);
	if (!elem) {
		return false;
	}
	*abbr = elem;
	return true;
}

RZ_IPI bool bc_scopes_add_abbr(RZ_NONNULL BcScopeStack *s, RZ_NONNULL BcAbbr *abbr) {
	SCOPE(scp);
	if (scp->is_initial) {
		return false;
	}
	return rz_pvector_push(scp->abbrs, abbr) != NULL;
}

RZ_IPI RZ_NULLABLE BcAbbr *bc_scopes_get_abbr(RZ_NONNULL BcScopeStack *s, ut64 abbr_id) {
	SCOPE(scp);
	if (scp->is_initial) {
		return NULL;
	}
	const size_t idx = abbr_id - FIRST_APPLICATION_ABBR_ID;
	return rz_pvector_at(scp->abbrs, idx);
}

RZ_IPI BcScope *bc_scope_new(ut64 blk_id, ut64 abbr_id_width) {
	BcScope *scp = RZ_NEW0(BcScope);
	scp->is_initial = false;
	scp->blk_id = blk_id;
	scp->abbr_id_width = abbr_id_width;
	scp->info_blk_id = UT64_MAX;
	scp->abbrs = rz_pvector_new(NULL); /// Underlying elements will be freed by \ref bitcode_parser_t
	return scp;
}

RZ_IPI void bc_scope_free(BcScope *scp) {
	if (scp) {
		rz_pvector_free(scp->abbrs);
		free(scp);
	}
}
