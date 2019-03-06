#pragma once

struct list {
	void *prev;
	void *next;
};

// Contrary to most popular linked list macro sets, such as the one in the
// Linux kernel, this one assumes that struct list can always be found in the
// parent structure as '.list'. This prevents elements from containing more
// than one list head, but so be it. This does have the advantage that we can
// use pointers to the actual node structures, instead of pointers to other
// list head structures. This keeps the foreach() loop simple.

#define list_init(x)		((x)->list.prev = (x)->list.next = NULL)
#define list_prev(x)		((__typeof__(x))(x)->list.prev)
#define list_next(x)		((__typeof__(x))(x)->list.next)
#define list_last(f,p)		for ((p) = (f); (p) && list_next(p); (p) = list_next(p)) { ; }
#define list_foreach(f,x)	for ((x) = (f); (x); (x) = list_next(x))

// f is pointer to starting list element;
// x will contain the iterated pointer to a node;
// y is scratch space.
// This macro safely lets us delete the active node.
#define list_foreach_safe(f,x,y)				\
	for ((x) = (f); (void)((x) && ((y) = list_next(x))), (x); (x) = (y))

// p is pointer to node to insert before;
// x is the pointer to the node to insert.
#define list_insert_before(p,x)					\
	(x)->list.prev = list_prev(p);				\
	(x)->list.next = (p);					\
	if (list_prev(p)) list_prev(p)->list.next = (x);	\
	(p)->list.prev = (x);

// p non-NULL, pointer to node after which to insert;
// x, pointer to node to insert:
#define list_insert_after(p,x)					\
	(x)->list.prev = (p);					\
	(x)->list.next = (p)->list.next;			\
	(p)->list.next = (x);					\
	if (list_next(x)) list_next(x)->list.prev = (x);

// s is pointer to pointer to first list element;
// e is pointer to pointer to last list element.
// x is pointer to the node to detach:
#define list_detach(s,e,x)					\
	if ((x)->list.prev != NULL) {				\
		list_prev(x)->list.next = list_next(x);		\
	}							\
	else {							\
		(s) = list_next(x);				\
	}							\
	if ((x)->list.next != NULL) {				\
		list_next(x)->list.prev = list_prev(x);		\
	}							\
	else {							\
		(e) = list_prev(x);				\
	}							\
	(x)->list.prev = (x)->list.next = NULL;

// f is pointer to first list element;
// is initialized if this is the first node to be appended;
// x is pointer to the node to append at end.
// If the previous node is available, use list_insert().
#define list_append(f,x)					\
	if ((f) == NULL) {					\
		(f) = (x);					\
		(x)->list.prev = (x)->list.next = NULL;		\
	}							\
	else {							\
		__typeof__(x) p;				\
		list_last(f, p);				\
		list_insert(p, x);				\
	}
