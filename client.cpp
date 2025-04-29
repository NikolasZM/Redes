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
#include <thread>   
#include <vector>
#include <cstring>
#include <iomanip>
#include <mutex>
using namespace std;

int date = 45008;
int SocketFD;
mutex mtx;
bool juego = false;
void readSocketThread(int cliSocket) {
    while (true) {

        char sizeBuffer[6] = {0};
        int n = read(cliSocket, sizeBuffer, 5);
        if (n <= 0) break;
 
        int tam = atoi(sizeBuffer);  

        char tipo;
        n = read(cliSocket, &tipo, 1);
        cout << "leyendo:" << tipo << endl;

        if (tipo == 'l') {
            vector<char> msg(tam - 1);
            n = read(cliSocket, msg.data(), tam - 1);
          
            cout << "Clientes conectados:\n-";
            for (char c : msg) {
                if (c == ',') {
                    std::cout << "\n-";
                } else {
                    std::cout << c;
                }
            }
            cout << endl;
        }
        else if(tipo == 'm' || tipo == 'b'){

            char msgLenBuffer[6] = {0};
            n = read(cliSocket, msgLenBuffer, 5);
            int msgLen = atoi(msgLenBuffer);

            std::string mensaje(msgLen, '\0');
            n = read(cliSocket, &mensaje[0], msgLen);

            char nameLenBuffer[6] = {0};
            n = read(cliSocket, nameLenBuffer, 5);
            int nameLen = atoi(nameLenBuffer);

            std::string nombre(nameLen, '\0');
            n = read(cliSocket, &nombre[0], nameLen);

            if (tipo == 'b') {
                cout << "Mensaje Global de " << nombre << ": " << mensaje << endl;
            } else {
                cout << "Mensaje de " << nombre << ": " << mensaje << endl;
            }

        } else if (tipo == 'f') {
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
        } else if(tipo == 'X') {
            string tablero(9, '\0');
            read(cliSocket, &tablero[0], 9);
            cout << "Tablero:\n";
            
            for (int i = 0; i < 3; ++i) {
                cout << tablero[i*3] << " | " << tablero[i*3+1] << " | " << tablero[i*3+2] << endl;
                if (i < 2) {
                    cout << "---------" << endl;
                }
            }
            
        } else if (tipo == 'T') {
            char ficha;
            n = read(cliSocket, &ficha, 1); 
            if (n <= 0) {
                cerr << "Error leyendo ficha.\n";
                return;
            }
            cout << "Es tu turno de jugar, escoge una casilla vacía (1-9):\n>>> ";
            int pos;

            while(1) {
            string input;
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); 

            //getline(cin, input);
            cin >> pos;
            cout << "Enviando..." << pos << ".\n";
            //cout << "Input: "<< input <<endl;
            //pos = (int);
            cout << "Enviando..." << pos << ".\n";

            if (pos < 1 || pos > 9) {
                cout << "Posición no válida. Intenta de nuevo.\n";
                continue; 
            }else {
                break;
            }
            cout <<"Enviando 2.\n";
            }
            string envio = "00003P" + to_string(pos) + ficha;
            cout << envio << endl;
            n = write(cliSocket, envio.c_str(), envio.size());
            if (n <= 0) {
                cerr << "Error enviando la posición.\n";
                return;
            }
        } else if(tipo == 'O') {
            char resultado;
            read(cliSocket, &resultado, 1);
            if ( resultado == 'W') {
                cout << "felicidades, ganaste el juego!!!\n";
            }else if (resultado == 'L') {
                cout <<" Lo siento, perdiste el juego.\n";
            }
            juego = false;
        }
    }
    shutdown(cliSocket, SHUT_RDWR);
    close(cliSocket);
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
    getline(cin,nickname);

    ostringstream ss;
    ss << setw(5) << setfill('0') << nickname.length() + 1 << "N" << nickname;
    string initMsg = ss.str();
    write(SocketFD, initMsg.c_str(), initMsg.length());

    thread(readSocketThread, SocketFD).detach();

    while(1) {
        int opt;
        if(!juego) {
        cout << "Conectado al servidor. Opciones: \n[1] Ver lista de contactos.";
        cout << "\n[2]Enviar un mensaje.\n";
        cout << "[3]Enviar mensaje global.\n[4]Desconectarte\n[5]Enviar un archivo.\n[6]Jugar 3 en raya\n>>>";
        cin >> opt;
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); 

        if (opt == 1) {
            string cmd = "00001L";
            write(SocketFD, cmd.c_str(), cmd.length());

        } else if(opt == 2) {
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

        } else if(opt == 3){
            string mensaje;
            cout <<"Ahora escribe el mensaje global:\n>>>";
            getline(cin, mensaje);

            ostringstream msg;
            msg << setw(5) << setfill('0') << mensaje.length() + 11
                << "B"
                << setw(5) << setfill('0') << mensaje.length()
                << mensaje;

            write(SocketFD, msg.str().c_str(), msg.str().length());

        } else if(opt == 4){
            string salida = "00001Q";
            write(SocketFD, salida.c_str(), salida.length());
            break;

        } else if (opt == 5) {
            string destinatario, rutaArchivo;
            cout << "Escribe el nombre del destinatario:\n>>>";
            getline(cin, destinatario);
            cout <<"Ruta del archivo a enviar (.txt o .jpg):\n>>>";
            getline(cin, rutaArchivo);
        
            size_t slashPos = rutaArchivo.find_last_of("/\\");
            string nombreArchivo = (slashPos == string::npos) ? rutaArchivo : rutaArchivo.substr(slashPos + 1);
        
            ifstream archivo(rutaArchivo, ios::binary);
            if (!archivo) {
                cerr << "No se pudo abrir el archivo.\n";
                return 0;
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
        } else if (opt == 6) {
            string cmd = "00001J";
            write(SocketFD, cmd.c_str(), cmd.length());
            cout << "Solicitud enviada para jugar 3 en raya.\n";
            juego = true;
        }

        else {
            cout << "Opción no válida.\n";
        }
    }
    }
    shutdown(SocketFD, SHUT_RDWR);
    close(SocketFD);
    return 0;
}
