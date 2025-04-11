//
//  improved_chat_client.cpp
//  chat_client
//
//  Created on 4/11/25.
//

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <atomic>
#include <sstream>
#include <queue>
#include <mutex>
#include <condition_variable>

// 서버 응답을 저장할 안전한 큐
class SafeMessageQueue {
private:
    std::queue<std::string> queue;
    std::mutex mutex;
    std::condition_variable cv;

public:
    void push(const std::string& message) {
        std::unique_lock<std::mutex> lock(mutex);
        queue.push(message);
        cv.notify_one();
    }

    bool pop(std::string& message, int timeout_ms) {
        std::unique_lock<std::mutex> lock(mutex);
        if (queue.empty()) {
            // 타임아웃까지 대기
            auto status = cv.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                                     [this] { return !queue.empty(); });
            if (!status) return false; // 타임아웃
        }
        
        if (!queue.empty()) {
            message = queue.front();
            queue.pop();
            return true;
        }
        return false;
    }
};

// 메시지 수신을 위한 스레드 함수
void receiveMessages(int socket, std::atomic<bool>& running, SafeMessageQueue& messageQueue) {
    char buffer[1024];
    while (running) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(socket, buffer, sizeof(buffer), 0);
        
        if (bytesReceived <= 0) {
            std::cout << "서버와의 연결이 끊어졌습니다." << std::endl;
            running = false;
            break;
        }
        
        std::string message(buffer);
        messageQueue.push(message);
        std::cout << "서버: " << buffer << std::endl;
    }
}

void showClientMenu() {
    std::cout << "\n==== 채팅 클라이언트 메뉴 ====\n"
              << "1. 로그인    2. 회원가입    3. 종료\n"
              << "선택: ";
}

// 로그인 요청 보내기
bool sendLoginRequest(int socket, const std::string& username, const std::string& password) {
    std::string loginCmd = "/login " + username + " " + password;
    if (send(socket, loginCmd.c_str(), loginCmd.length(), 0) < 0) {
        std::cerr << "로그인 요청 전송 실패" << std::endl;
        return false;
    }
    return true;
}

// 회원가입 요청 보내기
bool sendRegisterRequest(int socket, const std::string& username, const std::string& password) {
    std::string registerCmd = "/register " + username + " " + password;
    if (send(socket, registerCmd.c_str(), registerCmd.length(), 0) < 0) {
        std::cerr << "회원가입 요청 전송 실패" << std::endl;
        return false;
    }
    return true;
}

// 채팅 모드 처리 함수
void handleChatMode(int socket, std::atomic<bool>& running, bool& authenticated) {
    std::string message;
    std::cout << "채팅 모드를 시작합니다. '/exit'를 입력하면 종료됩니다." << std::endl;
    
    while (authenticated && running) {
        std::getline(std::cin, message);
        
        if (message == "/exit") {
            // 종료 메시지 전송
            send(socket, message.c_str(), message.length(), 0);
            authenticated = false;
            break;
        }
        
        // 메시지 전송
        if (send(socket, message.c_str(), message.length(), 0) < 0) {
            std::cerr << "메시지 전송 실패" << std::endl;
            authenticated = false;
            break;
        }
    }
}

int main() {
    // 소켓 생성
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "소켓 생성 실패" << std::endl;
        return -1;
    }
    
    // 서버 주소 설정
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);  // 서버가 사용하는 포트
    
    // 서버 IP 주소 설정 (기본적으로 로컬호스트 사용)
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
        std::cerr << "잘못된 주소" << std::endl;
        close(clientSocket);
        return -1;
    }
    
    // 서버에 연결
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "연결 실패" << std::endl;
        close(clientSocket);
        return -1;
    }
    
    std::cout << "서버에 연결되었습니다!" << std::endl;
    
    // 메시지 큐 생성
    SafeMessageQueue messageQueue;
    
    // 메시지 수신을 위한 스레드 생성
    std::atomic<bool> running(true);
    std::thread receiveThread(receiveMessages, clientSocket, std::ref(running), std::ref(messageQueue));
    
    // 인증 상태를 추적
    bool authenticated = false;
    
    // 메뉴 루프
    int choice;
    std::string username, password, serverResponse;
    // switch 문 밖에서 변수 선언
    std::string exitMsg;
    
    while (running) {
        showClientMenu();
        std::cin >> choice;
        std::cin.ignore(); // 버퍼 비우기
        
        switch (choice) {
            case 1: {
                // 로그인
                std::cout << "아이디: ";
                std::getline(std::cin, username);
                std::cout << "비밀번호: ";
                std::getline(std::cin, password);
                
                if (sendLoginRequest(clientSocket, username, password)) {
                    std::cout << "로그인 요청을 보냈습니다. 서버 응답 대기 중..." << std::endl;
                    
                    // 서버 응답 대기
                    if (messageQueue.pop(serverResponse, 5000)) { // 5초 타임아웃
                        // 로그인 성공 여부 확인
                        if (serverResponse.find("로그인 성공") != std::string::npos) {
                            authenticated = true;
                            // 별도 함수로 채팅 모드 처리
                            handleChatMode(clientSocket, running, authenticated);
                        } else {
                            std::cout << "로그인 실패. 메인 메뉴로 돌아갑니다." << std::endl;
                        }
                    } else {
                        std::cout << "서버 응답 타임아웃. 메인 메뉴로 돌아갑니다." << std::endl;
                    }
                }
                break;
            }
                
            case 2: {
                // 회원가입
                std::cout << "아이디: ";
                std::getline(std::cin, username);
                std::cout << "비밀번호: ";
                std::getline(std::cin, password);
                
                if (sendRegisterRequest(clientSocket, username, password)) {
                    std::cout << "회원가입 요청을 보냈습니다. 서버 응답 대기 중..." << std::endl;
                    
                    // 서버 응답 대기
                    if (messageQueue.pop(serverResponse, 5000)) {
                        std::cout << "서버 응답: " << serverResponse << std::endl;
                        
                        // 회원가입 성공 여부 확인
                        if (serverResponse.find("회원가입이 완료되었습니다") != std::string::npos) {
                            std::cout << "\n회원가입이 성공적으로 완료되었습니다!" << std::endl;
                            std::cout << "이제 로그인하여 채팅을 시작할 수 있습니다." << std::endl;
                        } else if (serverResponse.find("이미 존재하는 사용자명입니다") != std::string::npos) {
                            std::cout << "\n이미 사용 중인 아이디입니다. 다른 아이디를 사용해주세요." << std::endl;
                        }
                        
                        // 3초 대기 후 메인 메뉴로 돌아감
                        std::cout << "\n메인 메뉴로 돌아갑니다..." << std::endl;
                        sleep(3);
                    } else {
                        std::cout << "서버 응답 타임아웃. 메인 메뉴로 돌아갑니다." << std::endl;
                    }
                }
                break;
            }
                
            case 3: {
                // 종료
                running = false;
                exitMsg = "/exit";
                send(clientSocket, exitMsg.c_str(), exitMsg.length(), 0);
                break;
            }
                
            default: {
                std::cout << "잘못된 입력입니다." << std::endl;
                break;
            }
        }
    }
    
    // 스레드 종료 기다림
    if (receiveThread.joinable()) {
        receiveThread.join();
    }
    
    // 소켓 닫기
    close(clientSocket);
    std::cout << "채팅 클라이언트가 종료되었습니다." << std::endl;
    
    return 0;
}
