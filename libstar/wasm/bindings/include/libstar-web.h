#include <cstddef>;

struct star_Allocator* star_Allocator_new(size_t size);
void star_Allocator_destroy(struct star_Allocator* allocator);
