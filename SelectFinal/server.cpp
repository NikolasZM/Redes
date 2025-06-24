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
#include <map>
#include <string>
#include <vector>
#include <iomanip>
#include <sys/select.h>
#include <algorithm>

using namespace std;

int date = 45000;
map<string,int> mapSockets;
map<string,char> jugadores;

vector<vector<char>> tablero(3, vector<char>(3, ' '));

string recortar(string& str, int n) {
    if (n > str.size()) n = str.size(); 
    string extraido = str.substr(0, n); 
    str.erase(0, n);
    return extraido;
}

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


auto leerNBytes(int cantidad, int cliSocket) {
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

void handleClient(int cliSocket, const string& nickname, string buffer) {
    
    int n;



    string tipo = recortar(buffer,1);
    if (tipo.empty()) {
        shutdown(cliSocket, SHUT_RDWR);
        close(cliSocket);
        mapSockets.erase(nickname);
        return;
    }

    if (tipo == "L") {
        string msg;
        for (auto it = mapSockets.cbegin(); it != mapSockets.cend(); ++it) {
            if (it != mapSockets.cbegin()) msg += ",";
            msg += it->first;
        }

        ostringstream oss;
        oss << setw(5) << setfill('0') << (msg.length() + 1) << 'l' << msg;
        string mensaje = oss.str();

        write(mapSockets[nickname], mensaje.c_str(), mensaje.size());
        cout <<"ENVIANDO: "<< mensaje << endl;
    }
    else if (tipo == "M") {
        string msgLenStr = recortar(buffer,5);
        int msgLen = stoi(msgLenStr);
        std::string msg = recortar(buffer,msgLen);

        string dstLenStr = recortar(buffer,5);
        int dstLen = stoi(dstLenStr);
        string destinatario = recortar(buffer,dstLen);

        cout << "Mensaje de " << nickname << " para " << destinatario << ": " << msg << "\n";
        
        if (auto it = mapSockets.find(destinatario); it != mapSockets.end()) {
            ostringstream oss;
            oss << setw(5) << setfill('0') << (msgLen + nickname.length() + 11)
                << 'm'
                << setw(5) << msgLen << msg
                << setw(5) << nickname.length() << nickname;
            string envio = oss.str();
            write(it->second, envio.c_str(), envio.size());
            cout <<"ENVIANDO: "<< envio << endl;

        } else {
            cerr << "No se encontró el destinatario " << destinatario << "\n";
        }
    }
    else if (tipo == "B") {
        string msgLenStr = recortar(buffer,5);
        int msgLen = stoi(msgLenStr);
        string msg = recortar(buffer,msgLen);

        ostringstream oss;
        oss << setw(5) << setfill('0') << (msgLen + nickname.length() + 11)
            << 'b'
            << setw(5) << msgLen << msg
            << setw(5) << nickname.length() << nickname;
        string envio = oss.str();
        cout <<"ENVIANDO: "<< envio << endl;

        for (const auto& [_, sock] : mapSockets) {
            write(sock, envio.c_str(), envio.size());
            
        }
    } else if(tipo == "Q") {
        mapSockets.erase(nickname);
        cout << nickname << " se desconectó.\n";
        shutdown(cliSocket, SHUT_RDWR);
        close(cliSocket);
    } else if (tipo == "F") {
        string dstLenStr = leerNBytes(5,cliSocket);
        int dstLen = stoi(dstLenStr);
        string destinatario = leerNBytes(dstLen,cliSocket);
    
        string fileNameLenStr = leerNBytes(5,cliSocket);
        int fileNameLen = stoi(fileNameLenStr);
        string nombreArchivo = leerNBytes(fileNameLen,cliSocket);
    
        string tamArchivoStr = leerNBytes(15,cliSocket);
        int tamArchivo = stoi(tamArchivoStr);
    
        string contenido = leerNBytes(tamArchivo,cliSocket);
    
        string hashLenStr = leerNBytes(5,cliSocket);
        int hashLen = stoi(hashLenStr);
        string hash = leerNBytes(hashLen,cliSocket);
    
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
            cout <<"ENVIANDO: "<< envio << endl;

        } else if(jugadores.size() == 1) {
            jugadores[nickname] = 'O';
            for(auto it:jugadores) {
                string envio = "00024m00006inicio00006server";
                int n = cSock(it.second);
                write(n,envio.c_str(),envio.size());
                cout <<"ENVIANDO: "<< envio << endl;
                envio = "00010X" + convertir();
                write(n,envio.c_str(), envio.size());
                cout <<"ENVIANDO: "<< envio << endl;

            }
            int n = cSock('X');
            string envio = "00002TX";
            cout << "Enviando = " << envio << endl;
            write(n, envio.c_str(),envio.size());
            cout <<"ENVIANDO: "<< envio << endl;

        }else {
            jugadores[nickname] = '-';
            cout << "else opcion\n";
            string ee = "00053m00036El juego esta lleno. Modo espectador00006server";
            int n = mapSockets[nickname];
            write(n, ee.c_str(),ee.size());
            cout <<"ENVIANDO: "<< ee << endl;

        }
    } else if(tipo == "P") {
        char pos;
        char ficha;

        string aux = recortar(buffer,1);
        pos = aux[0];
        aux = recortar(buffer,1);
        ficha = aux[0];
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
            cout <<"ENVIANDO: "<< envio << endl;

        }
    }
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

    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_SET(SocketFD, &master_set);
    int max_fd = SocketFD;

    while (true) {
        read_set = master_set;
        
        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0) {
            perror("Error en select");
            continue;
        }

        if (FD_ISSET(SocketFD, &read_set)) {
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
                cout << "Tamaño: " << tamano << endl; 

                string linea(tamano, '\0');
                char tipo;
                n = read(ClientFD, &linea[0], tamano);
                tipo = linea[0];
                cout << "leyendo: "<<linea << endl;
                if (n <= 0) throw runtime_error("No se pudo leer tipo");

                if (tipo == 'N') {
                    
                    string nickname= recortar(linea,1);
                    nickname = linea;

                    mapSockets[nickname] = ClientFD;
                    cout << nickname << " se ha conectado.\n";
                    
                    FD_SET(ClientFD, &master_set);
                    if (ClientFD > max_fd) {
                        max_fd = ClientFD;
                    }
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

        for (auto it = mapSockets.begin(); it != mapSockets.end(); ) {
            int fd = it->second;
            if (FD_ISSET(fd, &read_set)) {
                string nickname = it->first;

                char sizeBuffer[6] = {0};
                int n = read(fd, sizeBuffer, 5);
                if (n <= 0) throw runtime_error("No se pudo leer tamaño");
                int tamano = stoi(string(sizeBuffer, 5));
                cout << "Tamaño: " << tamano << endl; 

                string linea(tamano, '\0');
                char tipo;
                n = read(fd, &linea[0], tamano);
                tipo = linea[0];
                cout << "leyendo: "<<linea << endl;
                handleClient(fd, nickname,linea);
                
                if (mapSockets.find(nickname) == mapSockets.end()) {
                    FD_CLR(fd, &master_set);
                    it = mapSockets.begin(); 
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }

    close(SocketFD);
    return 0;
}

