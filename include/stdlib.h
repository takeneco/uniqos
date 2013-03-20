/// @file   stdlib.h
/// @brief  used by ACPICA
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_STDLIB_H_
#define INCLUDE_STDLIB_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


unsigned long strtoul(const char *nptr, char **endptr, int base);


#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // include guard

