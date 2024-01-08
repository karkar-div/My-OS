#include "kernelUtil.h"
#include "gdt/gdt.h"
#include "interrupts/IDT.h"
#include "interrupts/interrupts.h"

uint64_t _KernelStart; 
uint64_t _KernelEnd;


KernelInfo kernelInfo;
static PageFrameAllocator t = PageFrameAllocator();

#define USER_END                         0x0000007fffffffff
#define TRANSLATED_PHYSICAL_MEMORY_BEGIN 0xffff800000000000llu
#define MMIO_BEGIN                       0xffffffff00000000llu
#define KERNEL_TEMP_BEGIN                0xffffffff40000000llu
#define KERNEL_DATA_BEGIN                0xffffffff80000000llu
#define KERNEL_HEAP_BEGIN                0xffffffff80300000llu

static inline uint64_t earlyVirtualToPhysicalAddr(const void* vAddr){
    if((0xfffff00000000000llu & (uint64_t)vAddr) == TRANSLATED_PHYSICAL_MEMORY_BEGIN)
        return ~TRANSLATED_PHYSICAL_MEMORY_BEGIN & (uint64_t)vAddr;
    else
        return ~KERNEL_DATA_BEGIN & (uint64_t)vAddr;
}

void PrepareMemory(BootInfo* bootInfo){
	// init Global allocater and his bitmap and lock it
    GlobalAllocator = t;
    GlobalAllocator.ReadEFIMemoryMap(bootInfo->mMapStart, bootInfo->mMapSize);

    uint64_t rFB = earlyVirtualToPhysicalAddr(GlobalRenderer->TargetFramebuffer->BaseAddress);


    PageTable* PML4 = (PageTable*)GlobalAllocator.RequestPage();
    memset(PML4, 0, 0x1000);
    asm volatile("mov %%cr3, %0" : "=r"(PML4));
    
    GlobalPageTableManager = PageTableManager(PML4);
    
	uint64_t fbBase = (uint64_t)bootInfo->framebuffer->BaseAddress;
	uint64_t fbSize = (uint64_t)bootInfo->framebuffer->BufferSize + 0x1000;
	
    kernelInfo.pageTableManager = &GlobalPageTableManager;
}




IDTR idtr;
void SetIDTGate(void* handler, uint8_t entryOffset, uint8_t type_attr, uint8_t selector){

	IDTDescEntry* interrupt = (IDTDescEntry*)(idtr.Offset + entryOffset * sizeof(IDTDescEntry));
	interrupt->SetOffset((uint64_t)handler);
	interrupt->type_attr = type_attr;
	interrupt->selector = selector;
}

void PrepareInterrupts(){
	idtr.Limit = 0x0FFF;
	idtr.Offset = (uint64_t)GlobalAllocator.RequestPage();

	SetIDTGate((void*)PageFault_Handler,   0xE,  IDT_TA_InterruptGate, 0x08);
	SetIDTGate((void*)DoubleFault_Handler, 0x8,  IDT_TA_InterruptGate, 0x08);
	SetIDTGate((void*)GPFault_Handler,     0xD,  IDT_TA_InterruptGate, 0x08);
	SetIDTGate((void*)KeyboardInt_Handler, 0x21, IDT_TA_InterruptGate, 0x08);
	SetIDTGate((void*)MouseInt_Handler,    0x2C, IDT_TA_InterruptGate, 0x08);
	SetIDTGate((void*)PITInt_Handler,      0x20, IDT_TA_InterruptGate, 0x08);
 
	asm ("lidt %0" : : "m" (idtr));

	RemapPIC();
}

void PrepareACPI(BootInfo* bootInfo){
	ACPI::SDTHeader* xsdt = (ACPI::SDTHeader*)(bootInfo->rsdp->XSDTAddress);
	
	ACPI::MCFGHeader* mcfg = (ACPI::MCFGHeader*)ACPI::FindTable(xsdt, (char*)"MCFG");

	PCI::EnumeratePCI(mcfg);
}

void ShowSettings(){
	if(setting_window->IsHidden)
		setting_window->IsHidden = false;
	else 
		setting_window->IsHidden = true;
}

void AppIconClick(){
	MousePosition;
	for(int i = 0; i < GlobalAppHandler->AppsCount;i++){
		uint8_t padding = 16;
		uint16_t min = menu->frame->containers[1]->Y
					 + i * ( padding + menu->frame->containers[1]->Width) ;
		uint16_t max = min 
					 + menu->frame->containers[1]->Width * (i+1);
		if( MousePosition.Y > min && MousePosition.Y < max )
			GlobalAppHandler->Run(i);
		
	}
	
}

