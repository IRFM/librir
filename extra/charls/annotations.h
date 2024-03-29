// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#ifdef _MSC_VER

#include <sal.h>

// Note: these macro's are not prefixed with CHARLS_, as these macro's are used for function parameters.
//       and long macro's would make the code harder to read.
#define IN_ _In_
#undef _In_opt_
#define _In_opt_
#define IN_Z_ _In_z_
#define IN_READS_BYTES_(size) _In_reads_bytes_(size)
#undef _Out_
#define _Out_
#define OUT_OPT_ _Out_opt_
#define OUT_WRITES_BYTES_(size) _Out_writes_bytes_(size)
#define OUT_WRITES_Z_(size_in_bytes) _Out_writes_z_(size_in_bytes)
#define RETURN_TYPE_SUCCESS_(expr) _Return_type_success_(expr)

#else

#define IN_
#define UNUSED________
#define IN_Z_
#define IN_READS_BYTES_(size)
#define UNUSED________
#define OUT_OPT_
#define OUT_WRITES_BYTES_(size)
#define OUT_WRITES_Z_(size_in_bytes)
#define RETURN_TYPE_SUCCESS_(expr)

#endif

#if defined(__clang__) || defined(__GNUC__)

#define CHARLS_ATTRIBUTE(a) __attribute__(a)

#else

#define CHARLS_ATTRIBUTE(a)

#endif