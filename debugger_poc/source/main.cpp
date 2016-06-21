#include <3ds.h>
#include <stdio.h>
#include <string>

enum CommandIds {
    HELLO_COMMAND_ID      = 0x1,
    BREAK_COMMAND_ID      = 0x2,
    OUTPUT_STR_COMMAND_ID = 0x3,
    SHUTDOWN_COMMAND_ID   = 0x4
};

#define STACKSIZE (4 * 1024)

Thread debugger_thread;
Handle debugger_handle;
bool shutdown_debugger = false;

u32 Pause() {
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & (KEY_A | KEY_B | KEY_UP | KEY_DOWN | KEY_START)) return hidKeysDown();

        gfxFlushBuffers();
        gfxSwapBuffers();

        gspWaitForVBlank();
    }

    return 0;
}

s32 EnableKernelDevMode() {
    volatile u8* debug_mode = ((volatile u8*)0xFFF2D00A);
    *debug_mode = 1;
    return 0;
}

Result HelloCommand(Handle service_handle, u32& param) {
    u32* cmd_buff = getThreadCommandBuffer();
    cmd_buff[0] = IPC_MakeHeader(HELLO_COMMAND_ID, 1, 0);
    cmd_buff[1] = 0x0000CAFE;

    Result result = svcSendSyncRequest(service_handle);
    param = cmd_buff[1];
    return result;
}

Result OutputStrCommand(Handle service_handle, u32& param) {
    u32* cmd_buff = getThreadCommandBuffer();
    cmd_buff[0] = IPC_MakeHeader(OUTPUT_STR_COMMAND_ID, 0, 0);
    Result result = svcSendSyncRequest(service_handle);
    param = cmd_buff[1];
    return result;
}

Result ShutdownCommand(Handle service_handle) {
    u32* cmd_buff = getThreadCommandBuffer();
    cmd_buff[0] = IPC_MakeHeader(SHUTDOWN_COMMAND_ID, 0, 0);
    return svcSendSyncRequest(service_handle);
}

u32 vaddr_bp_control = 0;

void DebuggingThread(void* arg) {
    printf("Thread started\n");

    DebugEventInfo info;
    Result result = 0;

    while (true) {
        result = svcGetProcessDebugEvent(&info, debugger_handle);
        if (info.type == DBG_EVENT_EXCEPTION && info.exception.type == EXC_EVENT_ATTACH_BREAK) {
            svcContinueDebugEvent(debugger_handle, BIT(0) | BIT(1));
            break;
        }
        svcContinueDebugEvent(debugger_handle, BIT(0) | BIT(1));
    }

    // Wait for attach
    while (!shutdown_debugger)
    {
        svcWaitSynchronization(debugger_handle, -1);

        if (shutdown_debugger)
            break;

        result = svcGetProcessDebugEvent(&info, debugger_handle);
        printf("%08X Type %u Thread: %08X unk[0] %08X unk[1] %08X\n", result, info.type, info.thread_id, info.unknown[0], info.unknown[1]);
        if (info.type == 0xB) {
            // Debug string, read the string from the Process at the specified address
            char* data = new char[info.output_string.string_size];
            svcReadProcessMemory(data, debugger_handle, info.output_string.string_addr, info.output_string.string_size);
            printf("OutputString, length %08X, address: %08X str %s\n", info.output_string.string_size, info.output_string.string_addr, data);
            delete[] data;
        }

        if (info.type == 4) {
            // Exception
            printf("Exception type %08X at address %08X\n", info.exception.type, info.exception.address);
            if (info.exception.address == 0x103244) {
                printf("Disabling breakpoint\n");

                u32 virtual_address = 0x1031F4;
                result = svcSetHardwareBreakpoint(1, vaddr_bp_control, virtual_address);
                printf("Hardware breakpoint result %08X\n", result);

                ThreadContext context;
                result = svcGetDebugThreadContext(&context, debugger_handle, info.thread_id, 3);
                printf("PC %08X, LR %08X\n", context.pc, context.lr);
                // There seems to be a bug somewhere in the CPU where a breakpoint will keep on triggering indefinitely even if you disable it.
                // So we just skip the instruction by interpreting it ourselves (It's a B instruction)
                context.pc = 0x103028;
                result = svcSetDebugThreadContext(debugger_handle, info.thread_id, &context, 3);
                printf("Execution is halted, press A to continue it %08X\n", result);
                u32 keys = Pause();
                if (keys & KEY_A) {
                    printf("Continuing with bit 0 and 1\n");
                    result = svcContinueDebugEvent(debugger_handle, BIT(0) | BIT(1));
                }
            } else if (info.exception.address == 0x1031F4) {
                printf("Disabling breakpoint\n");
                svcSetHardwareBreakpoint(1, 0, 0);
                svcSetHardwareBreakpoint(4, 0, 0);
                svcBackdoor(ExecuteKernelBarriers);
                //u32 virtual_address = 0x1031F4;
                //svcSetHardwareBreakpoint(1, vaddr_bp_control, virtual_address);

                ThreadContext context;
                result = svcGetDebugThreadContext(&context, debugger_handle, info.thread_id, 3);
                printf("PC %08X, LR %08X\n", context.pc, context.lr);
                context.pc = 0x103084;
                result = svcSetDebugThreadContext(debugger_handle, info.thread_id, &context, 3);
                printf("Execution is halted, press A to continue it %08X\n", result);
                u32 keys = Pause();
                if (keys & KEY_A) {
                    printf("Continuing with bit 0 and 1\n");
                    result = svcContinueDebugEvent(debugger_handle, BIT(0) | BIT(1));
                }
            }
        } else {
            result = svcContinueDebugEvent(debugger_handle, BIT(0) | BIT(1));
        }
        printf("Continue result: %08X\n", result);

        // Yield
        svcSleepThread(0);
    }

    printf("Exiting thread\n");
    svcContinueDebugEvent(debugger_handle, 0);
}

