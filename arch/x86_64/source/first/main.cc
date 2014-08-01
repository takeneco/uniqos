/// @file  main.cc
/// @brief first process main function.

typedef unsigned long uptr;

struct syscall_r
{
	uptr ret;
	uptr data;
};

extern "C" syscall_r syscall5(uptr, uptr, uptr, uptr, uptr, uptr);

syscall_r mount(const char* source, const char* target, const char* type,
unsigned long mountflags, const void* data)
{
	return syscall5((uptr)source, (uptr)target, (uptr)type,
	    mountflags, (uptr)data, 101);
}


extern "C" int main()
{
	mount(0, "/", "ramfs", 0, 0);
	for (;;) {
		asm volatile ("syscall" : : "a"(100));
	}
}
