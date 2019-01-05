// /* prototypes */
// extern void kmem_cache_init(void);

// extern int FASTCALL(kmem_cache_reap(int));
// extern int FASTCALL(kmem_ptr_validate(kmem_cache_t *cachep, void *ptr));


// /* SLOB allocator routines */

// void kmem_cache_init(void);
// kmem_cache_t *kmem_find_general_cachep(size_t, gfp_t gfpflags);
// kmem_cache_t *kmem_cache_create(const char *c, size_t, size_t, unsigned long,
// 	void (*)(void *, kmem_cache_t *, unsigned long),
// 	void (*)(void *, kmem_cache_t *, unsigned long));
// int kmem_cache_destroy(kmem_cache_t *c);
// void *kmem_cache_alloc(kmem_cache_t *c, gfp_t flags);
// void kmem_cache_free(kmem_cache_t *c, void *b);
// const char *kmem_cache_name(kmem_cache_t *);
// void *kmalloc(size_t size, gfp_t flags);
// void *kzalloc(size_t size, gfp_t flags);
// void kfree(const void *m);
// unsigned int ksize(const void *m);
// unsigned int kmem_cache_size(kmem_cache_t *c);

// static inline void *kcalloc(size_t n, size_t size, gfp_t flags)
// {
// 	return kzalloc(n * size, flags);
// }

// #define kmem_cache_shrink(d) (0)
// #define kmem_cache_reap(a)
// #define kmem_ptr_validate(a, b) (0)
// #define kmem_cache_alloc_node(c, f, n) kmem_cache_alloc(c, f)
// #define kmalloc_node(s, f, n) kmalloc(s, f)
