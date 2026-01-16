#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <string>
#include <thread>
#include <dirent.h>
#include <sys/stat.h>

using namespace std;

void ensureDirectoryExists(const string& path) {
    struct stat info;

    if (stat(path.c_str(), &info) != 0) {
        cout << "ВНИМАНИЕ: Директория '" << path << "' не найдена" << endl;
        cout << "Создаю директорию..." << endl;

        if (mkdir(path.c_str(), 0755) == 0) {
            cout << "Директория '" << path << "' успешно создана, файлы могут быть недоступны" << endl;
        } else {
            cerr << "ОШИБКА: Не удалось создать директорию '" << path << "': "
                 << strerror(errno) << endl;
        }
    }
}

string getFileList(const string& directory) {
    string fileList = "Доступные файлы:\n";

    DIR* dir = opendir(directory.c_str());
    if (dir == nullptr) {
        return "Нет доступных файлов";
    }

    struct dirent* entry;
    int count = 0;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] != '.') {
            fileList += "  - " + string(entry->d_name) + "\n";
            count++;
        }
    }
    closedir(dir);

    if (count == 0) {
        return "Нет доступных файлов";
    }

    return fileList;
}

bool sendFile(int clientSocket, const string& filename) {
    if (filename.find("..") != string::npos || filename.find("/") != string::npos || filename.find("\\") != string::npos){
        streamsize errorCode = -1;
        send(clientSocket, &errorCode, sizeof(errorCode), 0);
        string error = "ОШИБКА: Недопустимое имя файла\n";
        error += getFileList("Files_catalog");
        send(clientSocket, error.c_str(), error.length(), 0);

        cerr << "ОШИБКА: Некорректное имя файла: " << filename << endl;
        return false;
    }

    string fullPath = "Files_catalog/" + filename;
    ifstream file(fullPath, ios::binary | ios::ate);
    if (!file.is_open()) {
        streamsize errorCode = -1;
        send(clientSocket, &errorCode, sizeof(errorCode), 0);
        string error = "ОШИБКА: Файл не найден\n";
        error += getFileList("Files_catalog");
        send(clientSocket, error.c_str(), error.length(), 0);
        cerr << "ОШИБКА: Файл не найден: " << fullPath << endl;
        return false;
    }

    streamsize fileSize = file.tellg();
    file.seekg(0, ios::beg);

    cout << "Отправка файла: " << filename << " (" << fileSize << " байт)" << endl;

    send(clientSocket, &fileSize, sizeof(fileSize), 0);

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        send(clientSocket, buffer, file.gcount(), 0);
    }

    file.close();
    cout << "Файл успешно отправлен: " << filename << endl;
    return true;
}

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void handleClient(int clientSocket) {
    char buffer[256];
    int n;

    // Читаем имя файла от клиента
    bzero(buffer, 256);
    n = read(clientSocket, buffer, 255);
    if (n < 0) {
        perror("ОШИБКА при чтении из сокета");
        close(clientSocket);
        return;
    }

    string filename(buffer, n);
    cout << "Поток " << this_thread::get_id() << " обрабатывает файл: "
         << filename << endl;

    sendFile(clientSocket, filename);

    close(clientSocket);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        cerr << "ОШИБКА, порт не указан" << endl;
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ОШИБКА при открытии сокета");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    if (portno < 1 || portno > 65535) {
        cerr << "ОШИБКА: Порт должен быть от 1 до 65535" << endl;
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ОШИБКА при привязке");

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    cout << "Сервер запущен на порту " << portno << endl;
    ensureDirectoryExists("Files_catalog");
    cout << "Ожидание подключений..." << endl;

    while (true) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ОШИБКА при принятии");
            continue;
        }

        cout << "Новое подключение принято" << endl;

        thread clientThread(handleClient, newsockfd);

        clientThread.detach();
    }

    close(sockfd);
    return 0;
}
