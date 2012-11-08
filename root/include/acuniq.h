/// @file  include/acuniq.h
/// @brief used by ACPICA
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_ACUNIQ_H_
#define INCLUDE_ACUNIQ_H_


/// @def CONFIG_DEBUG_ACPI_VERBOSE
/// 0 なら何も出力しない
/// 1 ならエラーメッセージのみ出力する
/// 2 以上はデバッグメッセージを出力する
#if CONFIG_DEBUG_ACPI_VERBOSE == 0
# define ACPI_NO_ERROR_MESSAGES
#endif  // CONFIG_DEBUG_ACPI_VERBOSE == 0

#if CONFIG_DEBUG_ACPI_VERBOSE >= 2
# define ACPI_DEBUG_OUTPUT
#endif  // CONFIG_DEBUG_ACPI_VERBOSE >= 2

#define ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_USE_DO_WHILE_0

#if ARCH_ADR_BITS == 64
# define ACPI_MACHINE_WIDTH  64
#elif ARCH_ADR_BITS == 32
# define ACPI_MACHINE_WIDTH  32
#endif  // ARCH_ADR_BITS == 64

#define COMPILER_DEPENDENT_INT64   long
#define COMPILER_DEPENDENT_UINT64  unsigned long
#define ACPI_CACHE_T               void


#include <stdarg.h>
#include <platform/acgcc.h>


#endif  // include guard

