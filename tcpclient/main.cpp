#include <iostream>
#include <cstring>
#include <stdexcept>
#include "log.hpp"
#include "socket.hpp"
#include <string>
#include <chrono>
#include <mutex>
#include <cstdint>
#include <array>
#include <algorithm>
#include <limits>


#define PORT 8080
#define BUF_SIZE 1024
#define IP_ADDRESS "127.0.0.1"  //localhost loopback address
DWORD timeout = 5000; // 5 seconds

// Safe way to print to console from multiple active threads
std::mutex consoleMutex;
void printConsole(const std::string& message) {
	// Lock the console output within the scope of this method.
	std::lock_guard<std::mutex> lock(consoleMutex);
	std::cout << message << std::endl;
}

SOCKET sock = INVALID_SOCKET;

using Clock = std::chrono::steady_clock;


std::mutex pingMutex;
std::condition_variable pingResponse;
bool isPingReturned = false;

/*std::string sendMessage(std::string message)
{
	int bytesSent = send(sock, message.c_str(), static_cast<int>(message.size()), 0);

	if (bytesSent == SOCKET_ERROR || bytesSent != message.size()) {
		std::cerr << "Send error!\n";
		return "error";
	}

	char buffer[BUF_SIZE];
	int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
	if (bytes <= 0) {
		std::cerr << "Connection closed or error occurred while receiving data.\n";
		return "error";
	}
	buffer[bytes] = '\0';
	return std::string(buffer);
	//printConsole("Received from server: " + std::string(buffer) + "\n");
}*/

uint32_t nextMessageId = 1;

std::string sendMessage(std::string message)
{
	// Add a unique ID to the message.
	uint32_t messageId = nextMessageId++;

	std::string messageWithId =
		"[C-" + std::to_string(messageId) + "] " + message;

	int bytesSent = send(
		sock,
		messageWithId.c_str(),
		static_cast<int>(messageWithId.size()),
		0
	);

	if (bytesSent == SOCKET_ERROR || bytesSent != messageWithId.size()) {
		std::cerr << "Send error!\n";
		return "error";
	}

	char buffer[BUF_SIZE];



	int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);

	if (bytes == 0) {
		std::cerr << "Server closed the connection.\n";
		return "error";
	}

	if (bytes == SOCKET_ERROR) {

		int error = WSAGetLastError();

		if (error == WSAETIMEDOUT) {
			std::cerr << "Receive timeout: Server did not respond in time.\n";
		}
		else {
			std::cerr << "Receive error.\n";
		}

		return "error";
	}

	buffer[bytes] = '\0';

	return std::string(buffer);
}

void pingServer()
{
	std::string message = "ping";

	// Time the return time
	auto start = Clock::now();
	std::string response = sendMessage(message);
	auto end = Clock::now();

	if (response == "error") {
		std::cout << "Ping failed\n";
		return;
	}

	auto rtt = std::chrono::duration<double, std::milli>(end - start);
	std::cout << "Ping RTT: " << rtt.count() << " ms\n";
}

void checkPacketLoss(int numPackets)
{
	if (numPackets <= 0) {
		numPackets = 1;
	}
	else if (numPackets > 1000) {
		numPackets = 1000;
	}

	int lostPackets = 0;
	double totalPing = 0.0;

	for (int i = 0; i < numPackets; ++i) {
		auto start = Clock::now();

		std::string response = sendMessage("ping");

		auto end = Clock::now();

		if (response == "error") {
			++lostPackets;
		}
		else {
			auto rtt = std::chrono::duration<double, std::milli>(end - start);
			totalPing += rtt.count();
		}
	}

	int successfulPackets = numPackets - lostPackets;

	double packetLossRate = static_cast<double>(lostPackets) / numPackets * 100.0;

	std::cout << "\n--- Ping statistics ---\n";
	std::cout << "Packets sent: " << numPackets << '\n';
	std::cout << "Packets received: " << successfulPackets << '\n';
	std::cout << "Packet loss: " << packetLossRate << "%\n";

	if (successfulPackets > 0) {
		double averagePing = totalPing / successfulPackets;
		std::cout << "Average ping: " << averagePing << " ms\n";
	}
	else {
		std::cout << "Average ping: N/A\n";
	}
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
	std::cout << "TCP Client!\n";
	std::cout << __DATE__ << " " << __TIME__ << std::endl; // log date and time of compilation, not runtime
	LogTargetOperatingSystem();
	LogTargetCompiler();

#ifdef _WIN32
	WSADATA wsaData;
	(void)WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	// Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cerr << "Cannot create socket\n";
		throw std::runtime_error("Cannot create socket");
		return 1;
	}

	// Configure receive timeout (5 seconds)
	DWORD timeout = 5000;

	if (setsockopt(
		sock,
		SOL_SOCKET,
		SO_RCVTIMEO,
		reinterpret_cast<const char*>(&timeout),
		sizeof(timeout)) == SOCKET_ERROR)
	{
		std::cerr << "Failed to set receive timeout\n";
	}


	// Connect to server
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	int result = inet_pton(AF_INET, IP_ADDRESS, &addr.sin_addr);
	if (result <= 0) {
		std::cerr << "Invalid address\n";
		throw std::runtime_error("Invalid address");
		return 1;
	}
	if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		throw std::runtime_error("connect failed");
		std::cerr << "connect failed\n";
		return 1;
	}

	// Start a thread to handle incoming messages
	//std::thread messageHandler(handleIncomingMessages);

	std::string message;

	while (true) {

		std::cout << "Enter a message: ";
		std::getline(std::cin, message);


		// Check if the user wants to disconnect
		if (message == "") {
			continue;
		}
		if (message == "/") {
			std::cout << "Please enter a command\n";
			continue;
		}
		else if (message == "/ping") {
			pingServer();
		}
		else if (message == "/test") {
			checkPacketLoss(10);
		}
		else if (message == "exit") {
			break;
		}
		else
		{
			std::string retrievedMessage = sendMessage(message);
			printConsole("Received from server: " + retrievedMessage + "\n");
		}
		// Send message
		/*if (send(sock, message.c_str(), static_cast<int>(message.size()), 0) == SOCKET_ERROR) {
			std::cerr << "send error!\n";
			throw std::runtime_error("send error!");
		}*/


		// Wait for the returned message
		/*char buffer[BUF_SIZE];
		int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
		auto end = Clock::now();


		buffer[bytes] = '\0';
		std::cout << "Ontvangen van server: " << buffer << "\n";
		*/
	}

	closeSocket(sock);

#ifdef _WIN32
	(void)WSACleanup();
#endif

	return 0;
}
