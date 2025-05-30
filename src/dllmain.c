// needs mingw64-gcc package. and clang + clang extra tools for clangd
#include <libloaderapi.h>
#include <windows.h>

#include "QBDI/VM_C.h"
#include <assert.h>
#include <processenv.h>
#include <stdbool.h>
#include <stdio.h>

#include <QBDI.h>

#define STACK_SIZE 0x100000

QBDI_NOINLINE int secretFunc(unsigned int value)
{
    return value ^ 0x5c;
}

VMAction showInstruction(VMInstanceRef vm, GPRState *gprState, FPRState *fprState, void *data)
{
    // Obtain an analysis of the instruction from the VM
    const InstAnalysis *instAnalysis = qbdi_getInstAnalysis(vm, QBDI_ANALYSIS_INSTRUCTION | QBDI_ANALYSIS_DISASSEMBLY);
    // Printing disassembly
    printf("0x%" PRIRWORD ": %s\n", instAnalysis->address, instAnalysis->disassembly);
    return QBDI_CONTINUE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, // handle to DLL module
                    DWORD fdwReason,    // reason for calling function
                    LPVOID lpvReserved) // reserved
{
    // Perform actions based on the reason for calling.
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);

        if (!AllocConsole())
        {
            printf("Failed to allocate console\n");
            return FALSE; // If console allocation fails, return early.
        }
        const HANDLE out = GetStdHandle(STDOUT_FILENO);
        freopen("CONOUT$", "w", stdout);
        printf("DLL_PROCESS_ATTACH started\n");
        OutputDebugString("hi\n");

        VMInstanceRef vm = NULL;
        uint8_t *fakestack = NULL;

        // init VM
        qbdi_initVM(&vm, NULL, NULL, 0);

        // Get a pointer to the GPR state of the VM
        GPRState *state = qbdi_getGPRState(vm);
        assert(state != NULL);

        // Setup initial GPR state, this fakestack will produce a ret NULL at the end
        // of the execution
        bool res = qbdi_allocateVirtualStack(state, STACK_SIZE, &fakestack);
        assert(res == true);

        const HANDLE moduleBase = GetModuleHandle("Notepad.exe");
        printf("Module base: %p", moduleBase);

        // Add callback on our instruction range
        // uint32_t uid =
        //     qbdi_addCodeRangeCB(vm, (rword)&moduleBase, (rword)&moduleBase + 0x1000, QBDI_PREINST, showInstruction, vm, 0);
        uint32_t uid =
            qbdi_addMemRangeCB(vm, (rword)&moduleBase, (rword)&moduleBase + 0x1000, QBDI_PREINST, showInstruction, vm);

        assert(uid != QBDI_INVALID_EVENTID);

        // add executable code range
        res = qbdi_addInstrumentedModuleFromAddr(vm, (rword)&moduleBase);
        assert(res == true);

        // call secretFunc using VM, custom state and fake stack
        // eq: secretFunc(666);
        rword retval;
        res = qbdi_call(vm, &retval, (rword)secretFunc, 1, 666);
        assert(res == true);

        // get return value from current state
        printf("[*] retval=0x%" PRIRWORD "\n", retval);

        break;

    case DLL_PROCESS_DETACH:

        if (lpvReserved != NULL)
        {
            break; // do not do cleanup if process termination scenario
        }

        // free everything
        qbdi_alignedFree(fakestack);
        qbdi_terminateVM(vm);

        // Perform any necessary cleanup.
        break;
    }

    return TRUE; // Successful DLL_PROCESS_ATTACH.
}
