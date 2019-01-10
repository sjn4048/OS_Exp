/* prototypes */

extern void *slob_kmalloc(unsigned int size);

extern void *slob_kzalloc(unsigned int size);

extern void slob_kfree(const void *m);

extern void init_slob();

unsigned int slob_ksize(const void *m);

#define SLOB_SINGLE