#ifdef UDP_FORMAT_MSG_H
#define UDP_FORMAT_MSG_H

#include <iostream>
#include <string>
using namespace std;

struct MensajeDesencapsulado
{
    const int longitud;
    string mensaje;
    int checksum;
    int offset;
    bool flag;
    char tipo;
    int tamaño_padding;
    size_t posicion;

    MensajeDesencapsulado(const string & mensaje_encapsulado, const int longitud_fija = 1000)
    {
        longitud_fija = longitud_fija;

        // Obtener el tamaño del padding desde los últimos 4 caracteres
        tamaño_padding = stoi(mensaje_encapsulado.substr(longitud_fija - 4, 4));

        // Obtener el checksum desde los últimos 11 caracteres (excluyendo el padding)
        checksum = stoi(mensaje_encapsulado.substr(longitud_fija - 7, 3));

        // Obtener el offset desde los últimos 14 caracteres (excluyendo el checksum y el padding)
        offset = stoi(mensaje_encapsulado.substr(longitud_fija - 14, 7));
        
        // Obtener el flag 
        flag = (mensaje_encapsulado[longitud_fija - 15] == '1');
        
        // Obtener el tipo
        tipo = mensaje_encapsulado[longitud_fija - 16];
        
        // Obtener el mensaje eliminando el padding añadido
        mensaje = mensaje_encapsulado.substr(0, longitud_fija - 16 - tamaño_padding);

        posicion = 0;
    }

    string leerEspacios(int longitud) {
        if (posicion + longitud > mensaje.size()) {
            longitud = mensaje.size() - posicion; // Ajustar longitud si excede el tamaño del mensaje
        }

        string resultado = mensaje.substr(posicion, longitud);
        posicion += longitud;
        return resultado;
    }
    
    int calcularChecksum() const {
        int suma = 0;
        for (char c : mensaje) {
            suma += static_cast<unsigned char>(c);
        }
        return suma % 256;
    }
    
    bool verificarChecksum() const {
        return calcularChecksum() == checksum;
    }


    void print()
    {
        cout << "Mensaje: " << mensaje << endl;
        cout << "Checksum: " << checksum << endl;
        cout << "Offset: " << offset << endl;
        cout << "Flag: " << flag<< endl;
        cout << "Tipo: " << tipo << endl;
        cout << "Tamaño Padding: " << tamaño_padding << endl;
    }
};



#endif