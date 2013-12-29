/// @file   stdlib.h
/// @brief  used by ACPICA
//
// (C) 2012-2013 KATO Takeshi
//

#ifndef CORE_INCLUDE_STDLIB_H_
#define CORE_INCLUDE_STDLIB_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


unsigned long strtoul(const char *nptr, char **endptr, int base);


#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // CORE_INCLUDE_STDLIB_H_

