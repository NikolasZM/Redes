/* Client code in C */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <limits>

using namespace std;

int date = 45000;
int SocketFD;
bool turno = false;
char fichaJug;
string tablero(9, '\0');

string recortar(string& str, int n) {
    if (n > str.size()) n = str.size(); 
    string extraido = str.substr(0, n); 
    str.erase(0, n);
    return extraido;
}


void handleServerResponse(int cliSocket, string linea, int tam) {

    char tipo;
    string aux;
    aux = recortar(linea,1);
    tipo = aux[0];

    if (tipo == 'l') {
        string msg;
        msg = recortar(linea,tam-1);
        
        cout << "Clientes conectados:\n-";
        for (char c : msg) {
            if (c == ',') {
                cout << "\n-";
            } else {
                cout << c;
            }
        }
        cout << endl;
    }
    else if(tipo == 'm' || tipo == 'b') {
        string msgLenBuffer;
        msgLenBuffer = recortar(linea, 5);
        int msgLen = stoi(msgLenBuffer);

        string mensaje(msgLen, '\0');
        mensaje = recortar(linea, msgLen);

        string nameLenBuffer;
        nameLenBuffer = recortar(linea,5);
        int nameLen = stoi(nameLenBuffer);

        string nombre(nameLen, '\0');
        nombre = recortar(linea,nameLen);

        if (tipo == 'b') {
            cout << "Mensaje Global de " << nombre << ": " << mensaje << endl;
        } else {
            cout << "Mensaje de " << nombre << ": " << mensaje << endl;
        }
    } 
    else if (tipo == 'f') {
        auto leerCampo = [&](int tama) -> string {
            string campo(tama, '\0');
            int leido = 0;
            while (leido < tama) {
                int nActual = read(cliSocket, &campo[leido], tama - leido);
                if (nActual <= 0) return "";
                leido += nActual;
            }
            return campo;
        };
    
        string remitenteLenStr = leerCampo(5);
        int remitenteLen = stoi(remitenteLenStr);
        string remitente = leerCampo(remitenteLen);
    
        string nombreArchivoLenStr = leerCampo(5);
        int nombreArchivoLen = stoi(nombreArchivoLenStr);
        string nombreArchivo = leerCampo(nombreArchivoLen);
    
        string tamArchivoStr = leerCampo(15);
        int tamArchivo = stoi(tamArchivoStr);
    
        ostringstream contenidoStream;
        int leidos = 0;
        const int bufferSize = 4096;
        char buffer[bufferSize];
        while (leidos < tamArchivo) {
            int aLeer = min(bufferSize, tamArchivo - leidos);
            int nActual = read(cliSocket, buffer, aLeer);
            if (nActual <= 0) {
                cerr << "Error leyendo archivo\n";
                return;
            }
            contenidoStream.write(buffer, nActual);
            leidos += nActual;
        }
    
        string hashLenStr = leerCampo(5);
        int hashLen = stoi(hashLenStr);
        string hash = leerCampo(hashLen);
    
        string nombreCopia = "Copia_" + nombreArchivo;
        ofstream outFile(nombreCopia, ios::binary);
        if (outFile) {
            string contenido = contenidoStream.str();
            outFile.write(contenido.data(), contenido.size());
            outFile.close();
            cout << "Archivo recibido de " << remitente << ": " << nombreCopia
                 << " (" << tamArchivo << " bytes)" << endl;
            cout << "Hash recibido: " << hash << endl;
        } else {
            cerr << "Error al guardar archivo " << nombreCopia << endl;
        }
    } 
    else if(tipo == 'X') {
        
        tablero = recortar(linea,9);
        
        cout << "Tablero:\n";
        for (int i = 0; i < 3; ++i) {
            cout << tablero[i*3] << " | " << tablero[i*3+1] << " | " << tablero[i*3+2] << endl;
            if (i < 2) {
                cout << "---------" << endl;
            }
        }
    } 
    else if (tipo == 'T') {
        
        string aux = recortar(linea,1);
        fichaJug = aux[0];
        cout << "Es tu turno de jugar, cuando estes listo presiona [7].\n";
        turno = true;
    } 
    else if(tipo == 'O') {
        char resultado;
        string aux = recortar(linea,1);
        resultado = aux[0];
        
        
        if (resultado == 'W') {
            cout << "¡Felicidades, ganaste el juego!\n";
        } else if (resultado == 'L') {
            cout << "Lo siento, perdiste el juego.\n";
        } else if(resultado == 'E') {
            cout << "El juego terminó en empate.\n";
        }
        turno = false;
    }
}

