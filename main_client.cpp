#include "client_tcp_udp.h"
#include "udp_format_msg.h"

#include <thread>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <chrono>
#include <ctime>

// using namespace std;

bool running = 1;
int pipelined = 0;


string prepararMensaje(const string& mensaje, int offset, bool flag, char tipo, const int longitud_fija = 1000, const char padding_char = '#') {

    // Calcular el tamaño del padding
    int tamaño_padding = longitud_fija - mensaje.size() - 16; // 1000 menos el mensaje y menos los otros campos
    string padding(tamaño_padding, padding_char);

    // Calcular el checksum
    int checksum = 0;
    for (char c : mensaje) {
        checksum += static_cast<unsigned char>(c);
    }
    checksum %= 256;
    
    // Crear el stringstream para construir el mensaje final
    ostringstream ss;

    // Añadir el mensaje y el padding
    ss << mensaje << padding;

    // Añadir tipo
    ss << tipo;

    // Añadir flag
    ss << (flag ? '1' : '0');
    
    // Añadir offset
    ss << setw(7) << setfill('0') << offset;
    
    // Añadir checksum
    ss << setw(3) << setfill('0') << checksum;

    // Añadir el tamaño del padding
    ss << setw(4) << setfill('0') << tamaño_padding;

    return ss.str();
}

struct saveTime
{
    chrono::system_clock::time_point send_time;
    chrono::system_clock::time_point receive_time;
    int id;

    // Default constructor
    saveTime() : id(0), send_time(), receive_time() {}

    // Constructor for setting up time points and sequence number
    saveTime(int id_, const std::chrono::system_clock::time_point &send)
    {
        id = id_;
        send_time = send; // Placeholder; set receive_time later when actual receive happens
    }

    void setReceiveTime(const chrono::system_clock::time_point &receive)
    {
        receive_time = receive;
    }
    // Member function to print send and receive times
    void printTimes() const
    {
        auto printTimePoint = [](const chrono::system_clock::time_point &tp)
        {
            auto in_time_t = chrono::system_clock::to_time_t(tp);
            auto ms = chrono::duration_cast<chrono::milliseconds>(tp.time_since_epoch()) % 1000;

            cout << put_time(localtime(&in_time_t), "%Y-%m-%d %H:%M:%S")
                 << '.' << setfill('0') << setw(3) << ms.count();
        };

        cout << "ID: " << id << endl;
        cout << "Send time: ";
        printTimePoint(send_time);
        cout << endl;

        cout << "Receive time: ";
        printTimePoint(receive_time);
        cout << endl;

        auto diff = receive_time - send_time;
        cout << "Round trip time: " << chrono::duration<double, milli>(diff).count() << " ms" << endl;
    }
};

unordered_map<int, saveTime> time_map;

bool sendFile(const string filepath, UDPClient &client)
{
    pipelined = 0; // Reset pipelined count

    ifstream file(filepath, ios::binary);
    string string_buffer(1000, '\0');
    stringstream ss;

    if (file)
    {
        // Msg to start to send file [ F0000filename ]
        ss << "F" << setw(4) << setfill('0') << filepath.size() << filepath;
        // Preparar mensaje deja 984 caracteres para el archivo y 16 para los metadatos
        string_buffer = prepararMensaje(ss.str(), 0, true, 'S', 1000, '#');
        client.sendData(string_buffer); // Send start message
        // Send All file
        char *buffer = new char[1000 - 16]; // 984
        for (int i = 1; file.read(buffer, 1000 - 16) || file.gcount() > 0; i++)
        {
            if (pipelined < 10)
            {
                string_buffer = prepararMensaje(string(buffer, file.gcount()), i, (i == 1) ? true : false, 'D', 1000, '#');
                client.sendData(string_buffer);
                pipelined++; // Increment pipelined count
            }
        }
        client.sendData(prepararMensaje("E", 0, true, 'S', 1000, '#')); // Send end message
        delete[] buffer;
        file.close();
        return true;
    }
    else
    {
        cout << "Error al abrir el archivo" << endl;
        return false;
    }
}

void readUDP(UDPClient &client)
{
    string buffer(1000, '\0');
    int seqNum = 0;
    map<int, char *> data_map;
    while (running)
    {
        buffer = client.receiveData();
        seqNum = stoi(buffer.substr(1000 - 8, 8));
        time_map[seqNum].setReceiveTime(chrono::system_clock::now());
        /// cout << "Recibido: " << buffer.substr(0,3) << " " << stoi(buffer.substr(1000-8, 8))<< endl;
        pipelined--;
    }
}

void sendUDP(UDPClient &client)
{
    string buffer(1000, '\0');
    int seq_num = 1;
    stringstream ss;


    int test = 0;
    while (running)
    {
        ss.str("");
        if (pipelined < 2)
        {
            cout << endl;
            ss << "D" << setw(1000 - 9) << setfill('#') << "#" << setw(8) << setfill('0') << seq_num;
            time_map[seq_num] = saveTime(seq_num, chrono::system_clock::now());
            client.sendData(ss.str());
            pipelined++;
            if (test == 0)
            {
                thread(readUDP, ref(client)).detach();
            }
        }
        seq_num++;
        test++;
    }
}

int main()
{
    int c;
    UDPClient main_client("127.0.0.1", 8080, 1000);
    thread(sendUDP, ref(main_client)).detach();
    do
    {
        cout << "(Send 0 to quit):" << endl;
        cin >> c;
        cout << c << endl;
    } while (c != 0);

    for (auto &it : time_map)
    {
        it.second.printTimes();
    }

    fflush(stdout);
    return 0;
}