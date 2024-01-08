#pragma once

#include <stddef.h>
#include <stdint.h>

extern uint64_t _KernelStart;
extern uint64_t _KernelEnd;

#include "BasicRenderer.h"
#include "paging/PageTableManager.h"
#include "gui/gui_handler.h"
#include "userinput/mouse.h"
#include "app/app.h"
#include "ahci/ahci.h"
#include "scheduling/pit/pit.h"

struct BootInfo
{
	Framebuffer* framebuffer;
	PSF1_FONT* psf1_font;
    ACPI::RSDP2* rsdp;
	void* mMapStart;
	void* m2MapStart;
	uint64_t mMapSize;
	void* kernelStart;
	uint64_t kernelSize;
	void* kernelStartV;
};


struct KernelInfo{
    PageTableManager* pageTableManager;
};


#include "memory/memory.h" 




#include "IO.h"

 
#include "gdt/gdt.h"



void PrepareACPI(BootInfo* bootInfo);

void PrepareMemory(BootInfo* bootInfo);

void SetIDTGate(void* handler, uint8_t entryOffset, uint8_t type_attr, uint8_t selector);

void PrepareInterrupts();

void PrepareTerminals();

KernelInfo InitializeKernel(BootInfo* bootInfo);