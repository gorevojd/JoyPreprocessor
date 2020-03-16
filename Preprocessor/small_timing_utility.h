#ifndef SMALL_TIMING_UTILITY_H
#define SMALL_TIMING_UTILITY_H

#include <Windows.h>
#include <stdio.h>

struct timing_ctx {
	LARGE_INTEGER BeginClock;
	LARGE_INTEGER Freq;
	float OneOverFreq;
	unsigned long long BeginRDTSC;

	char Name[256];
};

inline void BeginTiming(timing_ctx* Timing, const char* Name) {
	strcpy(Timing->Name, Name);
	QueryPerformanceFrequency(&Timing->Freq);
	Timing->OneOverFreq = 1.0f / (float)Timing->Freq.QuadPart;

	Timing->BeginRDTSC = __rdtsc();

	QueryPerformanceCounter(&Timing->BeginClock);
}

inline void EndTiming(timing_ctx* Timing) {
	LARGE_INTEGER EndClockLI;
	QueryPerformanceCounter(&EndClockLI);
	unsigned long long ElapsedCounter = __rdtsc() - Timing->BeginRDTSC;
	unsigned long long ClocksElapsed = EndClockLI.QuadPart - Timing->BeginClock.QuadPart;
	float DeltaTime = (float)ClocksElapsed * Timing->OneOverFreq;

	printf("%s. Elapsed seconds %.3f, counter clocks %u \n", Timing->Name, DeltaTime, (unsigned)ClocksElapsed);

	Timing->Name[0] = 0;
}

#endif