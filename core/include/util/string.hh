// @file   core/string.hh
// @brief  Memory ops.
//
// (C) 2010-2014 KATO Takeshi
//

#ifndef CORE_STRING_HH_
#define CORE_STRING_HH_

#include <core/basic.hh>


int mem_compare(uptr bytes, const void* mem1, const void* mem2);  // OBSOLETED
int mem_compare(const void* mem1, const void* mem2, uptr bytes);
void mem_move(uptr bytes, const void* src, void* dest);  // OBSOLETED
void mem_move(const void* src, void* dest, uptr bytes);
void mem_copy(uptr bytes, const void* src, void* dest);  // OBSOLETED
void mem_copy(const void* src, void* dest, uptr bytes);
void mem_fill(uptr bytes, u8 c, void* dest);  // OBSOLETED
void mem_fill(u8 c, void* dest, uptr bytes);

int str_length(const char* str);
int str_compare(uptr max, const char* str1, const char* str2);  // OBSOLETED
int str_compare(const char* str1, const char* str2, uptr max);
int str_compare(const char* str1, const char* str2);
void str_copy(const char* src, char* dest);
void str_copy(uptr max, const char* src, char* dest);  // OBSOLETED
void str_copy(const char* src, char* dest, uptr max);
void str_concat(const char* src, char* dest);
void str_concat(uptr max, const char* src, char* dest);  // OBSOLETED
void str_concat(const char* src, char* dest, uptr max);

umax str_to_u(u8 base, const char* src, const char** end);
void str_to_upper(int length, char* str);  // OBSOLETED
void str_to_upper(char* str, int length);

int u_to_hexstr(umax n, char s[sizeof (umax) * 2]);
void u8_to_hexstr(u8 n, char s[2]);
int u_to_octstr(umax n, char s[(sizeof (umax) * 8 + 2) / 3]);
int u_to_binstr(umax n, char s[sizeof n * 8]);
int u_to_decstr(umax n, char s[sizeof n * 3]);


#endif  // include guard

