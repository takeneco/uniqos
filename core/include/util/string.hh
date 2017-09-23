// @file   core/string.hh
// @brief  Memory ops.

//  Uniqos  --  Unique Operating System
//  (C) 2017 KATO Takeshi
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

#ifndef CORE_STRING_HH_
#define CORE_STRING_HH_

#include <core/basic.hh>


int mem_compare(const void* mem1, const void* mem2, uptr bytes);
void mem_move(const void* src, void* dest, uptr bytes);
void mem_copy(const void* src, void* dest, uptr bytes);
void mem_fill(u8 c, void* dest, uptr bytes);

int str_length(const char* str);
int str_length1(const char* str);
sint str_compare(const char* str1, const char* str2, uptr max);
sint str_compare(const char* str1, const char* str2);
bool str_startswith(const char* str, const char* prefix);
void str_copy(const char* src, char* dest);
void str_copy(const char* src, char* dest, uptr max);
void str_concat(const char* src, char* dest);
void str_concat(const char* src, char* dest, uptr max);

umax str_to_u(u8 base, const char* src, const char** end);
void str_to_upper(char* str, int length);

int u_to_hexstr(umax n, char s[sizeof (umax) * 2]);
void u8_to_hexstr(u8 n, char s[2]);
int u_to_octstr(umax n, char s[(sizeof (umax) * 8 + 2) / 3]);
int u_to_binstr(umax n, char s[sizeof n * 8]);
int u_to_decstr(umax n, char s[sizeof n * 3]);


#endif  // CORE_STRING_HH_

