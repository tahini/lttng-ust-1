/*
 * lttng-context-callstack.c
 *
 * LTTng UST callstack context.
 *
 * Copyright (C) Geneviève Bastien <gbastien@versatic.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; only
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define _LGPL_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <lttng/ust-events.h>
#include <lttng/ust-tracer.h>
#include <lttng/ringbuffer-config.h>
#include <urcu/tls-compat.h>
#include <helper.h>
#include <execinfo.h>
#include "lttng-tracer-core.h"
#define UNW_LOCAL_ONLY
#include <libunwind.h>

#define CONTEXT_NAME "callstack_user"
#define MAX_ENTRIES 128


inline size_t unwind_stack(void** stack, size_t maxsize)
{
	unw_context_t unwind_context;
	unw_getcontext(&unwind_context);

  unw_cursor_t unwind_cursor;
  int ret = unw_init_local(&unwind_cursor, &unwind_context);
  if (ret != 0)
    return 0;

  size_t i;
  for (i = 0; i < maxsize; ++i)
  {
    void* ip = NULL;
    if (unw_get_reg(&unwind_cursor, UNW_REG_IP, (unw_word_t*)&ip) < 0)
      break;

    stack[i] = ip;
    if (unw_step(&unwind_cursor) <= 0)
      break;
  }

  return i;
}

struct lttng_backtrace {
	unsigned int nr_entries;
	void *entries[MAX_ENTRIES];
};

static
size_t callstack_get_size(struct lttng_ctx_field *field, size_t offset)
{
	size_t orig_offset = offset;
	struct lttng_backtrace *cs_backtrace = field->priv;
	size_t size;

	/* do not write data if no space is available */
	if (!cs_backtrace) {
		offset += lib_ring_buffer_align(offset, lttng_alignof(unsigned int));
		offset += sizeof(unsigned int);
		offset += lib_ring_buffer_align(offset, lttng_alignof(unsigned long));
		return offset - orig_offset;
	}

	/* Offset the backtrace size field */
	offset += lib_ring_buffer_align(offset, lttng_alignof(unsigned int));
	offset += sizeof(unsigned int);

	/* Obtain the backtrace at this point, returns the number of enries */
	size = unwind_stack(cs_backtrace->entries, MAX_ENTRIES);
	cs_backtrace->nr_entries = size;

	/* Add the number of entries to offset */
	offset += lib_ring_buffer_align(offset, lttng_alignof(unsigned long));
	offset += sizeof(unsigned long) * cs_backtrace->nr_entries;
	return offset - orig_offset;
}

static
void callstack_record(struct lttng_ctx_field *field,
		 struct lttng_ust_lib_ring_buffer_ctx *ctx,
		 struct lttng_channel *chan)
{
	struct lttng_backtrace *cs_backtrace = field->priv;
	unsigned int nr_seq_entries;

	/* No backtrace, save the 0 length */
	if (!cs_backtrace) {
		nr_seq_entries = 0;
		lib_ring_buffer_align_ctx(ctx, lttng_alignof(unsigned int));
		chan->ops->event_write(ctx, &nr_seq_entries, sizeof(unsigned int));
		lib_ring_buffer_align_ctx(ctx, lttng_alignof(unsigned long));
		return;
	}
	/* Record the size */
	nr_seq_entries = cs_backtrace->nr_entries;
	lib_ring_buffer_align_ctx(ctx, lttng_alignof(unsigned int));
	chan->ops->event_write(ctx, &nr_seq_entries, sizeof(unsigned int));
	/* Record the bactrace */
	lib_ring_buffer_align_ctx(ctx, lttng_alignof(unsigned long));
	chan->ops->event_write(ctx, cs_backtrace->entries,
			sizeof(unsigned long) * nr_seq_entries);
	return;
}

static
void callstack_get_value(struct lttng_ctx_field *field,
		struct lttng_ctx_value *value)
{
	// FIXME: Return a sensible value? The size?
	value->u.s64 = 0;
}

static
void callstack_destroy(struct lttng_ctx_field *field)
{
	/* Destroy the structure allocated for the bactrace */
	struct lttng_backtrace *cs_backtrace = field->priv;

	if (cs_backtrace) {
		free(cs_backtrace);
	}
}

int lttng_add_callstack_to_ctx(struct lttng_ctx **ctx)
{
	struct lttng_backtrace *backtrace;
	struct lttng_ctx_field *field;

	field = lttng_append_context(ctx);
	if (!field)
		return -ENOMEM;
	if (lttng_find_context(*ctx, CONTEXT_NAME)) {
		lttng_remove_context_field(ctx, field);
		return -EEXIST;
	}
	/* Allocate memory for the backtrace */
	backtrace = zmalloc(sizeof(struct lttng_backtrace));
	if (!backtrace) {
		return -ENOMEM;
	}

	field->event_field.name = CONTEXT_NAME;
	field->event_field.type.atype = atype_sequence;
	field->event_field.type.u.sequence.elem_type.atype = atype_integer;
	field->event_field.type.u.sequence.elem_type.u.basic.integer.size = sizeof(unsigned long) * CHAR_BIT;
	field->event_field.type.u.sequence.elem_type.u.basic.integer.alignment = lttng_alignof(long) * CHAR_BIT;
	field->event_field.type.u.sequence.elem_type.u.basic.integer.signedness = lttng_is_signed_type(unsigned long);
	field->event_field.type.u.sequence.elem_type.u.basic.integer.reverse_byte_order = 0;
	field->event_field.type.u.sequence.elem_type.u.basic.integer.base = 16;
	field->event_field.type.u.sequence.elem_type.u.basic.integer.encoding = lttng_encode_none;

	field->event_field.type.u.sequence.length_type.atype = atype_integer;
	field->event_field.type.u.sequence.length_type.u.basic.integer.size = sizeof(unsigned int) * CHAR_BIT;
	field->event_field.type.u.sequence.length_type.u.basic.integer.alignment = lttng_alignof(unsigned int) * CHAR_BIT;
	field->event_field.type.u.sequence.length_type.u.basic.integer.signedness = lttng_is_signed_type(unsigned int);
	field->event_field.type.u.sequence.length_type.u.basic.integer.reverse_byte_order = 0;
	field->event_field.type.u.sequence.length_type.u.basic.integer.base = 10;
	field->event_field.type.u.sequence.length_type.u.basic.integer.encoding = lttng_encode_none;

	field->get_size = callstack_get_size;
	field->record = callstack_record;
	field->get_value = callstack_get_value;
	field->destroy = callstack_destroy;
	field->priv = backtrace;
	lttng_context_update(*ctx);
	return 0;
}
