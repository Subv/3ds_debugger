#include <3ds.h>
#include <stdio.h>
#include <string>

enum CommandIds {
    HELLO_COMMAND_ID      = 0x1,
    BREAK_COMMAND_ID      = 0x2,
    OUTPUT_STR_COMMAND_ID = 0x3,
    SHUTDOWN_COMMAND_ID   = 0x4
};

void Pause() {
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_A) break;

        gfxFlushBuffers();
        gfxSwapBuffers();

        gspWaitForVBlank();
    }
}

int main(int argc, const char* argv[]) {
    gfxInitDefault();
    nsInit();
    consoleInit(GFX_TOP, NULL);

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

    u32* cmd_buff = getThreadCommandBuffer();
    cmd_buff[0] = IPC_MakeHeader(HELLO_COMMAND_ID, 1, 0);
    cmd_buff[1] = 0x0000CAFE;

    result = svcSendSyncRequest(service_handle);

    printf("Result: %08X, response: %08X\n", result, cmd_buff[1]);

    Pause();

    printf("Sending shutdown command\n");
    
    cmd_buff[0] = IPC_MakeHeader(SHUTDOWN_COMMAND_ID, 0, 0);

    result = svcSendSyncRequest(service_handle);

    printf("Result: %08X\n", result);

    Pause();

    nsExit();
    gfxExit();

    return 0;
}
