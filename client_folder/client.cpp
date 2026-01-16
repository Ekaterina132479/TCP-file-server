#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>
#include <sys/stat.h>

using namespace std;

void ensureDirectoryExists(const string& path) {
    struct stat info;

    if (stat(path.c_str(), &info) != 0) {
        cout << "ВНИМАНИЕ: Директория '" << path << "' не найдена" << endl;
        cout << "Создаю директорию..." << endl;

        if (mkdir(path.c_str(), 0755) == 0) {
            cout << "Директория '" << path << "' успешно создана" << endl;
        } else {
            cerr << "ОШИБКА: Не удалось создать директорию '" << path << "': "
                 << strerror(errno) << endl;
        }
    }
}

void receiveFile(int sockfd, const string& requestedFilename) {
    streamsize fileSize;
    int n = recv(sockfd, &fileSize, sizeof(fileSize), 0);

    if (n <= 0) {
        cerr << "ОШИБКА: Соединение разорвано" << endl;
        return;
    }

    if (fileSize < 0) {
        char error[1024];
        int received = recv(sockfd, error, 1023, 0);
        if (received < 0){
            cerr << "ОШИБКА получения сообщения об ошибке" << endl;
            return;
        }
        error[received] = '\0';
        cerr << error << endl;
        return;
    }

    string saveFilename = "Downloads/" + requestedFilename;

    ofstream outFile(saveFilename, ios::binary);
    if (!outFile.is_open()) {
        cerr << "ОШИБКА: не удалось создать файл" << endl;
        return;
    }

    char buffer[4096];
    streamsize received = 0;
    while (received < fileSize) {
        int n = recv(sockfd, buffer, min((streamsize)sizeof(buffer), fileSize - received), 0);
        if (n <= 0) break;
        outFile.write(buffer, n);
        received += n;
    }

    outFile.close();

    if (received == fileSize) {
        cout << "Файл сохранен: " << saveFilename << endl;
    } else {
        cerr << "ПРЕДУПРЕЖДЕНИЕ: получено " << received
             << " байт из " << fileSize << " (файл может быть поврежден)" << endl;
    }
}

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 4) {
        cerr << "Использование: " << argv[0] << " <hostname> <port> <filename>" << endl;
        cerr << "Пример: " << argv[0] << " localhost 12345 test.txt" << endl;
        exit(1);
    }

    portno = atoi(argv[2]);
    if (portno < 1 || portno > 65535) {
        cerr << "ОШИБКА: Порт должен быть от 1 до 65535" << endl;
        exit(1);
    }
    string filename = argv[3];
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ОШИБКА при открытии сокета");

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        cerr << "ОШИБКА, хост не найден" << endl;
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ОШИБКА при подключении");

    ensureDirectoryExists("Downloads");

    cout << "Подключение к " << argv[1] << ":" << portno << endl;
    cout << "Запрашиваем файл: " << filename << endl;

    n = write(sockfd, filename.c_str(), filename.length());
    if (n < 0)
        error("ОШИБКА при записи в сокет");

    receiveFile(sockfd, filename);

    close(sockfd);
    return 0;
}
