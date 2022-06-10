#include <cstddef>;

struct sig_Allocator* sig_Allocator_new(size_t size);
void sig_Allocator_destroy(struct sig_Allocator* allocator);
