#include <string>

#include <3ds.h>

#include "network_manager.h"
#include "logger.h"

const int DebuggerPort = 1234;

int main(int argc, const char* argv[]) {
    Logger::Initialize();

    Logger::Log("Initializing daemon");
    NetworkManager network;

    Logger::Log("Waiting for connection on port %u", DebuggerPort);
    network->Listen(DebuggerPort);

    // TODO(Subv): The following code should be in a loop,
    // we don't want the debugger to exit after the first client disconnects
    network->Accept();

    Logger::Log("Client connected from IP: %s", network->GetClientIPStr().c_str());

    while (network->IsClientConnected()) {
        std::string command = network->ReceiveCommand();
        Logger::Log("Received command %s", command.c_str());
        network->SendReply("Received %s\n", command.c_str());
    }

    Logger::Finalize();
    return 0;
}
