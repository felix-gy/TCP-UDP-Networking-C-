#include "client_tcp_udp.h"
#include "udp_format_msg.h"

#include <thread>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <cmath>
// using namespace std;

bool running = 1;
int pipelined = 0;

size_t size_cout;
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
    // Función auxiliar para formatear un tiempo en string
    static string formatTimePoint(const chrono::system_clock::time_point &tp)
    {
        ostringstream oss;
        auto in_time_t = chrono::system_clock::to_time_t(tp);
        auto ms = chrono::duration_cast<chrono::milliseconds>(tp.time_since_epoch()) % 1000;

        oss << put_time(localtime(&in_time_t), "%Y-%m-%d %H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }

    // Función para obtener el tiempo de envío como string
    string getSendTime() const
    {
        return formatTimePoint(send_time);
    }

    // Función para obtener el tiempo de recepción como string
    string getReceiveTime() const
    {
        return formatTimePoint(receive_time);
    }

    // Función para obtener la diferencia de tiempo como string
    string getRoundTripTime() const
    {
        if (receive_time != chrono::system_clock::time_point()) {
            auto diff = receive_time - send_time;
            std::ostringstream oss;
            oss << chrono::duration<double, milli>(diff).count() << " ms";
            return oss.str();
        } else {
            return "Round trip time not available (no receive time)";
        }
    }

    string getLogEntry() const
    {
        ostringstream oss;
        oss << "ID: " << id << "\n";
        oss << "Send time: " << getSendTime() << "\n";
        oss << "Receive time: " << getReceiveTime() << "\n";
        oss << "Round trip time: " << getRoundTripTime() << "\n";
        return oss.str();
    }
};

map<int, saveTime> time_map;

void savMapTime(string filename)
{
    string output_filename = filename + "_output.txt";
    ofstream outputFile(output_filename);
    if (!outputFile) {
        cerr << "Error opening file for writing" << std::endl;
        return;
    }
    
    for (const auto &it : time_map)
    {
        outputFile << it.second.getLogEntry();
    }
    outputFile.close();
}

void readUDP(UDPClient &client)
{
    string buffer(1000, '\0');
    int seqNum = 0;
    map<int, char *> data_map;
    while (running)
    {
        buffer = client.receiveData();
        if (buffer.substr(0,3) == "ACK"){

            seqNum = stoi(buffer.substr(1000 - 8, 8));
            time_map[seqNum].setReceiveTime(chrono::system_clock::now());
            cout << time_map[seqNum].getReceiveTime() << " Recibido: " << buffer.substr(0,3) << seqNum  << " " << time_map[seqNum].getRoundTripTime()  << endl;
            pipelined--;
        }
    }
}


bool sendFile(const string filepath, UDPClient &client)
{
    bool flag = true;
    pipelined = 0; // Reset pipelined count
    ifstream file(filepath, ios::binary);
    string string_buffer(1000, '\0');
    stringstream ss;

    if (file)
    {
        // Msg to start to send file [ F0000filename ]
        // ss << "F" << setw(4) << setfill('0') << filepath.size() << filepath;
        // Preparar mensaje deja 984 caracteres para el archivo y 16 para los metadatos
        // string_buffer = prepararMensaje(ss.str(), 0, true, 'S', 1000, '#');
        // client.sendData(string_buffer); // Send start message
        // Send All file
        const int chunk_size = 984;
        char *buffer = new char[1000 - 16]; // 984

        // Obtener tamaño del archivo
        file.seekg(0, ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, ios::beg);

        // Calcular número total de paquetes
        size_cout = ceil(static_cast<double>(file_size) / chunk_size);
        

        for (int i = 1; file.read(buffer, 1000 - 16) || file.gcount() > 0; i++)
        {
            if (pipelined < 10)
            {
                string_buffer = prepararMensaje(string(buffer, file.gcount()), i, (i == 1) ? true : false, 'D', 1000, '#');
                time_map[i] = saveTime(i, chrono::system_clock::now());
                client.sendData(string_buffer);
                pipelined++; // Increment pipelined count
                if (flag)
                {
                    thread(readUDP, ref(client)).detach();
                    flag = false;
                }
                
            }
        }
        // client.sendData(prepararMensaje("E", 0, true, 'S', 1000, '#')); // Send end message
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
/*
void sendUDP(UDPClient &client)
{
    string buffer(1000, '\0');
    int seq_num = 1;
    stringstream ss;
    int test = 0;
    while (running)
    {
        string filename;
        cout << "Ingrese el nombre del archivo a enviar: " << endl;
        cin >> filename;
        getline(cin, filename);  
        if (sendFile(filename, client))
        {
            cout << "Archivo enviado" << endl;
            cout << "Chunk necearios " << size_cout << endl;
            cin >> test;
            
            savMapTime(filename);
        }
        time_map.clear();
    }
}

*/
int main()
{
    int c;
    UDPClient main_client("4.201.139.31", 8080, 1000);
    //thread(sendUDP, ref(main_client)).detach();
    
    string buffer(1000, '\0');
    stringstream ss;
    bool flag = true;
    
    string filename;


    //time_map.clear();
    do
    {
        cout << "Ingrese el nombre del archivo a enviar: " << endl;
        getline(cin, filename);  
        cout << filename << endl;
        if (sendFile(filename, main_client))
        {
            flag = false;
            cout << "Archivo enviado" << endl;
            cout << "Coloque - Enter o otro - para cerrra el programa y crear un txt con los tiempos de cada paquete asta este momento " << endl;
            cin >> c;
            savMapTime(filename);
            cout << "Tiempos guardados " << endl;
            cout << "Chunk necearios: " << size_cout << endl;
            cout << "Paquetes perdidos: " << size_cout - time_map.size() << endl;
            time_map.clear();
        }
    } while (flag);
    /*
    for (auto &it : time_map)
    {
        cout << it.first << " ";
    }
    */
    fflush(stdout);
    return 0;
}