#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

using namespace std;
#pragma comment(lib, "Ws2_32.lib") // 链接Winsock库，确保编译器在链接阶段包含Ws2_32.lib
#define PORT 8080        // 服务器监听的端口号
#define BUFFER_SIZE 1024 // 缓冲区大小，用于接收和发送数据


void receiveMessages(SOCKET client_socket)
{
    char buffer[BUFFER_SIZE]; // 用于存储从服务器接收的数据
    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);                                   // 将缓冲区清零，以准备接收新消息
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0); // 接收来自服务器的数据
        if (bytes_received > 0)
        { // 如果接收到数据，则输出服务器消息
            cout << buffer;
        }
    }
}

int main()
{

    WSADATA wsaData; // 用于存储Winsock库的版本信息和其他初始化数据

    // 初始化Socket DLL
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    { // 使用Winsock版本2.2
        cout << "Failed to initialize Winsock." << endl;
        return -1;
    }

    // 创建客户端套接字
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0); // 使用TCP协议创建套接字
    if (client_socket == INVALID_SOCKET)
    {
        cout << "Failed to create socket." << endl;
        WSACleanup(); // 结束使用Socket，释放Socket DLL资源
        return -1;
    }

    sockaddr_in server_addr;                                // 定义一个结构体来保存服务器地址信息
    server_addr.sin_family = AF_INET;                       // 使用IPv4地址
    server_addr.sin_port = htons(PORT);                     // 将端口号转换为网络字节序
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr); // 将IP地址转换为二进制格式并存储在结构中

    // 连接到服务器
    if (connect(client_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        cout << "Connection to server failed." << endl;
        closesocket(client_socket); // 关闭套接字
        WSACleanup();               // 清理Winsock资源
        return -1;
    }

    cout << "(Connected to server. You can start chatting! Type \"exit\" to end the conversation.)" << endl;

    thread(receiveMessages, client_socket).detach(); // 启动一个新线程接收服务器的消息，并与主线程分离

    char message[BUFFER_SIZE]; // 用于存储用户输入的消息
    while (true)
    {
        cin.getline(message, BUFFER_SIZE); // 从标准输入读取用户输入的消息
        if (strcmp(message, "exit") == 0)
        {
            cout << "Exiting chat..." << endl;
            break;
        }
        send(client_socket, message, strlen(message), 0); // 将用户输入的消息发送给服务器
    }

    closesocket(client_socket); // 关闭客户端套接字
    WSACleanup();               // 清理Winsock资源
    return 0;
}