int kbhit() {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

int main(void) {
    struct sockaddr_in stSockAddr{};    
    SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == SocketFD) {
        perror("cannot create socket");
        exit(EXIT_FAILURE);
    }

    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(date);
    int res = inet_pton(AF_INET, "10.0.2.15", &stSockAddr.sin_addr);

    if (0 >= res) {
        perror("error: first parameter is not a valid address family");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    if (-1 == connect(SocketFD, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr))) {
        perror("connect failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    cout << "Bienvenido!!!\nPara empezar escoge un nickname:\n>>>";
    string nickname;
    getline(cin, nickname);

    ostringstream ss;
    ss << setw(5) << setfill('0') << nickname.length() + 1 << "N" << nickname;
    string initMsg = ss.str();
    write(SocketFD, initMsg.c_str(), initMsg.length());

    fd_set readfds;
    struct timeval tv;
    int retval;
/*
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    */

    while(1) {
        FD_ZERO(&readfds);
        FD_SET(SocketFD, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int max_fd = max(SocketFD, STDIN_FILENO) + 1;

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        retval = select(max_fd, &readfds, NULL, NULL, &tv);

        if (retval == -1) {
            perror("select()");
            break;
        } else if (retval) {
            if (FD_ISSET(SocketFD, &readfds)) {

                char sizeBuffer[6] = {0};
                int n = read(SocketFD, sizeBuffer, 5);
                if (n <= 0) throw runtime_error("No se pudo leer tamaño");
                int tamano = stoi(string(sizeBuffer, 5));

                string linea(tamano,'\0');
                read(SocketFD,&linea[0],tamano);
                //cout << "leyendo: " << linea << " tamaño: "<<tamano<<endl;


                handleServerResponse(SocketFD,linea,tamano);
            }

            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                int opt;
                if (kbhit()) {
                    cout << "Conectado al servidor. Opciones: \n[1] Ver lista de contactos.";
                    cout << "\n[2]Enviar un mensaje.\n";
                    cout << "[3]Enviar mensaje global.\n[4]Desconectarte\n[5]Enviar un archivo.\n[6]Jugar 3 en raya\n>>>";
                    cin >> opt;
                    cin.ignore(numeric_limits<streamsize>::max(), '\n'); 

                    if (opt == 1) {
                        string cmd = "00001L";
                        write(SocketFD, cmd.c_str(), cmd.length());
                    } 
                    else if(opt == 2) {
                        string destinatario, mensaje;
                        cout << "Opción 2: Enviar mensaje, escribe el destinatario:\n>>>";
                        getline(cin, destinatario);
                        cout << "Ahora escribe el mensaje:\n>>>";
                        getline(cin, mensaje);

                        ostringstream msg;
                        msg << setw(5) << setfill('0') << mensaje.length() + destinatario.length() + 11
                            << "M"
                            << setw(5) << setfill('0') << mensaje.length() << mensaje
                            << setw(5) << setfill('0') << destinatario.length() << destinatario;

                        write(SocketFD, msg.str().c_str(), msg.str().length());
                    } 
                    else if(opt == 3) {
                        string mensaje;
                        cout << "Escribe el mensaje global:\n>>>";
                        getline(cin, mensaje);

                        ostringstream msg;
                        msg << setw(5) << setfill('0') << mensaje.length() + 11
                            << "B"
                            << setw(5) << setfill('0') << mensaje.length()
                            << mensaje;

                        write(SocketFD, msg.str().c_str(), msg.str().length());
                    } 
                    else if(opt == 4) {
                        string salida = "00001Q";
                        write(SocketFD, salida.c_str(), salida.length());
                        break;
                    } 
                    else if (opt == 5) {
                        string destinatario, rutaArchivo;
                        cout << "Escribe el nombre del destinatario:\n>>>";
                        getline(cin, destinatario);
                        cout << "Ruta del archivo a enviar (.txt o .jpg):\n>>>";
                        getline(cin, rutaArchivo);
                    
                        size_t slashPos = rutaArchivo.find_last_of("/\\");
                        string nombreArchivo = (slashPos == string::npos) ? rutaArchivo : rutaArchivo.substr(slashPos + 1);
                    
                        ifstream archivo(rutaArchivo, ios::binary);
                        if (!archivo) {
                            cerr << "No se pudo abrir el archivo.\n";
                            continue;
                        }
                    
                        ostringstream buffer;
                        buffer << archivo.rdbuf();
                        string contenido = buffer.str();  
                        archivo.close();
                    
                        int hash = 0;
                        for (unsigned char c : contenido) hash += c;
                        string hashStr = to_string(hash);
                    
                        ostringstream cuerpo;
                        cuerpo << 'F';
                        cuerpo << setw(5) << setfill('0') << destinatario.length() << destinatario;
                        cuerpo << setw(5) << setfill('0') << nombreArchivo.length() << nombreArchivo;
                        cuerpo << setw(15) << setfill('0') << contenido.size();
                        cuerpo.write(contenido.data(), contenido.size());
                        cuerpo << setw(5) << setfill('0') << hashStr.length() << hashStr;

                        string cuerpoStr = cuerpo.str();

                        ostringstream finalStream;
                        int totalLength = 5 + cuerpoStr.length();  
                        finalStream << setw(5) << setfill('0') << 1;  
                        finalStream << cuerpoStr;

                        string paqueteFinal = finalStream.str();
                    
                        write(SocketFD, paqueteFinal.c_str(), paqueteFinal.size());
                        cout << "Archivo '" << nombreArchivo << "' enviado a " << destinatario << " correctamente.\n";
                    } 
                    else if (opt == 6) {
                        string cmd = "00001J";
                        write(SocketFD, cmd.c_str(), cmd.length());
                        cout << "Solicitud enviada para jugar 3 en raya.\n";
                    } 
                    else if(opt == 7 && turno == true) {
                        int pos;
                        while(1) {
                            string input;
                            for (int i = 0; i < 3; ++i) {
                                cout << tablero[i*3] << " | " << tablero[i*3+1] << " | " << tablero[i*3+2] << endl;
                                if (i < 2) {
                                    cout << "---------" << endl;
                                }
                            }
                            cout << "Escoge una posicion del tablero (1-9)\n>>>";
                            cin >> pos;
                            cout << "Enviando..." << pos << ".\n";

                            if (pos < 1 || pos > 9) {
                                cout << "Posición no válida. Intenta de nuevo.\n";
                                continue; 
                            } else {
                                break;
                            }
                        }
                        string envio = "00003P" + to_string(pos) + fichaJug;
                        int n = write(SocketFD, envio.c_str(), envio.size());
                        if (n <= 0) {
                            cerr << "Error enviando la posición.\n";
                            break;
                        }
                        turno = false;
                    }
                    else {
                        cout << "Opción no válida.\n";
                    }
                }
            }
        }
    }

    //tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    shutdown(SocketFD, SHUT_RDWR);
    close(SocketFD);
    return 0;
}

