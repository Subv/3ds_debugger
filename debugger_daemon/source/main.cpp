#include <string>

#include <3ds.h>

#include <fmt/format.h>

#include "logger.h"
#include "network_manager.h"

const int DebuggerPort = 1234;

int main(int argc, const char* argv[]) {
    Logger::Initialize();

    Logger::Log("Initializing daemon");
    NetworkManager network;

    Logger::Log(fmt::format("Waiting for connection on port {}", DebuggerPort));
    network.Listen(DebuggerPort);

    // TODO(Subv): The following code should be in a loop,
    // we don't want the debugger to exit after the first client disconnects
    network.Accept();

    Logger::Log(fmt::format("Client connected from IP: {}", network.GetClientIPStr()));

    while (network.IsClientConnected()) {
        std::string command = network.ReceiveCommand();
        Logger::Log(fmt::format("Received command {}", command));
        network.SendReply(fmt::format("Received {}\n", command));
    }

    Logger::Finalize();
    return 0;
}
