#pragma once

#include "efiMemory.h"
#include "../acpi.h"


uint64_t GetMemorySize(EFI_MEMORY_DESCRIPTOR* mMap, uint64_t mMapEntries, uint64_t mMapDescSize);

void memset(void* start, uint8_t value, uint64_t num);

void memcpy(void* src, void* dest, uint64_t size);

int memcmp(void* src, void* dest, int amt);
