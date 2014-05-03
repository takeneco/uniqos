/// @file  main.cc
/// @brief first process main function.


extern "C" int main()
{
	for (;;) {
		asm volatile ("syscall" : : "a"(100));
	}
}
