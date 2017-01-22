/*
 * Copyright (C) 2017
 * Author: Yuqiong Sun <sunyuqiong1988@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 */

#include <linux/kref.h>
#include <linux/ns_common.h>
#include <linux/nsproxy.h>
#include <linux/rculist.h>
#include <linux/sched.h>

struct apparmor_namespace {
   struct kref kref;
   struct user_namespace *user_ns;
   struct ns_common ns;
   struct apparmor_namespace *parent;
};

extern struct apparmor_namespace init_apparmor_ns;

#ifdef CONFIG_AA_NS
extern void free_aa_ns(struct kref *kref);

static inline void get_aa_ns(struct apparmor_namespace *ns)
{
   kref_get(&ns->kref);
}

static inline void put_aa_ns(struct apparmor_namespace *ns)
{
   kref_put(&ns->kref, free_aa_ns);
}

extern struct apparmor_namespace *copy_aa(unsigned long flags,
                   struct user_namespace *user_ns,
                   struct apparmor_namespace *old_ns);

#else
static inline void get_aa_ns(struct apparmor_namespace *ns)
{
}

static inline void put_aa_ns(struct apparmor_namespace *ns)
{
}

static inline struct apparmor_namespace *copy_aa(unsigned long flags,
                   struct user_namespace *user_ns,
                   struct apparmor_namespace *old_ns)
{
   if (flags & CLONE_NEWAA)
       return ERR_PTR(-EINVAL);
   return old_ns;
}
#endif