void WriteBreakInstruction() {
    printf("Setting hardware breakpoint\n");

    // Enable a linked ContextID breakpoint for register 4
    u32 mode = BIT(1) | BIT(2); // Privileged | User
    u32 context_id_bp = BIT(21);
    u32 enable_linking = BIT(20);
    u32 linked_brp = BIT(16); // Register 1
    u32 byte_select = BIT(5) | BIT(6) | BIT(7) | BIT(8);
    svcSetHardwareBreakpoint(4, BIT(0) | context_id_bp | enable_linking | linked_brp | mode | byte_select, debugger_handle);

    printf("Context condition set\n");

    // Enable a linked VAddr breakpoint on register 1
    linked_brp = BIT(18); // Register 4
    u32 virtual_address = 0x103244;
    vaddr_bp_control = (1 * BIT(0)) | mode | enable_linking | linked_brp | byte_select, virtual_address;
    svcSetHardwareBreakpoint(1, vaddr_bp_control, virtual_address);
    printf("Hardware breakpoint set\n");
}

/**
 * Creates a debug object connected to the specified ProcessID
 * and sends a command to the tst:srv service that will generate a fault,
 * then waits on the debug object.
 */
void StartDebuggingSession(Handle service_handle, u32 pid) {
    // We need to patch some kernel value first, otherwise DebugActiveProcess will fail.
    svcBackdoor(EnableKernelDevMode);

    u32 param = 0;
    Result result = 0;

    s32 priority = 0;
    svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);

    printf("Creating a KDebug object\n");

    result = svcDebugActiveProcess(&debugger_handle, pid);
    printf("KDebug object created, result: %08X handle: %08X\n", result, debugger_handle);

    if (result != 0) {
        printf("Error when creating the KDebug object\n");
        return;
    }

    // Now create a thread to handle all debugging breaks
    debugger_thread = threadCreate(DebuggingThread, nullptr, STACKSIZE, priority, 1, false);
    printf("Thread created.\n");

    if (result != 0) {
        printf("Error when generating the fault\n");
        return;
    }

    // Yield the thread
    svcSleepThread(0);
}

int main(int argc, const char* argv[]) {
    gfxInitDefault();
    nsInit();
    consoleInit(GFX_TOP, NULL);

    APT_SetAppCpuTimeLimit(30);

    printf("Initializing, launching process\n");

    u32 pid = 0;
    Result result = NS_LaunchTitle(0x000400000FF3FF00LL, 1, &pid);

    printf("Finished launching, result: %08X. pid = %08X\n", result, pid);

    Pause();

    printf("Opening process pid = %08X\n", pid);

    Handle process = 0;
    result = svcOpenProcess(&process, pid);

    printf("Opened, process handle: %08X result: %08X\n", process, result);

    Pause();

    printf("Attempting to access the created service\n");

    Handle service_handle;
    result = srvGetServiceHandleDirect(&service_handle, "tst:srv");

    printf("Result: %08X handle: %08X\n", result, service_handle);

    Pause();

    printf("Sending hello command\n");

    u32 param = 0;
    result = HelloCommand(service_handle, param);

    printf("Result: %08X, response: %08X\n", result, param);

    Pause();

    printf("Preparing the debugging session\n");

    StartDebuggingSession(service_handle, pid);

    printf("Press A to write a BKPT instruction\n");

    Pause();

    WriteBreakInstruction();

    printf("Press A to trigger breakpoint\n");

    Pause();

    printf("Sending output str command\n");
    result = OutputStrCommand(service_handle, param);
    printf("Result: %08X, response: %08X\n", result, param);

    Pause();

    printf("Sending shutdown command\n");
    
    result = ShutdownCommand(service_handle);

    printf("Result: %08X\n", result);

    svcSleepThread(0);
    svcSleepThread(0);
    svcSleepThread(0);
    svcSleepThread(0);
    svcSleepThread(0);
    svcSleepThread(0);
    svcSleepThread(0);
    svcSleepThread(0);
    svcSleepThread(0);

    shutdown_debugger = true;
    svcBreakDebugProcess(debugger_handle);

    svcCloseHandle(debugger_handle);
    svcCloseHandle(process);

    printf("Debugging session ended\n");

    threadFree(debugger_thread);

    Pause();

    nsExit();
    gfxExit();

    return 0;
}
