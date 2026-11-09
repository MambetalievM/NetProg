#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <getopt.h>

class UdpTimeClient {
private:
    int socket_fd;
    std::string server_ip;
    int server_port;

    void print_instructions() const {
        std::cout << "Клиент (UDP)\n";
        std::cout << "Использование: ./daytime_client\n";
        std::cout << "  -h, --help     Справка\n";
        std::cout << "  -a, --address  IP-адрес сервера (по умолчанию 172.16.40.1)\n";
        std::cout << "  -p, --port     Порт сервера (по умолчанию 13)\n";
        std::cout << "  -r, --run      Запуск с параметрами по умолчанию\n";
    }

public:
    UdpTimeClient() : socket_fd(-1), server_ip("172.16.40.1"), server_port(13) {}

    ~UdpTimeClient() {
        if (socket_fd >= 0) {
            close(socket_fd);
        }
    }

    void process_arguments(int argc, char** argv) {
        static struct option long_opts[] = {
            {"help", no_argument, nullptr, 'h'},
            {"address", required_argument, nullptr, 'a'},
            {"port", required_argument, nullptr, 'p'},
            {"run", no_argument, nullptr, 'r'},
            {nullptr, 0, nullptr, 0}
        };

        int current_opt;
        while ((current_opt = getopt_long(argc, argv, "ha:p:r", long_opts, nullptr)) != -1) {
            switch (current_opt) {
                case 'h':
                    print_instructions();
                    exit(0);
                case 'a':
                    server_ip = optarg;
                    break;
                case 'p':
                    try {
                        server_port = std::stoi(optarg);
                    } catch (...) {
                        throw std::invalid_argument("Ошибка: порт должен быть числом.");
                    }
                    break;
                case 'r':
                    server_ip = "172.16.40.1";
                    server_port = 13;
                    break;
                default:
                    print_instructions();
                    exit(1);
            }
        }
    }

    void run() {
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd < 0) {
            throw std::runtime_error("Не удалось создать UDP сокет.");
        }

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(server_port);

        if (inet_pton(AF_INET, server_ip.c_str(), &addr.sin_addr) <= 0) {
            throw std::invalid_argument("Некорректный IP-адрес: " + server_ip);
        }

        std::cout << "Запрос времени у " << server_ip << ":" << server_port << "...\n";

        char ping_byte = '\0';
        if (sendto(socket_fd, &ping_byte, 1, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            throw std::runtime_error("Ошибка при отправке UDP пакета.");
        }

        std::vector<char> rx_buffer(512, 0);
        sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);

        ssize_t bytes_received = recvfrom(socket_fd, rx_buffer.data(), rx_buffer.size() - 1, 0, 
                                          reinterpret_cast<sockaddr*>(&from_addr), &from_len);

        if (bytes_received < 0) {
            throw std::runtime_error("ошибка при получении ответа.");
        }

        std::cout << "Ответ сервера:\n" << rx_buffer.data() << "\n";
    }
};

int main(int argc, char** argv) {
    try {
        UdpTimeClient client;
        client.process_arguments(argc, argv);
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Исключение: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
