#include <libloaderapi.h>
#include <windows.h>

#include "QBDI/VM_C.h"
#include <assert.h>
#include <processenv.h>
#include <stdbool.h>
#include <stdio.h>

#include <QBDI.h>

#define STACK_SIZE 0x100000

VMAction showSyscallInstruction(VMInstanceRef vm, GPRState *gprState, FPRState *fprState, void *data)
{
    // Obtain an analysis of the instruction from the VM
    const InstAnalysis *instAnalysis = qbdi_getInstAnalysis(vm, QBDI_ANALYSIS_INSTRUCTION | QBDI_ANALYSIS_DISASSEMBLY);

    // Check if the current instruction is a syscall (opcode: 0x0F 0x05 for x86_64)
    // if (instAnalysis->disassembly && strstr(instAnalysis->disassembly, "syscall"))
    // {
    printf("Syscall detected at 0x%" PRIRWORD ": %s\n", instAnalysis->address, instAnalysis->disassembly);
    // }

    return QBDI_CONTINUE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
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

        VMInstanceRef vm = NULL;
        uint8_t *fakestack = NULL;

        // Init VM
        qbdi_initVM(&vm, NULL, NULL, 0);

        // Get a pointer to the GPR state of the VM
        GPRState *state = qbdi_getGPRState(vm);
        assert(state != NULL);

        // Setup initial GPR state, this fakestack will produce a ret NULL at the end
        // of the execution
        bool res = qbdi_allocateVirtualStack(state, STACK_SIZE, &fakestack);
        assert(res == true);

        const HANDLE ntdllMod = GetModuleHandle("ntdll.dll");
        const PVOID pNtCreateFile = GetProcAddress(ntdllMod, "NtCreateFile");
        printf("Ntdll module: %p\n", ntdllMod);
        printf("NtCreateFile: %p\n", pNtCreateFile);

        // Add callback for syscall detection across the whole code range
        uint32_t uid = qbdi_addCodeCB(vm, QBDI_PREINST, showSyscallInstruction, vm, 0);
        assert(uid != QBDI_INVALID_EVENTID);

        qbdi_addInstrumentedRange(vm, (rword)pNtCreateFile, (rword)pNtCreateFile+16);

        // Add executable code range
        res = qbdi_addInstrumentedModuleFromAddr(vm, (rword)ntdllMod);
        assert(res == true);

        break;

    case DLL_PROCESS_DETACH:
        if (lpvReserved != NULL)
        {
            break; // Do not do cleanup if process termination scenario
        }

        // Free everything
        qbdi_alignedFree(fakestack);
        qbdi_terminateVM(vm);

        // Perform any necessary cleanup.
        break;
    }

    return TRUE; // Successful DLL_PROCESS_ATTACH.
}
