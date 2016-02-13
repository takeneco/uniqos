/// @file  main.cc
/// @brief first process main function.

//  Uniqos  --  Unique Operating System
//  (C) 2015 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.


typedef unsigned long uptr;

struct syscall_r
{
	uptr ret;
	uptr data;
};

extern "C" syscall_r syscall0(uptr);
extern "C" syscall_r syscall1(uptr, uptr);
extern "C" syscall_r syscall2(uptr, uptr, uptr);
extern "C" syscall_r syscall3(uptr, uptr, uptr, uptr);
extern "C" syscall_r syscall4(uptr, uptr, uptr, uptr, uptr);
extern "C" syscall_r syscall5(uptr, uptr, uptr, uptr, uptr, uptr);

syscall_r mount(const char* source, const char* target, const char* type,
unsigned long mountflags, const void* data)
{
	return syscall5((uptr)source, (uptr)target, (uptr)type,
	    mountflags, (uptr)data, 101);
}

syscall_r read(int iod, void* buf, uptr bytes)
{
	return syscall3((uptr)iod, (uptr)buf, bytes, 102);
}

syscall_r write(int iod, const void* buf, uptr bytes)
{
	//return syscall3((uptr)iod, (uptr)buf, bytes, 103);
	return syscall5((uptr)iod, (uptr)buf, bytes, 0, 0, 103);
}

syscall_r open(const char* path, unsigned int flags)
{
	return syscall2((uptr)path, flags, 104);
}

syscall_r close(int iod)
{
	return syscall1(iod, 105);
}

syscall_r mkdir(const char* path)
{
	return syscall1((uptr)path, 106);
}

extern "C" int main()
{
	mount(0, "/", "ramfs", 0, 0);
	mkdir("/dev");
	for (;;) {
		syscall_r iod = open("/test", 0x01/*create*/|0x02/*write*/);

		syscall1(0/*dummy*/, 100);
		for (int i = 0; i < 256; ++i) {
			//write(1, "X\n", 2);
			write(iod.data, "x", 1);
		}

		close(iod.data);

		iod = open("/test", 0);

		char buf[512];
		syscall_r size = read(iod.data, buf, sizeof buf);
		write(1, buf, size.data);

		close(iod.data);
	}
}
