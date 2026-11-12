#include <iostream>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>
#include "log.hpp"
#include "socket.hpp"
#include <string>

#define PORT 8080
#define BUF_SIZE 1024
constexpr int BACKLOG = 5;


std::mutex consoleMutex;
void printConsole(const std::string& message) {
	// Lock the console output within the scope of this method.
	std::lock_guard<std::mutex> lock(consoleMutex);
	std::cout << message << std::endl;
}

// Client handler method
void HandleClient(SOCKET clientSock) {
	char buffer[BUF_SIZE];
	int bytesReceived = 0;

	while ((bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0)) > 0) {

		{
			std::lock_guard<std::mutex> lock(consoleMutex);

			std::cout << "Client " << clientSock << ": ";
			std::cout.write(buffer, bytesReceived);
			std::cout << '\n';
			//printConsole("Received " + std::to_string(bytesReceived) + " bytes from client " + std::to_string(clientSock));
		}

		// Echo the received data back to the client.
		/*if (send(clientSock, buffer, bytesReceived, 0) == SOCKET_ERROR) {
			printConsole("Send error for client " + std::to_string(clientSock));
			break;
		}*/

		// send a message back
		buffer[bytesReceived] = '\0';
		std::string returnMessage = std::string(buffer) + " ontvangen";
		if (send(clientSock, returnMessage.c_str(), static_cast<int>(returnMessage.size()), 0) == SOCKET_ERROR) {
			printConsole("Send error for client " + std::to_string(clientSock));
			break;
		}

	}

	if (bytesReceived == 0) {
		printConsole("Client " + std::to_string(clientSock) + " disconnected.");
	}
	else if (bytesReceived == SOCKET_ERROR) {
		std::lock_guard<std::mutex> lock(consoleMutex);
		std::cerr << "Receive error for client " << clientSock << '\n';
	}


	closeSocket(clientSock);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
	std::cout << "TCP Server!\n";
	std::cout << __DATE__ << " " << __TIME__ << std::endl; // log date and time of compilation, not runtime
	LogTargetOperatingSystem();
	LogTargetCompiler();
#ifdef _WIN32
	WSADATA wsaData;
	(void)WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSock == INVALID_SOCKET) {
		std::cerr << "Cannot create socket!\n";
		throw std::runtime_error("Cannot create socket");
		return 1;
	}

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		closeSocket(serverSock);
		std::cerr << "Cannot bind socket\n";
		throw std::runtime_error("Cannot bind socket");
		return 1;
	}

	std::cout << "TCP Server listening on port " << PORT << "...\n";

	if (listen(serverSock, BACKLOG) == SOCKET_ERROR) {
		closeSocket(serverSock);
		std::cerr << "listen error!\n";
		throw std::runtime_error("listen error!");
		return 1;
	}

	std::vector<std::thread> clientThreads;

	while (true) {
		SOCKET clientSock = accept(serverSock, nullptr, nullptr);

		if (clientSock == INVALID_SOCKET) {
			std::lock_guard<std::mutex> lock(consoleMutex);
			std::cerr << "Accept error!\n";
			break;
		}

		printConsole("New client connected: " + std::to_string(clientSock));


		clientThreads.emplace_back(HandleClient, clientSock);
	}

	// Close the listening socket.
	closeSocket(serverSock);

	// TODO Add activeClients
	/*for (SOCKET client : activeClients) {
		interruptSocket(client);
	}*/

	// Wait for all client threads to finish.
	for (std::thread& clientThread : clientThreads) {
		clientThread.join();
	}



	/*SOCKET clientSock = accept(serverSock, nullptr, nullptr);
	if (clientSock == INVALID_SOCKET) {
		std::cerr << "accept error!\n";
		throw std::runtime_error("accept error!");
		return 1;
	}

	char buffer[BUF_SIZE];
	int bytes = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
	buffer[bytes] = '\0';

	std::cout << "Ontvangen: " << buffer << "\n";

	if (send(clientSock, message, static_cast<int>(strlen(message)), 0) == SOCKET_ERROR) {
		std::cerr << "send error!\n";
		throw std::runtime_error("send error!");
		closeSocket(clientSock);
		closeSocket(serverSock);
		return 1;
	}*/

#ifdef _WIN32
	(void)WSACleanup();
#endif

	return 0;
}

// Helper method for turning off sockets
void interruptSocket(SOCKET sock) {
#ifdef _WIN32
	shutdown(sock, SD_BOTH);
#else
	shutdown(sock, SHUT_RDWR);
#endif
}

