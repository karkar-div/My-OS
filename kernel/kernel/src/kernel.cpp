#include "kernelUtil.h"


void boot(BootInfo* bootInfo){  

    *(uint8_t*)bootInfo->mMapStart = 0xff;
    KernelInfo kernelInfo = InitializeKernel(bootInfo);
    GlobalRenderer->Println(">> kernel init");
    while(true){
		GlobalGUI->RenderFrame();
		PIT::Sleep(15);
	}
}



#include "kernel.h"

 
void bootTest(Framebuffer fb, ACPI::RSDP2* rsdp, PSF1_FONT* psf1_font,  void* freeMemStart, void* extraMemStart, uint64_t freeMemSize, void* kernelStart, uint64_t kernelSize, void* kernelStartV)
{
    *(uint8_t*)freeMemSize = 0xff;
    BootInfo tempBootInfo;
    tempBootInfo.framebuffer = &fb;
    tempBootInfo.rsdp = rsdp;

    tempBootInfo.psf1_font = psf1_font;



    tempBootInfo.mMapStart = freeMemStart;
    tempBootInfo.m2MapStart = extraMemStart;
    tempBootInfo.mMapSize = freeMemSize;
    
    tempBootInfo.kernelStart = kernelStart;
    tempBootInfo.kernelSize = kernelSize;
    tempBootInfo.kernelStartV = kernelStartV;

    
    *(uint8_t*)tempBootInfo.mMapStart = 0xff;
    boot(&tempBootInfo);
    return;
}


