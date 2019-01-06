/* prototypes */

void *slob_kmalloc(unsigned int size);
void *slob_kzalloc(unsigned int size);
void slob_kfree(const void *m);
unsigned int slob_ksize(const void *m);