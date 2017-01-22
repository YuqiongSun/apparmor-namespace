/*
 * Copyright (C) 2017
 * Author: Yuqiong Sun <sunyuqiong1988@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
*/

#include <linux/export.h>
#include <linux/aa_namespace.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/user_namespace.h>
#include <linux/proc_ns.h>

static struct apparmor_namespace *create_aa_ns(void)
{
   struct apparmor_namespace *aa_ns;

   aa_ns = kmalloc(sizeof(struct apparmor_namespace), GFP_KERNEL);
   if (aa_ns)
       kref_init(&aa_ns->kref);

   return aa_ns;
}

/**
 * Clone a new ns copying an original aa namespace, setting refcount to 1
 * @old_ns: old aa namespace to clone
 * @user_ns: user namespace that current task runs in
 * Return ERR_PTR(-ENOMEM) on error (failure to kmalloc), new ns otherwise
 */
static struct apparmor_namespace *clone_aa_ns(struct user_namespace *user_ns,
                       struct apparmor_namespace *old_ns)
{
   struct apparmor_namespace *ns;
   int err;

   ns = create_aa_ns();
   if (!ns)
       return ERR_PTR(-ENOMEM);

   err = ns_alloc_inum(&ns->ns);
   if (err) {
       kfree(ns);
       return ERR_PTR(err);
   }

   ns->ns.ops = &aans_operations;
   get_aa_ns(old_ns);
   ns->parent = old_ns;
   ns->user_ns = get_user_ns(user_ns);

   return ns;
}

/**
 * Copy task's aa namespace, or clone it if flags
 * specifies CLONE_NEWAA.  In latter case, events
 * in new aa namespace will be measured against a
 * separate measurement policy and results will be
 * extended into a sparate measurement list
 *
 * @flags: flags used in the clone syscall
 * @user_ns: user namespace that current task runs in
 * @old_ns: old aa namespace to clone
 */
struct apparmor_namespace *copy_aa(unsigned long flags,
               struct user_namespace *user_ns,
               struct apparmor_namespace *old_ns)
{
    struct apparmor_namespace *new_ns;

    BUG_ON(!old_ns);
    get_aa_ns(old_ns);

    if (!(flags & CLONE_NEWAA))
        return old_ns;
    printk(KERN_DEBUG "SYQ: AppArmor namespace is created\n");
    
    new_ns = clone_aa_ns(user_ns, old_ns);
    put_aa_ns(old_ns);

    return new_ns;
}

static void destroy_aa_ns(struct apparmor_namespace *ns)
{
   put_user_ns(ns->user_ns);
   ns_free_inum(&ns->ns);
   kfree(ns);
}

void free_aa_ns(struct kref *kref)
{
   struct apparmor_namespace *ns;
   struct apparmor_namespace *parent;

   ns = container_of(kref, struct apparmor_namespace, kref);

   while (ns != &init_apparmor_ns) {
       parent = ns->parent;
       destroy_aa_ns(ns);
       put_aa_ns(parent);
       ns = parent;
   }
}


static inline struct apparmor_namespace *to_aa_ns(struct ns_common *ns)
{
   return container_of(ns, struct apparmor_namespace, ns);
}

static struct ns_common *aans_get(struct task_struct *task)
{
   struct apparmor_namespace *ns = NULL;
   struct nsproxy *nsproxy;

   task_lock(task);
   nsproxy = task->nsproxy;
   if (nsproxy) {
       ns = nsproxy->aa_ns;
       get_aa_ns(ns);
   }
   task_unlock(task);

   return ns ? &ns->ns : NULL;
}

static void aans_put(struct ns_common *ns)
{
   put_aa_ns(to_aa_ns(ns));
}

static int aans_install(struct nsproxy *nsproxy, struct ns_common *new)
{
   struct apparmor_namespace *ns = to_aa_ns(new);

   if (!ns_capable(ns->user_ns, CAP_SYS_ADMIN) ||
       !ns_capable(current_user_ns(), CAP_SYS_ADMIN))
       return -EPERM;

   get_aa_ns(ns);
   put_aa_ns(nsproxy->aa_ns);
   nsproxy->aa_ns = ns;
   return 0;
}

const struct proc_ns_operations aans_operations = {
   .name           = "aa",
   .type           = CLONE_NEWAA,
   .get            = aans_get,
   .put            = aans_put,
   .install        = aans_install,
};
