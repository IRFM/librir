#pragma once

#include "rir_config.h"

// Add missing defines with msvc
#ifdef _MSC_VER

#ifdef __AVX2__
// AVX2
#elif defined(__AVX__)
// AVX
#elif (defined(_M_AMD64) || defined(_M_X64))
// SSE2 x64
#ifndef __SSE2__
#define __SSE2__
#endif
#elif _M_IX86_FP == 2
// SSE2 x32
#ifndef __SSE2__
#define __SSE2__
#endif
#elif _M_IX86_FP == 1
// SSE x32
#ifndef __SSE__
#define __SSE__
#endif
#else
// nothing
#endif

#endif

#include <immintrin.h>

/** @file
 */

namespace rir
{
	/**
	Information on supported instruction sets for current CPUs
	*/
	struct SIMD
	{
		//  Misc.
		bool HW_MMX;
		bool HW_x64;
		bool HW_ABM; // Advanced Bit Manipulation
		bool HW_RDRAND;
		bool HW_BMI1;
		bool HW_BMI2;
		bool HW_ADX;
		bool HW_PREFETCHWT1;

		//  SIMD: 128-bit
		bool HW_SSE;
		bool HW_SSE2;
		bool HW_SSE3;
		bool HW_SSSE3;
		bool HW_SSE41;
		bool HW_SSE42;
		bool HW_SSE4a;
		bool HW_AES;
		bool HW_SHA;

		//  SIMD: 256-bit
		bool HW_AVX;
		bool HW_XOP;
		bool HW_FMA3;
		bool HW_FMA4;
		bool HW_AVX2;

		//  SIMD: 512-bit
		bool HW_AVX512F;	//  AVX512 Foundation
		bool HW_AVX512CD;	//  AVX512 Conflict Detection
		bool HW_AVX512PF;	//  AVX512 Prefetch
		bool HW_AVX512ER;	//  AVX512 Exponential + Reciprocal
		bool HW_AVX512VL;	//  AVX512 Vector Length Extensions
		bool HW_AVX512BW;	//  AVX512 Byte + Word
		bool HW_AVX512DQ;	//  AVX512 Doubleword + Quadword
		bool HW_AVX512IFMA; //  AVX512 Integer 52-bit Fused Multiply-Add
		bool HW_AVX512VBMI; //  AVX512 Vector Byte Manipulation Instructions
	};

	/**
	Returns supported instruction sets for current CPUs
	*/
	TOOLS_EXPORT SIMD &detectInstructionSet();

}