/* Server code in C */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream> 
#include <thread>   
#include <map>
#include <string>
#include <vector>
#include <iomanip>

using namespace std;



int date = 45003;
map<string,int> mapSockets;
map<string,char> jugadores;

vector<vector<char>> tablero(3, vector<char>(3, ' '));

char cambio(char x) {
    if (x == 'X') return 'O';
    if (x=='O') return 'X';
    return ' ';
}

bool marcar(int posicion, char ficha) {
    
    if (posicion < 1 || posicion > 9) {
        return false; 
    }
    
    int fila = (posicion - 1) / 3; 
    int columna = (posicion - 1) % 3; 
    
    if (tablero[fila][columna] != ' ') {
        return false; 
    }

    tablero[fila][columna] = ficha;
    return true;
}

int cSock(char x) {
    for(auto it:jugadores) {
        if(it.second == x) {
            return mapSockets[it.first];
        }
    }
    return -1;
}

string convertir() {
    string resultado = "";
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            resultado += tablero[i][j];
        }
    }
    return resultado;
}

pair<int, string> comprobarEstado() {
    for (int i = 0; i < 3; i++) {
        if (tablero[i][0] == tablero[i][1] && tablero[i][1] == tablero[i][2] && tablero[i][0] != ' ') {
            return {1, string(1, tablero[i][0])};
        }
    }

    for (int i = 0; i < 3; i++) {
        if (tablero[0][i] == tablero[1][i] && tablero[1][i] == tablero[2][i] && tablero[0][i] != ' ') {
            return {1, string(1, tablero[0][i])}; 
        }
    }

    if (tablero[0][0] == tablero[1][1] && tablero[1][1] == tablero[2][2] && tablero[0][0] != ' ') {
        return {1, string(1, tablero[0][0])}; 
    }
    if (tablero[0][2] == tablero[1][1] && tablero[1][1] == tablero[2][0] && tablero[0][2] != ' ') {
        return {1, string(1, tablero[0][2])}; 
    }

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (tablero[i][j] == ' ') {
                return {0, ""}; 
            }
        }
    }

    return {2, "Empate"}; 
}



