#ifndef LTTNG_UST_STATEDUMP_GLOBAL_H
#define LTTNG_UST_STATEDUMP_GLOBAL_H

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

#include <lttng/ust-events.h>

void lttng_ust_statedump_global_init(void);
void lttng_ust_statedump_global_destroy(void);

int do_lttng_ust_statedump_global(struct lttng_session *session);

#endif /* LTTNG_UST_STATEDUMP_GLOBAL_H */
