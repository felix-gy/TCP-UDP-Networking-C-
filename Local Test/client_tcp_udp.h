#ifndef CLIENT_TCP_UDP_H 
#define CLIENT_TCP_UDP_H

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

class UDPClient {
private:
    int sockfd;
    int bytes;
    struct sockaddr_in server_addr;
    socklen_t addr_len;
    int maxBuffer; // 1000
    char aux_buffer[1001];

public:

    UDPClient(const char* server_ip, int port, int buffer_size = 1000) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) handleError("socket creation failed");

        memset(&server_addr, 0, sizeof(server_addr));

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(server_ip);

        maxBuffer = buffer_size; // 1000

        addr_len = sizeof(server_addr);
    }

    void handleError(const char *message) {
        perror(message);
        exit(EXIT_FAILURE);
    }

    void sendData(const string & message) {
        bytes = sendto(sockfd, message.c_str(), maxBuffer, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
        //cout << "UDPClient sent " << bytes << " bytes to " << inet_ntoa(server_addr.sin_addr) << " : " << ntohs(server_addr.sin_port) << std::endl;
    }   

    string receiveData() {
        bytes = recvfrom(sockfd, aux_buffer, maxBuffer, 0, (struct sockaddr*)&server_addr, &addr_len);
        if (bytes < 0) {
            cerr << "Error receiving data" << endl;
            return "";
        }
        aux_buffer[bytes] = '\0';  // Null-terminate received data
        //cout << "UDPClient received "<< bytes << " bytes from " << inet_ntoa(server_addr.sin_addr) << ":" << ntohs(server_addr.sin_port) << endl;
        return string(aux_buffer);
    }

    ~UDPClient() {
        close(sockfd);
    }
};

class TCPClient {
private:
    int sockfd;
    int bytes;
    struct sockaddr_in server_addr;
    string aux_buffer;

public:
    TCPClient(const char* server_ip, int port) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) handleError("socket creation failed");

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(server_ip);

        if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
            handleError("connection failed");
    }

    void handleError(const char *message) {
        perror(message);
        exit(EXIT_FAILURE);
    }

    // Send
    void sendData(const string & message) {
        bytes = send(sockfd, message.c_str(), message.length(), 0);
        cout << "TCPClient write "<< bytes << " bytes to Server" << sockfd << endl;
    }

    void writeData(const string & message)
    {
        bytes =  write(sockfd, message.c_str(), message.length());
        cout << "TCPClient write "<< bytes << " bytes to Server" << sockfd << endl;
    }
    
    string readData(int socketRead, int numberRead)
    {
        aux_buffer.resize(numberRead);
        bytes = read(socketRead, aux_buffer.data(), numberRead);
        aux_buffer.resize(bytes);
        cout << "TCPServer read"<< bytes << " bytes from Socket " << socketRead << endl;
        return aux_buffer;
    }


    ~TCPClient() {
        close(sockfd);
    }
};

#endif 