/// @file  ctype.h
/// @brief used by ACPICA
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_CTYPE_H_
#define INCLUDE_CTYPE_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


int isalpha(int c);
int isdigit(int c);
int isprint(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);


#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // INCLUDE_CTYPE_H_

