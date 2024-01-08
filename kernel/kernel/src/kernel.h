#pragma once

#include "Framebuffer.h"
#include "acpi.h"

#include "SimpleFont.h"



void bootTest(Framebuffer fb, ACPI::RSDP2* rsdp, PSF1_FONT* psf1_font, void* freeMemStart, void* extraMemStart, uint64_t freeMemSize, void* kernelStart, uint64_t kernelSize, void* kernelStartV);

void RenderLoop();
void RecoverDed();