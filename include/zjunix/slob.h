/* prototypes */

extern void *slob_kmalloc(unsigned int size);

extern void *slob_kzalloc(unsigned int size);

extern void slob_kfree(const void *m);

extern void init_slob();

unsigned int slob_ksize(const void *m);

//#define SLOB_DEBUG

#ifdef SLOB_DEBUG
#include <driver/ps2.h>
#endif // DEBUG

#define SLOB_SINGLE