void readSocketThread(int cliSocket, string nickname)
{
    string buffer;
    int n;

    auto leerNBytes = [&](int cantidad) -> std::string {
        std::string resultado(cantidad, '\0');
        int leidos = 0;
        while (leidos < cantidad) {
            int nActual = read(cliSocket, &resultado[leidos], cantidad - leidos);
            if (nActual <= 0) {
                throw std::runtime_error("Error leyendo del socket o conexión cerrada al intentar leer " + std::to_string(cantidad) + " bytes");
            }
            leidos += nActual;
        }
        cout << "resultadoBuffer = " << resultado << endl;
        return resultado;
    };
    

    while(1) {
        string sizeStr = leerNBytes(5);
        if (sizeStr.empty()) break;

        int tamano = stoi(sizeStr);
        string tipo = leerNBytes(1);
        if (tipo.empty()) break;

    if (tipo == "L"){
      
        string msg;
        for (auto it = mapSockets.cbegin(); it != mapSockets.cend(); ++it) {
            if (it != mapSockets.cbegin()) msg += ",";
            msg += it->first;
        }

        ostringstream oss;
        oss << setw(5) << setfill('0') << (msg.length() + 1) << 'l' << msg;
        string mensaje = oss.str();

        write(mapSockets[nickname], mensaje.c_str(), mensaje.size());

    }
    else if (tipo == "M") {
        string msgLenStr = leerNBytes(5);
        int msgLen = stoi(msgLenStr);
        std::string msg = leerNBytes(msgLen);

        string dstLenStr = leerNBytes(5);
        int dstLen = stoi(dstLenStr);
        string destinatario = leerNBytes(dstLen);
  
        cout << "Mensaje de " << nickname << " para " << destinatario << ": " << msg << "\n";
        
        if (auto it = mapSockets.find(destinatario); it != mapSockets.end()) {
            ostringstream oss;
            oss << setw(5) << setfill('0') << (msgLen + nickname.length() + 11)
                << 'm'
                << setw(5) << msgLen << msg
                << setw(5) << nickname.length() << nickname;
            string envio = oss.str();
            write(it->second, envio.c_str(), envio.size());
        } else {
            cerr << "No se encontró el destinatario " << destinatario << "\n";
        }
    }
    else if (tipo == "B"){
        string msgLenStr = leerNBytes(5);
        int msgLen = stoi(msgLenStr);
        string msg = leerNBytes(msgLen);

        ostringstream oss;
        oss << setw(5) << setfill('0') << (msgLen + nickname.length() + 11)
            << 'b'
            << setw(5) << msgLen << msg
            << setw(5) << nickname.length() << nickname;
        string envio = oss.str();

        for (const auto& [_, sock] : mapSockets) {
            write(sock, envio.c_str(), envio.size());
        }
    } else if(tipo == "Q") {
      
        mapSockets.erase(nickname);
        cout << nickname << " se desconectó.\n";
        break;

    } else if (tipo == "F") {
        string dstLenStr = leerNBytes(5);
        int dstLen = stoi(dstLenStr);
        string destinatario = leerNBytes(dstLen);
    
        string fileNameLenStr = leerNBytes(5);
        int fileNameLen = stoi(fileNameLenStr);
        string nombreArchivo = leerNBytes(fileNameLen);
    
        string tamArchivoStr = leerNBytes(15);
        int tamArchivo = stoi(tamArchivoStr);
    
        string contenido = leerNBytes(tamArchivo);
    
        string hashLenStr = leerNBytes(5);
        int hashLen = stoi(hashLenStr);
        string hash = leerNBytes(hashLen);
    
        cout << "Archivo recibido de " << nickname << " para " << destinatario
             << ": " << nombreArchivo << " (" << tamArchivo << " bytes)\n";
    
        if (auto it = mapSockets.find(destinatario); it != mapSockets.end()) {
            ostringstream cuerpo;
            cuerpo << 'f';
            cuerpo << setw(5) << setfill('0') << nickname.length() << nickname;
            cuerpo << setw(5) << setfill('0') << fileNameLen << nombreArchivo;
            cuerpo << setw(15) << setfill('0') << tamArchivo;
            cuerpo.write(contenido.data(), contenido.size());
            cuerpo << setw(5) << setfill('0') << hashLen << hash;
    
            string cuerpoStr = cuerpo.str();
    
            ostringstream finalStream;
            finalStream << setw(5) << setfill('0') << 1;
            finalStream << cuerpoStr;
    
            string envio = finalStream.str();
            write(it->second, envio.c_str(), envio.size());
    
            cout << "Archivo reenviado a " << destinatario << ".\n";
        }
    } else if(tipo == "J") {
        if(jugadores.size() == 0) {
            jugadores[nickname] = 'X';
            ostringstream oss;
            oss << "00035m00018waiting for player00006server";
            string envio = oss.str();
            write(cliSocket, envio.c_str(), envio.size());
        } else if(jugadores.size() == 1) {
            jugadores[nickname] = 'O';
            for(auto it:jugadores) {
                string envio = "00024m00006inicio00006server";
                int n = cSock(it.second);
                write(n,envio.c_str(),envio.size());
                envio = "00010X" + convertir();
                write(n,envio.c_str(), envio.size());
            }
            int n = cSock('X');
            string envio = "00002TX";
            cout << "Enviando = " << envio << endl;
            write(n, envio.c_str(),envio.size());
        }else {
            jugadores[nickname] = '-';
            cout << "else opcion\n";
            string ee = "00053m00036El juego esta lleno. Modo espectador00006server";
            int n = mapSockets[nickname];
            write(n, ee.c_str(),ee.size());
        }

    }else if(tipo == "P") {
        
        char pos;
        char ficha;
        read(cliSocket, &pos, 1);
        read(cliSocket, &ficha, 1);
        int posi = pos - '0';
        
        if (marcar(posi,ficha)) {
            
            pair<int,string> p = comprobarEstado();
            if(p.first == 1) {
                cout << "Opcion1\n";
                for(auto it:jugadores) {
                    string envio = "00010X" + convertir();
                    string name = it.first;
                    int n = mapSockets[name];
                    write(n,envio.c_str(), envio.size());
                }

                string envio1 = "00002OW";
                string envio2 = "00002OL";
                cout << "Ganador = " << p.second[0] << endl;
                int n =cSock(p.second[0]);
                cout << "Enviando: " << envio1 << endl;
                write(n, envio1.c_str(), envio1.size());
                n = cSock(cambio(p.second[0]));
                cout << "Enviando: " << envio2 << endl;
                write(n, envio2.c_str(),envio2.size());
            } else if(p.first == 2){
                cout << "Opcion2\n";
                for(auto it:jugadores) {
                    string envio = "00010X" + convertir();
                    string name = it.first;
                    int n = mapSockets[name];
                    write(n,envio.c_str(), envio.size());
                    string envio1 = "00002OE";
                    cout << "Enviando: " << envio1 << endl;
                    write(n, envio1.c_str(), envio1.size());
                }

            }else{
                cout << "Opcion3\n";
                for(auto it:jugadores) {
                    string envio = "00010X" + convertir();
                    string name = it.first;
                    int n = mapSockets[name];
                    write(n,envio.c_str(), envio.size());
                }
                
                cout << "cambioficha = " << cambio(ficha) << endl;
                int n = cSock(cambio(ficha));
                cout << "Enviando T a =" << n <<endl;
                string lin = "00002T";
                lin += cambio(ficha);

                cout << lin << endl;
                write(n, lin.c_str(), lin.size());

            }
        } else {
            string envio = "00022E00016Posicion ocupada";
            write(cliSocket, envio.c_str(), envio.size());
        }


    }
    }
    shutdown(cliSocket, SHUT_RDWR);
    close(cliSocket);
    mapSockets.erase (nickname);
}

