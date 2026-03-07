// SPDX-License-Identifier: LGPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ehab-24 <ehabs1775@gmail.com>

#include <rz_types.h>
#include <rz_util/ht_up.h>
#include <rz_util/rz_assert.h>
#include <rz_util/rz_buf.h>
#include <rz_util/rz_stack.h>
#include <rz_vector.h>

#define LLVM_BITCODE_MAGIC        0xDEC04342
#define INITIAL_ABBR_ID_WIDTH     2
#define BLOCK_SIZE                8
#define BLOCK_SIZE_BITS           BLOCK_SIZE * 8
#define MAX_VBR_BITS              32
#define BLOCK_ALIGNMENT           32
#define FIRST_APPLICATION_ABBR_ID 4

#define BLOCK_ID_BLOCKINFO_ID 0
#define BLOCK_ID_RESERVED1    1
#define BLOCK_ID_RESERVED2    2
#define BLOCK_ID_RESERVED3    3
#define BLOCK_ID_RESERVED4    4
#define BLOCK_ID_RESERVED5    5
#define BLOCK_ID_RESERVED6    6
#define BLOCK_ID_RESERVED7    7

typedef struct bitcode_scope_t BcScope;
typedef RzStack BcScopeStack /* <BcScope *> */;
typedef RzVector /* <ut64> */ BcFields;
typedef RzPVector /* <BcAbbrOp *> */ BcAbbrOps;

typedef struct bit_cursor_t {
	ut32 rbytes; ///< number of read bytes
	ut32 rbits; ///< number of remaining bits in the current block
	ut64 cblk; ///< current block containing at most \p BLOCK_SIZE_BITS bits
} BcCursor;

typedef struct bitcode_parser_t {
	BcCursor *curs;
	BcScopeStack /* <BcScope *> */ *scopes;
	HtUP /* <ut64, RzPVector<BcAbbr *>> */ *info_blks; ///< mapping (block id => BLOCKINFO block)
} BcParser;

enum {
	ABBR_ID_ENDBLOCK,
	ABBR_ID_ENTER_SUBBLOCK,
	ABBR_ID_DEFINE_ABBR,
	ABBR_ID_UNABBR_RECORD
};

enum {
	ABBR_OP_TYPE_ENCODED,
	ABBR_OP_TYPE_LITERAL
};

enum {
	BLOCKINFO_CODE_SETBID = 1,
	BLOCKINFO_CODE_BLOCKNAME,
	BLOCKINFO_CODE_SETRECORDNAME,
};

enum {
	ABBR_OP_ENC_FIXED = 1,
	ABBR_OP_ENC_VBR,
	ABBR_OP_ENC_ARRAY,
	ABBR_OP_ENC_CHAR6,
	ABBR_OP_ENC_BLOB
};

typedef enum {
	STREAM_ENTRY_ENDBLOCK,
	STREAM_ENTRY_SUBBLOCK,
	STREAM_ENTRY_RECORD
} BcStreamEntryType;

typedef struct {
	ut64 id;
	ut64 len; ///< in bytes
} BcBlock;

typedef struct {
	ut64 abbr_id; ///< UT64_MAX represents an unset id
	ut64 code;
	BcFields *fields;
} BcRecord;

typedef struct bitcode_stream_entry_t {
	BcStreamEntryType type;
	union {
		BcBlock block;
		BcRecord record;
	};
} BcStreamEntry;

struct bitcode_scope_t {
	ut64 abbr_id_width;
	ut64 blk_id;
	ut64 info_blk_id; ///< UT64_MAX represents an unset id
	RzPVector /* <BcAbbr *> */ *abbrs;
	bool is_initial;
};

typedef enum {
	ABBR_SCOPE_BLOCKINFO_BLOCK,
	ABBR_SCOPE_BLOCK
} BcAbbrScope;

typedef struct {
	BcAbbrScope scope;
	BcAbbrOps *ops;
} BcAbbr;

typedef enum {
	ABBR_OP_LITERAL,
	ABBR_OP_VBR,
	ABBR_OP_FIXED,
	ABBR_OP_ARRAY,
	ABBR_OP_CHAR6,
	ABBR_OP_BLOB
} BcAbbrOpType;

typedef struct abbr_op_t {
	BcAbbrOpType type;
	union {
		ut64 literal; ///< The literal value
		ut64 vbr; ///< The width of VBR blocks
		ut64 fixed; ///< The exact (fixed) width of the field
		struct abbr_op_t *arr; ///< The type of array elements. Only FIXED, VBR and CHAR6 are allowed.
	};
} BcAbbrOp;

