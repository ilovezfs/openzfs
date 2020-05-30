/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
#ifndef _SPL_ZFS_CONTEXT_OS_H
#define _SPL_ZFS_CONTEXT_OS_H

#define MSEC_TO_TICK(msec)		((msec) / (MILLISEC / hz))

#define	noinline		__attribute__((noinline))

/* really? */
#define	kpreempt_disable()		((void)0)
#define	kpreempt_enable()		((void)0)

/* Make sure kmem and vmem are already included */
#include <sys/seg_kmem.h>
#include <sys/kmem.h>

/* Since Linux code uses vmem_free() and we already have one: */
#define vmem_free(A, B)			zfs_kmem_free((A), (B))
#define vmem_alloc(A, B)		zfs_kmem_alloc((A), (B))
#define vmem_zalloc(A, B)		zfs_kmem_zalloc((A), (B))

typedef	int	fstrans_cookie_t;
#define	spl_fstrans_mark()		(0)
#define	spl_fstrans_unmark(x)	(x = 0)

#define	zuio_offset(U)		uio_offset((U))
#define	zuio_resid(U)		uio_resid((U))
#define	zuio_iovcnt(U)		uio_iovcnt((U))
#define	zuio_update(U, N)	uio_update((U), (N))
#define	zuio_nonemptyindex(U, O, I)							\
	do {													\
	uint64_t _len;											\
	for ((I) = 0;											\
	    !uio_getiov((U), (I), NULL, &_len) &&				\
	    (I) < uio_iovcnt((U)) &&							\
	    (O) >= _len;										\
	    (O) -= _len, (I)++)									\
		;													\
	} while(0)

static inline uint64_t zuio_iovlen(struct uio *u, uint_t i)
{
	user_size_t iov_len;
	VERIFY0(uio_getiov(u, i, NULL, &iov_len));
	return iov_len;
}

static inline void *zuio_iovbase(struct uio *u, uint_t i)
{
	user_addr_t iov_base;
	VERIFY0(uio_getiov(u, i, &iov_base, NULL));
	return (void *)iov_base;
}

#define	zuio_iov(U, I, B, S)								\
	do {													\
		user_size_t iov_len;								\
		user_addr_t iov_base;								\
		VERIFY0(uio_getiov((U), (I), &iov_base, &iov_len));	\
		(B) = (void *)(uintptr_t) iov_base;					\
		(S) = (uint64_t) iov_len;							\
	} while(0)

#ifdef _KERNEL

struct hlist_node {
	struct hlist_node *next, **pprev;
};

struct hlist_head {
	struct hlist_node *first;
};

typedef struct {
	volatile int counter;
} atomic_t;

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

#define barrier()		__asm__ __volatile__("": : :"memory")
#define smp_rmb()		barrier()

#define READ_ONCE(x) ({							\
			__typeof(x) __var = ({				\
					barrier();					\
					ACCESS_ONCE(x);				\
				});                             \
			barrier();							\
			__var;								\
		})

#define WRITE_ONCE(x, v) do {           \
        barrier();                      \
        ACCESS_ONCE(x) = (v);           \
        barrier();                      \
	} while (0)

/* BEGIN CSTYLED */
#define hlist_for_each(p, head)					\
	for (p = (head)->first; p; p = (p)->next)

#define hlist_entry(ptr, type, field)   container_of(ptr, type, field)
/* END CSTYLED */

static inline void
hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{
	n->next = h->first;
	if (h->first != NULL)
		h->first->pprev = &n->next;
	WRITE_ONCE(h->first, n);
	n->pprev = &h->first;
}

static inline void
hlist_del(struct hlist_node *n)
{
	WRITE_ONCE(*(n->pprev), n->next);
	if (n->next != NULL)
		n->next->pprev = n->pprev;
}


#define HLIST_HEAD_INIT { }
#define HLIST_HEAD(name) struct hlist_head name = HLIST_HEAD_INIT
#define INIT_HLIST_HEAD(head) (head)->first = NULL

/* BEGIN CSTYLED */
#define INIT_HLIST_NODE(node)											\
	do {																\
		(node)->next = NULL;											\
		(node)->pprev = NULL;											\
	} while (0)

/* END CSTYLED */

static inline int
atomic_read(const atomic_t *v)
{
	return (READ_ONCE(v->counter));
}

static inline int
atomic_inc(atomic_t *v)
{
	return (__sync_fetch_and_add(&v->counter, 1) + 1);
}

static inline int
atomic_dec(atomic_t *v)
{
	return (__sync_fetch_and_add(&v->counter, -1) - 1);
}

#endif // _KERNEL

#endif