int main() {
    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (SocketFD == -1) {
        perror("No se pudo crear el socket");
        return EXIT_FAILURE;
    }

    sockaddr_in stSockAddr{};
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(date); 
    stSockAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(SocketFD, reinterpret_cast<const sockaddr*>(&stSockAddr), sizeof(stSockAddr)) == -1) {
        perror("Error en bind");
        close(SocketFD);
        return EXIT_FAILURE;
    }

    if (listen(SocketFD, 10) == -1) {
        perror("Error en listen");
        close(SocketFD);
        return EXIT_FAILURE;
    }

    cout << "Servidor escuchando en el puerto " << date << "...\n";

    while (true) {
        int ClientFD = accept(SocketFD, nullptr, nullptr);
        if (ClientFD < 0) {
            perror("Error en accept");
            continue;
        }

        try {
            char sizeBuffer[6] = {0};
            int n = read(ClientFD, sizeBuffer, 5);
            if (n <= 0) throw runtime_error("No se pudo leer tamaño");
            int tamano = stoi(string(sizeBuffer, 5));

            char tipo;
            n = read(ClientFD, &tipo, 1);
            if (n <= 0) throw runtime_error("No se pudo leer tipo");

            if (tipo == 'N') {
                vector<char> nickBuffer(tamano - 1);
                int leidos = 0;
                while (leidos < tamano - 1) {
                    int bytes = read(ClientFD, &nickBuffer[leidos], tamano - 1 - leidos);
                    if (bytes <= 0) throw runtime_error("Error leyendo nickname");
                    leidos += bytes;
                }
                string nickname(nickBuffer.begin(), nickBuffer.end());

                mapSockets[nickname] = ClientFD;
                cout << nickname << " se ha conectado.\n";
                thread(readSocketThread, ClientFD, nickname).detach();
            } else {
                shutdown(ClientFD, SHUT_RDWR);
                close(ClientFD);
            }

        } catch (const exception& e) {
            cerr << "Error manejando cliente: " << e.what() << endl;
            shutdown(ClientFD, SHUT_RDWR);
            close(ClientFD);
        }
    }

    close(SocketFD);
    return 0;
}

