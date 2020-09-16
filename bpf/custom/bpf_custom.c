// SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause)
/* Copyright (C) 2021 Authors of Cilium */

#include <bpf/ctx/skb.h>
#include <bpf/api.h>

#include "lib/common.h"

/* Include custom header file (.h) containing the implementation for
 * custom_prog(), for example:
 *
 *     #include "bytecount.h"
 *
 * You can also pass it to the Makefile:
 *
 *     BPF_CUSTOM_PROG=bytecount.h make
 */
#define TO_STRING(X) #X
#define STRINGIFY(X) TO_STRING(X)
#include STRINGIFY(BPF_CUSTOM_PROG)

__section("custom")
int custom_hook(const struct __ctx_buff *ctx)
{
	__u32 custom_meta = ctx_load_meta(ctx, CB_CUSTOM_CALLS);
	__u32 identity = custom_meta & 0xffffff;
	int ret = (custom_meta >> 24) & 0xff;

	/* Call user-defined function from custom header file. */
	custom_prog(ctx, identity);

	/* Return action code selected from parent program, independently of
	 * what the custom function does, to maintain datapath consistency.
	 */
	return ret;
}
