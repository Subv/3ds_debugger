#include <3ds.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fmt/format.h>

#include "logger.h"
#include "network_manager.h"
#include "utils.h"

static const u32 SocketMemorySize = 0x1000;

NetworkManager::NetworkManager() {
    // Prepare an area of heap memory for the socket shared memory.
    socket_shared_memory = nullptr; //static_cast<u32*>(mappableAlloc(SocketMemorySize));
    //ASSERT(socket_shared_memory != nullptr);

    Logger::Log(fmt::format("Preparing to socInit"));

    // Initialize the socket service (soc:u)
    Result result = socInit(socket_shared_memory, SocketMemorySize);
    Logger::Log(fmt::format("socInit result: {}", result));
}

NetworkManager::~NetworkManager() {
    // Close the server socket if it was open
    if (server_socket != -1)
        close(server_socket);

    // Finalize the socket services
    socExit();

    // Free the shared memory buffer
    mappableFree(socket_shared_memory);
    socket_shared_memory = nullptr;
}

void NetworkManager::Listen(int port) {
    ASSERT(server_socket == -1);
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    ASSERT(server_socket >= 0);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    s32 error = bind(server_socket, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (error < 0) {
        Logger::Log(fmt::format("Socket bind error: {}", error));
        return;
    }

    error = listen(server_socket, 10);
    if (error < 0) {
        Logger::Log(fmt::format("Socket listen error: {}", error));
        return;
    }

    Logger::Log(fmt::format("Listening on port: {}", port));
}

void NetworkManager::Accept() {

}

bool NetworkManager::IsClientConnected() {
    return false;
}

std::string NetworkManager::GetClientIPStr() {
    return "";
}

std::string NetworkManager::ReceiveCommand() {
    return "";
}

void NetworkManager::SendReply(const std::string& command) {

}