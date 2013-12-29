// @file   string.h
// @brief  used by ACPICA
//
// (C) 2013 KATO Takeshi
//

#ifndef CORE_INCLUDE_STRING_H_
#define CORE_INCLUDE_STRING_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


int memcmp(const void *s1, const void *s2, unsigned long n);
void *memcpy(void *dest, const void *src, unsigned long n);
void *memset(void *dest, int c, unsigned long n);

unsigned long strlen(const char* s);
int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, unsigned long n);
char *strcpy(char *dest, const char *src);
char *strncpy(char* dest, const char* src, unsigned long n);
char *strcat(char *dest, const char *src);


#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // CORE_INCLUDE_STRING_H_

