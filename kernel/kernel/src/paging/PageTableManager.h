#pragma once
#include "paging.h"
#include "PageMapIndexer.h"
#include "PageFrameAllocator.h"
#include "../memory/memory.h"
#include <stdint.h>


class PageTableManager
{
    public:
    PageTable* PML4;
    PageTableManager(PageTable* PML4Address);

    void MapMemory(void* virtualMemory, void* physicalMemory);
    void MapMemories(void* virtualMemory, void* physicalMemory, int c);
    void MapFramebufferMemory(void* virtualMemory, void* physicalMemory);

};

extern PageTableManager GlobalPageTableManager;