void CreateMenu(PSF1_FONT* font){

	uint64_t b = 0x00ff0000 ;
	prerendered_screen = (Screen*)GlobalAllocator.RequestBytesInPages(GlobalRenderer->TargetFramebuffer->Height*sizeof(void*));
	GlobalRenderer->Println("Prerendered screen init rows alloc colplited");
	for (int i = 0; i < GlobalRenderer->TargetFramebuffer->Height; i++){
		GlobalRenderer->Print("R");
		if(i % 7 == 0)
			b -= 0x00010000;
		uint64_t backgroundcolor = 0xff006666;
		prerendered_screen->rows[i] = (Row*)GlobalAllocator.RequestBytesInPages(GlobalRenderer->TargetFramebuffer->Width*sizeof(uint32_t));
		for (int n = 0; n < GlobalRenderer->TargetFramebuffer->Width; n++){
			prerendered_screen->rows[i]->pixels[n] = backgroundcolor + b;
			if (n % 32 == 5){
				backgroundcolor --;
			}
			
		}
	}
	GlobalRenderer->Println("    >> Background Done");

	/*
	 *  Program & Settings Menu
	 */ 

	uint8_t icon_size = 12*8;
	uint8_t main_container_padding = 16;

	menu = (Window*)GlobalAllocator.RequestBytesInPages(sizeof(Window));
	*menu = Window(icon_size+main_container_padding*2,GlobalRenderer->TargetFramebuffer->Height,Black,font);
	menu->X = GlobalRenderer->TargetFramebuffer->Width- (icon_size + main_container_padding*2);
	menu->Y = 0;
	menu->IsHidden = false;
	
	Container* LogoContainer = (Container*) GlobalAllocator.RequestBytesInPages(sizeof(Container));
	*LogoContainer = Container(main_container_padding,main_container_padding,icon_size,icon_size,Gray);
	menu->AddContainer(LogoContainer);
	
	Container* ProgramListContainer = (Container*) GlobalAllocator.RequestBytesInPages(sizeof(Container));
	*ProgramListContainer = Container(main_container_padding,main_container_padding*2+icon_size,GlobalRenderer->TargetFramebuffer->Height-130,icon_size,Gray);
	menu->AddContainer(ProgramListContainer);

	Label* settings_label = (Label*)GlobalAllocator.RequestBytesInPages(sizeof(Label));
	*settings_label = Label(0,0);
	settings_label->SetText(L"الاعدادات");
	settings_label->Horizontal_Dock = Center;
	settings_label->Vertical_Dock   = Right_Bottom;
	settings_label->Direction = RTL;
	menu->frame->containers[0]->AddElement(settings_label);

	BitImg* KarKarLogo = (BitImg*)GlobalAllocator.RequestBytesInPages(sizeof(BitImg));
	*KarKarLogo = BitImg(0,0);
	KarKarLogo->SetImg(Logo,64,64);
	KarKarLogo->Horizontal_Dock = Center;
	KarKarLogo->Vertical_Dock   = Left_Top;
	menu->frame->containers[0]->AddElement(KarKarLogo);

	menu->frame->containers[0]->Function[0] = ShowSettings;

	for ( int i = 0; i < GlobalAppHandler->AppsCount; i++){
		Container* ProgramButton = (Container*) GlobalAllocator.RequestBytesInPages(sizeof(Container));
		*ProgramButton = Container(main_container_padding,menu->frame->containers[1]->Y+(icon_size*i),icon_size,icon_size,Gray);
		menu->AddContainer(ProgramButton);
		menu->frame->containers[i+2]->Function[0] = &AppIconClick;

		App* app = GlobalAppHandler->Apps[i];
		Label* ProgramName = (Label*)GlobalAllocator.RequestBytesInPages(sizeof(Label));
		*ProgramName = Label(0,0);
		ProgramName->SetText(app->Name);
		ProgramName->Horizontal_Dock = Center;
		ProgramName->Vertical_Dock   = Right_Bottom;
		ProgramName->Direction = RTL;
		
		menu->frame->containers[i+2]->AddElement(ProgramName);
	}

	GlobalGUI->Push(menu);
	GlobalRenderer->Println("    >> Program Menu Done");

	/*
	 * Top bar as a window
	 */

	top_bar = (Window*)GlobalAllocator.RequestBytesInPages(sizeof(Window));
	*top_bar = Window(GlobalRenderer->TargetFramebuffer->Width-(icon_size+main_container_padding*2),32,Black,font);
	top_bar->X = 0; 
	top_bar->Y = 0;
	top_bar->IsHidden = false;

	Container* bar = (Container*) GlobalAllocator.RequestBytesInPages(sizeof(Container));
	*bar = Container(0,0,32,top_bar->Width,Black);
	top_bar->AddContainer(bar);

	Label* OS_Name_Label = (Label*)GlobalAllocator.RequestBytesInPages(sizeof(Label));
	*OS_Name_Label = Label(0,0);
	OS_Name_Label->SetText(L"نظام تشغيل كركر");
	OS_Name_Label->Horizontal_Dock = Right_Bottom;
	OS_Name_Label->Vertical_Dock   = Center;
	OS_Name_Label->Direction = RTL;
	top_bar->frame->containers[0]->AddElement(OS_Name_Label);

	Label* Time_Label = (Label*)GlobalAllocator.RequestBytesInPages(sizeof(Label));
	*Time_Label = Label(0,0);
	Time_Label->SetText(L"HH:MM:SS | DD/MM/YYYY");
	Time_Label->Horizontal_Dock = Left_Top;
	Time_Label->Vertical_Dock   = Center;
	Time_Label->Direction = RTL;
	top_bar->frame->containers[0]->AddElement(Time_Label);

	GlobalGUI->Push(top_bar);
	GlobalRenderer->Println("    >> Top Menu Done");
	/*
	 *  Settings menu
	 */


	setting_window = (Window*)GlobalAllocator.RequestBytesInPages(sizeof(Window));
	*setting_window = Window(600,400,White,font);
	setting_window->X = GlobalRenderer->TargetFramebuffer->Width - menu->Width-600;
	setting_window->Y = 32;
	setting_window->IsHidden = true;

	GlobalGUI->Push(setting_window);
	GlobalRenderer->Println("    >> Setting Window Done");

}

