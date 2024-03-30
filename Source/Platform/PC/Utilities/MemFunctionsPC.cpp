#include "Precomp.h"
#include "Utilities/MemFunctions.h"

void* CE::FastAlloc(size_t size, size_t alignHint)
{
	return _aligned_malloc(size, alignHint);
}

void CE::FastFree(void* buffer)
{
	_aligned_free(buffer);
}