#ifndef SERVER_TCP_UDP_H 
#define SERVER_TCP_UDP_H

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <set>
#include <algorithm>
#include <unordered_set>

using namespace std;

void handleError(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

// Struct para poder guardar las direcciones de los clientes y trabajar con ellas
struct saveAddress{
    struct sockaddr_in address;
    saveAddress() = default;
    saveAddress(const sockaddr_in & address_)
    {
        this->address = address_;
    };
    bool operator==(const saveAddress& other) const {
        return(address.sin_addr.s_addr == other.address.sin_addr.s_addr);
    }
};

class UDPServer {
private:
    vector <saveAddress> client_vector;
    int sockfd;
    int bytes_read, bytes_sent;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;
public:
    char aux_buffer[1001];
    int maxBuffer; // 1000
    UDPServer(int port, int buffer_size = 1000) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) handleError("socket creation failed");

        memset(&server_addr, 0, sizeof(server_addr));
        memset(&client_addr, 0, sizeof(client_addr));

        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);


        if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            handleError("Bind failed");
        }
        
        addr_len = sizeof(client_addr);

        maxBuffer = buffer_size; // 1000

        cout << "UDPServer listening on port " << port << " - Buffer " <<  maxBuffer<< endl;
        fflush(stdout);
    }
    string receiveData(saveAddress & new_address) {
        bytes_read = recvfrom(sockfd, aux_buffer, maxBuffer, 0, (struct sockaddr*)&client_addr, &addr_len);
        if (bytes_read < 0) {
            cerr << "Error receiving data" << endl;
            return "-1";
        }
        aux_buffer[bytes_read] = '\0';  // Null-terminate received data
        // cout << "UDPServer received "<< bytes_read << " bytes from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << endl;
        
        // Actualizamos new_address con la direccion del cliente que envio mensaje
        new_address.address = client_addr; 

        // Guardar direccion
        saveAddress newClientAddr(client_addr);
        auto it = find(client_vector.begin(), client_vector.end(), newClientAddr);
        if (it == client_vector.end()) { 
            client_vector.push_back(newClientAddr); // Solo guardar nuevas direcciones
        }

        return  string(aux_buffer);
    }

    void sendDataToClient(const string & message, const saveAddress & client_addr_select) {
        bytes_sent = sendto(sockfd, message.c_str(), maxBuffer, 0, (struct sockaddr *)&client_addr_select.address, sizeof(client_addr_select));
        if (bytes_sent < 0) {
            perror("sendto failed");
        } else {
            // cout << "UDPServer sent " << bytes_sent << " bytes to " << inet_ntoa(client_addr_select.address.sin_addr) << " : " << ntohs(client_addr_select.address.sin_port) << std::endl;
        }
    }

    void sendDataToAllClients(const string & message)
    {
        cout << "UDPServer Sending data to all Clients" << endl;
        for (vector<saveAddress>::iterator it = client_vector.begin(); it != client_vector.end(); ++it)
        {
            sendDataToClient(message, *it);
        }
    }
    
    ~UDPServer() {
        close(sockfd);
    }
};

class TCPServer {
private:
    struct sockaddr_in server_addr,client_addr; 
    int sock;
    int addr_len;
    unordered_set<int> client_socket_set;
    int new_socket, bytes;
    string aux_buffer;
public:
    TCPServer(int port) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) handleError("socket failed");

        if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,"1",sizeof(int)) == -1) {
            perror("Setsockopt");
            exit(1);
        }

        memset(&server_addr, 0, sizeof(server_addr));

        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) handleError("bind failed");

        // Backlog
        if (listen(sock, 10) < 0)
            handleError("listen failed");

        addr_len = sizeof(server_addr);
        
        cout << "TCPServer listening on port " << port << endl;

        fflush(stdout);
    }

    int acceptConnection() {
        //new_socket = accept(sock, (struct sockaddr *)&client_addr,(socklen_t*)&addr_len);
        new_socket = accept(sock, NULL,NULL);

        if (new_socket < 0) {
            handleError("accept failed");
            return -1;
        }
        cout << "TCPServer I got a connection from Socket " << new_socket << " - " << inet_ntoa(client_addr.sin_addr) << " : " << ntohs(client_addr.sin_port) << endl;
        
        client_socket_set.insert(new_socket);
        
        return new_socket;
    }

    // Receive
    void receiveData(char * buffer, int buffer_size, int socketRead) {
        bytes = recv(socketRead, buffer, buffer_size, 0);
        if (bytes < 0) {
            cerr << "Error receiving data!" << endl;
            return;
        }
        
        buffer[bytes] = '\0';
        
        cout << "TCPServer received "<< bytes << " bytes from Socket " << sock << endl;
    }
    
    string readData(int socketRead, int numberRead)
    {
        aux_buffer.resize(numberRead);
        bytes = read(socketRead, aux_buffer.data(), numberRead);
        aux_buffer.resize(bytes);
        cout << "TCPServer read"<< bytes << " bytes from Socket " << sock << endl;
        return aux_buffer;
    }

    // Sent
    void sendData(const char* message, int sockWrite) {
        bytes = send(sockWrite, message, strlen(message), 0);
        cout << "TCPServer sent "<< bytes << " bytes to Socket " << sock << endl;
    }

    void writeData(const string & message, int sockWrite)
    {
        bytes =  write(sockWrite, message.c_str(), message.size());
        cout << "TCPServer write "<< bytes << " bytes to Socket " << sock << endl;
    }

    void sendDataAll(const char* message)
    {
        for (unordered_set<int>::iterator it = client_socket_set.begin(); it != client_socket_set.end(); ++it)
        {
            sendData(message, *it);
        }
    }

    ~TCPServer() {
        
        for (unordered_set<int>::iterator it = client_socket_set.begin(); it != client_socket_set.end(); ++it)
        {
            close(*it);
        }
        close(sock);
    }
};

#endif