RZ_IPI RZ_OWN BcCursor *bc_curs_new();
RZ_IPI void bc_curs_free(BcCursor *c);
RZ_IPI bool bc_curs_has_consumed(RZ_NONNULL const BcCursor *c, size_t nbytes);
RZ_IPI size_t bc_curs_tell_bit(RZ_NONNULL const BcCursor *c);
RZ_IPI size_t bc_curs_tell_byte(RZ_NONNULL const BcCursor *c);
RZ_IPI void bc_curs_align(RZ_NONNULL BcCursor *c, ut32 width);
RZ_IPI bool bc_curs_read(RZ_NONNULL BcCursor *c, RZ_NONNULL RzBuffer *buf, ut32 n, RZ_OUT ut64 *bits);
RZ_IPI bool bc_curs_read_vbr(RZ_NONNULL BcCursor *c, RZ_NONNULL RzBuffer *buf, ut32 width, RZ_OUT ut64 *bits);
RZ_IPI bool bc_curs_read_svbr(RZ_NONNULL BcCursor *c, RZ_NONNULL RzBuffer *buf, ut32 width, RZ_OUT ut64 *bits);
RZ_IPI bool bc_curs_read_8(RZ_NONNULL BcCursor *c, RZ_NONNULL RzBuffer *buf, RZ_OUT ut8 *bits);
RZ_IPI bool bc_curs_read_16(RZ_NONNULL BcCursor *c, RZ_NONNULL RzBuffer *buf, RZ_OUT ut16 *bits);
RZ_IPI bool bc_curs_read_32(RZ_NONNULL BcCursor *c, RZ_NONNULL RzBuffer *buf, RZ_OUT ut32 *bits);
RZ_IPI bool bc_curs_read_64(RZ_NONNULL BcCursor *c, RZ_NONNULL RzBuffer *buf, RZ_OUT ut64 *bits);

RZ_IPI RZ_OWN BcParser *bc_parser_new();
RZ_IPI void bc_parser_free(BcParser *p);
RZ_IPI bool bc_parser_is_finished(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf);
RZ_IPI RZ_OWN BcStreamEntry *bc_parser_next(RZ_NONNULL BcParser *p, RZ_NONNULL RzBuffer *buf);

RZ_IPI bool bc_scopes_push(RZ_NONNULL BcScopeStack *s, ut64 blk_id, ut64 abbr_id_width);
RZ_IPI BcScope *bc_scopes_pop(RZ_NONNULL BcScopeStack *s);
RZ_IPI bool bc_scopes_blkinfo_id(RZ_NONNULL BcScopeStack *s, RZ_OUT ut64 *id);
RZ_IPI bool bc_scopes_is_infoblk(RZ_NONNULL BcScopeStack *s);
RZ_IPI bool bc_scopes_is_initial(RZ_NONNULL BcScopeStack *s);
RZ_IPI bool bc_scopes_set_infoblk_id(RZ_NONNULL BcScopeStack *s, ut64 id);
RZ_IPI bool bc_scopes_find_abbr(RZ_NONNULL BcScopeStack *s, ut64 id, RZ_OUT BcAbbr **abbr);
RZ_IPI bool bc_scopes_add_abbr(RZ_NONNULL BcScopeStack *s, RZ_NONNULL BcAbbr *abbr);
RZ_IPI RZ_NULLABLE BcAbbr *bc_scopes_get_abbr(RZ_NONNULL BcScopeStack *s, ut64 abbr_id);
RZ_IPI RZ_OWN BcScope *bc_scope_new(ut64 blk_id, ut64 abbr_id_width);
RZ_IPI void bc_scope_free(BcScope *scp);
RZ_IPI bool bc_scope_is_infoblk(RZ_NONNULL BcScope *scp);

RZ_IPI RZ_OWN BcStreamEntry *bc_stent_new_endblk();
RZ_IPI RZ_OWN BcStreamEntry *bc_stent_new_subblk(ut64 blk_id, ut64 blk_len);
RZ_IPI RZ_OWN BcStreamEntry *bc_stent_new_record_abbr(ut64 abbr_id, ut64 code, RZ_NONNULL RZ_OWN BcFields *fields);
RZ_IPI RZ_OWN BcStreamEntry *bc_stent_new_record_unabbr(ut64 code, RZ_NONNULL RZ_OWN BcFields *fields);
RZ_IPI void bc_stent_free(BcStreamEntry *stent);

RZ_IPI RZ_OWN BcRecord *bc_record_new_unabbr(ut64 code, RZ_NONNULL RZ_OWN BcFields *fields);
RZ_IPI RZ_OWN BcRecord *bc_record_new_abbr(ut64 abbr_id, ut64 code, RZ_NONNULL RZ_OWN BcFields *fields);
RZ_IPI void bc_record_free(BcRecord *record);

RZ_IPI RZ_OWN BcAbbr *bc_abbr_new(RZ_OWN BcAbbrOps *ops);
RZ_IPI void bc_abbr_free(BcAbbr *abbr);
RZ_IPI RZ_OWN BcAbbrOp *bc_abbr_op_new_literal(ut64 val);
RZ_IPI RZ_OWN BcAbbrOp *bc_abbr_op_new_vbr(ut64 val);
RZ_IPI RZ_OWN BcAbbrOp *bc_abbr_op_new_fixed(ut64 val);
RZ_IPI RZ_OWN BcAbbrOp *bc_abbr_op_new_array(RZ_NONNULL RZ_OWN BcAbbrOp *elem_type);
RZ_IPI RZ_OWN BcAbbrOp *bc_abbr_op_new_char6();
RZ_IPI RZ_OWN BcAbbrOp *bc_abbr_op_new_blob();
RZ_IPI void bc_abbr_op_free(BcAbbrOp *op);

RZ_IPI RZ_OWN BcFields *bc_fields_new(size_t capacity);
RZ_IPI void bc_fields_free(BcFields *fields);
RZ_IPI bool bc_fields_push(RZ_NONNULL BcFields *fields, ut64 field);