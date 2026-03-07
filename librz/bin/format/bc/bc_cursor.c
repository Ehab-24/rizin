// SPDX-License-Identifier: LGPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ehab-24 <ehabs1775@gmail.com>

#include "bc.h"
#include "rz_util/rz_assert.h"

static void reset_cblk(RZ_NONNULL BcCursor *c) {
	c->cblk = 0;
	c->rbits = 0;
}

static bool load_blk(RZ_NONNULL BcCursor *c, RZ_NONNULL RzBuffer *buf) {
	reset_cblk(c);
	const size_t start = bc_curs_tell_byte(c);
	const size_t len = RZ_MIN(BLOCK_SIZE, rz_buf_size(buf) - start);

	ut8 bytes[BLOCK_SIZE] = { 0 };
	if (rz_buf_read_at(buf, start, bytes, len) < 0) {
		return false;
	}
	for (size_t i = 0; i < len; ++i) {
		c->cblk |= (ut64)bytes[i] << (i * 8);
	}
	c->rbytes += len;
	c->rbits = len * 8;
	return true;
}

RZ_IPI bool bc_curs_has_consumed(RZ_NONNULL const BcCursor *c, size_t nbytes) {
	return c->rbits == 0 && c->rbytes >= nbytes;
}

RZ_IPI size_t bc_curs_tell_bit(RZ_NONNULL const BcCursor *c) {
	return c->rbytes * 8 - c->rbits;
}

RZ_IPI size_t bc_curs_tell_byte(RZ_NONNULL const BcCursor *c) {
	return bc_curs_tell_bit(c) / 8;
}

RZ_IPI void bc_curs_align(RZ_NONNULL BcCursor *c, ut32 width) {
	if (c->rbits > width) {
		c->cblk >>= c->rbits - width;
		c->rbits = width;
	} else if (c->rbits < width) {
		reset_cblk(c);
	}
}

RZ_IPI bool bc_curs_read(RZ_NONNULL BcCursor *c, RZ_NONNULL RzBuffer *buf, ut32 n, RZ_OUT ut64 *bits) {
	rz_return_val_if_fail(n > 0 && n <= BLOCK_SIZE_BITS, false);
	if (n <= c->rbits) {
		const ut64 val = c->cblk & (0xFFFFFFFFFFFFFFFF >> (BLOCK_SIZE_BITS - n));
		c->rbits -= n;
		c->cblk >>= n;
		*bits = val;
		return true;
	}
	const ut32 need = n - c->rbits;
	const ut64 pblk = c->cblk; ///< previous block
	if (!load_blk(c, buf)) {
		return false;
	}
	if (need > c->rbits) {
		return false;
	}
	const ut64 val = c->cblk & (0xFFFFFFFFFFFFFFFF >> (BLOCK_SIZE_BITS - need));
	c->rbits -= need;
	c->cblk >>= need;
	*bits = (val << (n - need)) | pblk;
	return true;
}

RZ_IPI bool bc_curs_read_vbr(RZ_NONNULL BcCursor *c, RZ_NONNULL RzBuffer *buf, ut32 width, RZ_OUT ut64 *bits) {
	rz_return_val_if_fail(width >= 2 && width <= MAX_VBR_BITS, false);
	const ut64 mask = 1 << (width - 1);
	ut32 shift = 0;
	ut64 ret = 0;
	while (true) {
		ut64 val;
		if (!bc_curs_read(c, buf, width, &val)) {
			return false;
		}
		ret |= (val & ~mask) << shift;
		if ((val & mask) == 0) {
			break;
		}
		shift += width - 1;
	}
	*bits = ret;
	return true;
}

RZ_IPI bool bc_curs_read_svbr(RZ_NONNULL BcCursor *c, RZ_NONNULL RzBuffer *buf, ut32 width, RZ_OUT ut64 *bits) {
	ut64 val;
	if (!bc_curs_read_vbr(c, buf, width, &val)) {
		return false;
	}
	const ut32 sign = val & (1 << (width - 1));
	val >>= 1;
	*bits = sign ? -val : val;
	return true;
}

RZ_IPI bool bc_curs_read_8(RZ_NONNULL BcCursor *c, RZ_NONNULL RzBuffer *buf, RZ_OUT ut8 *bits) {
	ut64 val;
	if (!bc_curs_read(c, buf, 8, &val)) {
		return false;
	}
	*bits = (ut8)val;
	return true;
}

RZ_IPI bool bc_curs_read_16(RZ_NONNULL BcCursor *c, RZ_NONNULL RzBuffer *buf, RZ_OUT ut16 *bits) {
	ut64 val;
	if (!bc_curs_read(c, buf, 16, &val)) {
		return false;
	}
	*bits = (ut16)val;
	return true;
}

RZ_IPI bool bc_curs_read_32(RZ_NONNULL BcCursor *c, RZ_NONNULL RzBuffer *buf, RZ_OUT ut32 *bits) {
	ut64 val;
	if (!bc_curs_read(c, buf, 32, &val)) {
		return false;
	}
	*bits = (ut32)val;
	return true;
}

RZ_IPI bool bc_curs_read_64(RZ_NONNULL BcCursor *c, RZ_NONNULL RzBuffer *buf, RZ_OUT ut64 *bits) {
	return bc_curs_read(c, buf, 64, bits);
}

RZ_IPI RZ_OWN BcCursor *bc_curs_new() {
	return RZ_NEW0(BcCursor);
}

RZ_IPI void bc_curs_free(BcCursor *c) {
	free(c);
}
