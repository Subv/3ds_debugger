#include <3ds.h>
#include <stdio.h>
#include <string>
#include <string.h>

#define CONCAT2(x, y) DO_CONCAT2(x, y)
#define DO_CONCAT2(x, y) x ## y

#define variable_name(var) CONCAT2(var, __LINE__)

#define fprintf(...) { u8* variable_name(savedbuff) = my_printf_guard(); fprintf(__VA_ARGS__); my_printf_restore(variable_name(savedbuff)); }

u8* my_printf_guard() {
    // Saves the contents of the TLS command buffer and returns them in a variable
    // The caller is responsible for the management of the returned memory.
    // my_printf_restore deletes this memory automatically.
    // This is needed because printf corrupts the TLS+0x80 area.
    u8* tls = new u8[0x200];
    memcpy(tls, getThreadCommandBuffer(), 0x200);

    return tls;
}

void my_printf_restore(u8* tls) {
    memcpy(getThreadCommandBuffer(), tls, 0x200);
    delete[] tls;
}

enum CommandIds {
    HELLO_COMMAND_ID      = 0x1,
    BREAK_COMMAND_ID      = 0x2,
    OUTPUT_STR_COMMAND_ID = 0x3,
    SHUTDOWN_COMMAND_ID   = 0x4
};

int main(int argc, const char* argv[]) {
    sdmcInit();
    FILE* log_file = fopen("service_log.log", "a");

    Handle handles[] = { 0 /* Server port */, 0 /* Server session */};
    bool shutdown = false;
    s32 index = 0;
    int num_handles = 1;
    
    u32* cmd_buff = getThreadCommandBuffer();

    fprintf(log_file, "Registering service\n");

    // Register a dummy service
    Result result = srvRegisterService(&handles[0], "tst:srv", 1);
    if (result != 0) {
        fprintf(log_file, "Could not register the service: %08X\n", (u32)result);
        goto exit;
    }

    fprintf(log_file, "Service registered, starting wait loop\n");

    cmd_buff[0] = 0xFFFF0000;
    do {
        fprintf(log_file, "Command1: %08X\n", cmd_buff[0]);
        fprintf(log_file, "Command2: %08X\n", cmd_buff[0]);
        Result result = svcReplyAndReceive(&index, handles, shutdown ? 0 : num_handles, handles[1]);
        fprintf(log_file, "Command3: %08X\n", cmd_buff[0]);

        fprintf(log_file, "ReplyAndReceive returned index %08X shutting down: %s\n", index, shutdown ? "yes" : "no");

        if (shutdown)
            break;

        if (result != 0) {
            fprintf(log_file, "svcReplyAndReceive error: %08X\n", (u32)result);
            break;
        }

        if (index >= num_handles) {
            fprintf(log_file, "Index error %08X\n", index);
            break;
        }

        switch (index) {
            case 0: // The ServerPort was signaled
                // Accept the new ServerSession
                result = svcAcceptSession(&handles[1], handles[0]);
                if (result != 0) {
                    fprintf(log_file, "svcAcceptSession error: %08X\n", (u32)result);
                    goto cleanup;
                }
                fprintf(log_file, "Accepted a new session\n");
                num_handles++;
                break;
            case 1: { // The ServerSession was signaled
                // Handle the command
                u32 command = (cmd_buff[0] >> 16) & 0xFFFF;
                switch (command) {
                    case HELLO_COMMAND_ID:
                        fprintf(log_file, "Hello, received %08X\n", cmd_buff[1]);
                        cmd_buff[0] = IPC_MakeHeader(HELLO_COMMAND_ID, 1, 0);
                        cmd_buff[1] = 0x0000BABE;
                        break;
                    case SHUTDOWN_COMMAND_ID:
                        fprintf(log_file, "Shutting down the server.\n");
                        cmd_buff[0] = IPC_MakeHeader(SHUTDOWN_COMMAND_ID, 0, 0);
                        shutdown = true;
                        break;
                    default:
                        fprintf(log_file, "Unknown command %08X\n", command);
                        break;
                }
                break;
            }
            default:
                fprintf(log_file, "Unknown index signaled: %08X\n", index);
                break;
        }
    } while (true);

    fprintf(log_file, "Exiting process\n");

    cleanup:
    srvUnregisterService("tst:srv");

    exit:
    if (log_file)
        fclose(log_file);

    sdmcExit();
    svcExitProcess();
    return 0;
}
