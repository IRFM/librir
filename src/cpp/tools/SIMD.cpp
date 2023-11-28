#include "SIMD.h"

// https://stackoverflow.com/questions/6121792/how-to-check-if-a-cpu-supports-the-sse3-instruction-set

#ifdef _MSC_VER

#include "Windows.h"
//  Windows
#define cpuid(info, x) __cpuidex(info, x, 0)

#else

//  GCC Intrinsics
#include <cpuid.h>
void cpuid(int info[4], int InfoType)
{
	__cpuid_count(InfoType, 0, info[0], info[1], info[2], info[3]);
}

#endif

namespace rir
{
	SIMD &detectInstructionSet()
	{
		static SIMD simd;
		static bool detect = false;
		if (detect)
			return simd;

		detect = true;
		int info[4];
		cpuid(info, 0);
		int nIds = info[0];

		cpuid(info, 0x80000000);
		unsigned nExIds = info[0];

		//  Detect Features
		if (nIds >= 0x00000001)
		{
			cpuid(info, 0x00000001);
			simd.HW_MMX = (info[3] & ((int)1 << 23)) != 0;
			simd.HW_SSE = (info[3] & ((int)1 << 25)) != 0;
			simd.HW_SSE2 = (info[3] & ((int)1 << 26)) != 0;
			simd.HW_SSE3 = (info[2] & ((int)1 << 0)) != 0;

			simd.HW_SSSE3 = (info[2] & ((int)1 << 9)) != 0;
			simd.HW_SSE41 = (info[2] & ((int)1 << 19)) != 0;
			simd.HW_SSE42 = (info[2] & ((int)1 << 20)) != 0;
			simd.HW_AES = (info[2] & ((int)1 << 25)) != 0;

			simd.HW_AVX = (info[2] & ((int)1 << 28)) != 0;
			simd.HW_FMA3 = (info[2] & ((int)1 << 12)) != 0;

			simd.HW_RDRAND = (info[2] & ((int)1 << 30)) != 0;
		}
		if (nIds >= 0x00000007)
		{
			cpuid(info, 0x00000007);
			simd.HW_AVX2 = (info[1] & ((int)1 << 5)) != 0;

			simd.HW_BMI1 = (info[1] & ((int)1 << 3)) != 0;
			simd.HW_BMI2 = (info[1] & ((int)1 << 8)) != 0;
			simd.HW_ADX = (info[1] & ((int)1 << 19)) != 0;
			simd.HW_SHA = (info[1] & ((int)1 << 29)) != 0;
			simd.HW_PREFETCHWT1 = (info[2] & ((int)1 << 0)) != 0;

			simd.HW_AVX512F = (info[1] & ((int)1 << 16)) != 0;
			simd.HW_AVX512CD = (info[1] & ((int)1 << 28)) != 0;
			simd.HW_AVX512PF = (info[1] & ((int)1 << 26)) != 0;
			simd.HW_AVX512ER = (info[1] & ((int)1 << 27)) != 0;
			simd.HW_AVX512VL = (info[1] & ((int)1 << 31)) != 0;
			simd.HW_AVX512BW = (info[1] & ((int)1 << 30)) != 0;
			simd.HW_AVX512DQ = (info[1] & ((int)1 << 17)) != 0;
			simd.HW_AVX512IFMA = (info[1] & ((int)1 << 21)) != 0;
			simd.HW_AVX512VBMI = (info[2] & ((int)1 << 1)) != 0;
		}
		if (nExIds >= 0x80000001)
		{
			cpuid(info, 0x80000001);
			simd.HW_x64 = (info[3] & ((int)1 << 29)) != 0;
			simd.HW_ABM = (info[2] & ((int)1 << 5)) != 0;
			simd.HW_SSE4a = (info[2] & ((int)1 << 6)) != 0;
			simd.HW_FMA4 = (info[2] & ((int)1 << 16)) != 0;
			simd.HW_XOP = (info[2] & ((int)1 << 11)) != 0;
		}

		return simd;
	}

}