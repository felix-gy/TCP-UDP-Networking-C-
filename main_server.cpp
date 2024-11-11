#include "server_tcp_udp.h"

#include <thread>
#include <fstream>
#include <sstream> 
#include <iostream>
#include <iomanip>

bool running = 1;

// UDP
void server_read_UDP(UDPServer & server)
{
    saveAddress Address_update;
    string buffer(1000,'\0');
    int seq_num = 0;
    stringstream ss;

    while (running)
    {
        buffer = server.receiveData(Address_update);
        ss.str("");
        seq_num = stoi(buffer.substr(server.maxBuffer - 8,  8));
        ss << "ACK"  << setw(1000 - 11) << setfill('#') << "#" << setw(8) << setfill('0') << seq_num;
        server.sendDataToClient(ss.str(), Address_update);
    }

} 

// TCP
/*
void server_accept_TCP(TCPServer & server)
{
    vector<thread> thread_vec;
    int connection;
    while (running)
    {
        connection = server.acceptConnection();
        if (connection == -1)
        {
            continue;
        }
        
    }
} 
*/

int main()
{
    int c;
    // SERVER - PORT 8080  
    // UDP
    UDPServer main_server(8080, 1000);
    thread (server_read_UDP, ref(main_server)).detach();
    // TCP
    //TCPServer main_server(9090);
    //thread (server_accept_TCP, ref(main_server)).detach();
    do
    {
        cout << "(0 to quit):" << endl;
        cin >> c;
        cout << c << endl;
    } while ( c != 0);

    running = 0;

    fflush(stdout);
    return 0;
}