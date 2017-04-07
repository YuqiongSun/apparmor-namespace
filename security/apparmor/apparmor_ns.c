/*
 * Copyright (C) 2017
 * Author: Yuqiong Sun <sunyuqiong1988@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 */



#include <linux/export.h>
#include <linux/user_namespace.h>
#include <linux/proc_ns.h>
#include <linux/aa_namespace.h>

struct apparmor_namespace init_apparmor_ns = {
    .kref = {
        .refcount = ATOMIC_INIT(2),
    },
    .user_ns = &init_user_ns,
    .ns.inum = PROC_AA_INIT_INO,
#ifdef CONFIG_AA_NS
    .ns.ops = &aans_operations,
#endif
    .parent = NULL,
    .root_ns = NULL,
};
EXPORT_SYMBOL(init_apparmor_ns);
