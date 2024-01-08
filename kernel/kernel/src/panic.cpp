#include "panic.h"
#include "BasicRenderer.h"

void Panic(const char* panicMessage){


    GlobalRenderer->Colour = 0xffffffff;

    GlobalRenderer->Print("Kernel Panic");

    GlobalRenderer->Next();
    GlobalRenderer->Next();

    GlobalRenderer->Print(panicMessage);
}