/*void InitializeFiles(){
	if (AHCI::GloablAHCIDriver == NULL) return;
	GlobalFileIO = (FileIO*)GlobalAllocator.RequestBytesInPages(sizeof(GlobalFileIO));
	*GlobalFileIO = FileIO(AHCI::GloablAHCIDriver);
}*/

void InitializeGUI(){
	GlobalGUI = (GUI*)GlobalAllocator.RequestBytesInPages(sizeof(GUI));
	*GlobalGUI = GUI();

	IsGUIDone = true;
}

void InitializeUnicodeDecoder(){
	GlobalDecoder = (UnicodeDecoder*) GlobalAllocator.RequestBytesInPages(sizeof(GlobalDecoder));
	*GlobalDecoder = UnicodeDecoder();
}

void LoadApps(){
	GlobalAppHandler = (AppsHandler*) GlobalAllocator.RequestBytesInPages(sizeof(AppsHandler));
	*GlobalAppHandler = AppsHandler();
	
	/*DirectoryEntry* AppsDir = (DirectoryEntry*) GlobalFileIO->Read("APPS       ");
	DirectoryEntry* AppsDirDir = GlobalFileIO->findFile("APPS       ");
	uint8_t ProgramNumber = 1;
	for ( int i = 0; i < ProgramNumber; i++){
		GlobalRenderer->Print("App Init");
		GlobalAppHandler->Push(
			(App*)GlobalFileIO->Read(&AppsDir[2+i],1),
			AppsDir[2+i].Name);
	}*/
}

BasicRenderer r = BasicRenderer(NULL, NULL);
KernelInfo InitializeKernel(BootInfo* bootInfo){

	memset(bootInfo->framebuffer->BaseAddress, 0, bootInfo->framebuffer->BufferSize);

	r = BasicRenderer(bootInfo->framebuffer, bootInfo->psf1_font);
	GlobalRenderer = &r;
	GlobalRenderer->Println(">> Grafic init");

	GDTDescriptor gdtDescriptor;
	gdtDescriptor.Size = sizeof(GDT) - 1;
	gdtDescriptor.Offset = (uint64_t)&DefaultGDT;
	LoadGDT(&gdtDescriptor);
	GlobalRenderer->Println(">> GDT Loaded");

	PrepareMemory(bootInfo);
	GlobalRenderer->Println(">> Memmory Prepared");

	PrepareInterrupts();
	GlobalRenderer->Println(">> Interrupts Prepared");

	InitPS2Mouse();
	GlobalRenderer->Println(">> Mouse Init");

	//PrepareACPI(bootInfo);
	GlobalRenderer->Println(">> AHCI Prepared");

	outb(PIC1_DATA, 0b11111000);
	outb(PIC2_DATA, 0b11101111);

	asm ("sti");

	//InitializeFiles();

	LoadApps();
	GlobalRenderer->Println(">> Apps Loaded");

	InitializeUnicodeDecoder();
	GlobalRenderer->Println(">> Decoder Init");

	InitializeGUI();
	GlobalRenderer->Println(">> Gui Init");

	CreateMenu(bootInfo->psf1_font);
	GlobalRenderer->Println(">> Menu Created");
	

	return kernelInfo;
}


