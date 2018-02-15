/*
 * Copyright (C) 2018 Geneviève Bastien <gbastien@versatic.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#define _GNU_SOURCE
#define _LGPL_SOURCE
#include <link.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

#include <lttng/ust-elf.h>
#include <helper.h>
#include "lttng-tracer-core.h"
#include "lttng-ust-statedump-global.h"
#include "jhash.h"
#include "getenv.h"

#define TRACEPOINT_DEFINE

#define TRACEPOINT_CREATE_PROBES
#define TP_SESSION_CHECK
#include "lttng-ust-statedump-global-provider.h"	/* Define and create probes. */

static
void trace_statedump_global_start(struct lttng_session *session)
{
	tracepoint(lttng_ust_statedump_global, start, session);
}

static
void trace_statedump_global_end(struct lttng_session *session)
{
	tracepoint(lttng_ust_statedump_global, end, session);
}

/*
 * Generate a global statedump for the session. Unlike the other
 * lttng_ust_statedump which is linked to a traced application, this
 * one will be triggered even if there is no traced application or
 * events. start and end events will occur at beginning and end
 * of the global statedump.
 *
 * Grab the ust_lock outside of the RCU read-side lock because we
 * perform synchronize_rcu with the ust_lock held, which can trigger
 * deadlocks otherwise.
 */
int do_lttng_ust_statedump_global(struct lttng_session *session)
{
	DBG("Start global statedump");
	ust_lock_nocheck();
	trace_statedump_global_start(session);
	ust_unlock();

//	do_baddr_statedump(owner);
	DBG("End global statedump");
	ust_lock_nocheck();
	trace_statedump_global_end(session);
	ust_unlock();

	return 0;
}

void lttng_ust_statedump_global_init(void)
{
	__tracepoints__init();
	__tracepoints__ptrs_init();
	__lttng_events_init__lttng_ust_statedump_global();
}

void lttng_ust_statedump_global_destroy(void)
{
	__lttng_events_exit__lttng_ust_statedump_global();
	__tracepoints__ptrs_destroy();
	__tracepoints__destroy();
}
