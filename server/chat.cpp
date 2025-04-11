//
//  chat.cpp
//  db_socket_server
//
//  Created by Pyoung Jin Ji on 4/10/25.
//  Modified on 4/11/25
//

#include "chat.hpp"
#include "signin.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mysql/jdbc.h>
#include <sstream>
#include <string>
#include <vector>

// 문자열을 공백으로 분리하는 도우미 함수
std::vector<std::string> splitString(const std::string& s) {
    std::vector<std::string> tokens;
    std::istringstream iss(s);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

void chat(std::unique_ptr<sql::Connection>& con) {
    int serverSocket, clientSocket;
    int currentUserId = -1;  // 현재 로그인한 사용자 ID
    bool isAuthenticated = false;  // 인증 상태 추적
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);
    char buffer[1024] = {0};

    // 서버 소켓 생성
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return;
    }

    // 소켓 옵션 설정 - 주소 재사용 허용
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Setsockopt failed" << std::endl;
        close(serverSocket);
        return;
    }

    // 서버 주소 설정
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);

    // 소켓 바인딩
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        close(serverSocket);
        return;
    }

    // 연결 대기
    if (listen(serverSocket, 5) < 0) {  // 최대 5개 연결 대기 허용
        std::cerr << "Listen failed" << std::endl;
        close(serverSocket);
        return;
    }

    std::cout << "Chat server waiting for connection on port 8080..." << std::endl;

    while (true) {  // 여러 클라이언트 연결을 처리하기 위한 외부 루프
        // 클라이언트 연결 수락
        if ((clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLen)) < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;  // 다음 연결 시도로 넘어감
        }

        std::cout << "Client connected" << std::endl;
        
        // 클라이언트 연결 처리
        isAuthenticated = false;
        currentUserId = -1;

        // 채팅 루프
        while (true) {
            // 메시지 수신
            memset(buffer, 0, sizeof(buffer));
            int valread = recv(clientSocket, buffer, sizeof(buffer), 0);
            
            if (valread <= 0) {
                std::cout << "Client disconnected" << std::endl;
                break;
            }

            // 수신된 메시지 출력
            std::cout << "Received: " << buffer << std::endl;
            
            // 메시지 처리
            std::string message(buffer);
            std::vector<std::string> tokens = splitString(message);
            
            // 메시지가 명령인지 확인
            if (!tokens.empty() && tokens[0][0] == '/') {
                // 종료 명령 처리
                if (tokens[0] == "/exit") {
                    std::cout << "Client initiated exit" << std::endl;
                    break;
                }
                // chat.cpp 파일의 메시지 처리 부분에 추가할 코드
                // 아래 코드를 tokens[0] == "/exit" 조건문 다음에 추가합니다

                // 회원 삭제 명령 처리
                else if (tokens[0] == "/delete" && isAuthenticated) {
                    try {
                        // 현재 로그인한 사용자 정보 확인
                        std::unique_ptr<sql::PreparedStatement> checkStmt(
                            con->prepareStatement("SELECT username FROM users WHERE user_id = ?")
                        );
                        checkStmt->setInt(1, currentUserId);
                        std::unique_ptr<sql::ResultSet> checkRes(checkStmt->executeQuery());
                        
                        std::string username;
                        if (checkRes->next()) {
                            username = checkRes->getString("username");
                        }
                        
                        // 사용자 삭제를 위한 확인 메시지 전송
                        std::string confirmMsg = "정말 회원 탈퇴하시겠습니까? 모든 데이터가 삭제됩니다. 계속하려면 '/confirm-delete'를 입력하세요.";
                        send(clientSocket, confirmMsg.c_str(), confirmMsg.length(), 0);
                    }
                    catch (sql::SQLException& e) {
                        std::cerr << "회원 정보 확인 중 오류: " << e.what() << std::endl;
                        std::string response = "서버 오류가 발생했습니다.";
                        send(clientSocket, response.c_str(), response.length(), 0);
                    }
                }
                // 회원 삭제 확인 명령 처리
                else if (tokens[0] == "/confirm-delete" && isAuthenticated) {
                    try {
                        // 먼저 회원 정보 백업
                        std::unique_ptr<sql::PreparedStatement> userStmt(
                            con->prepareStatement("SELECT username FROM users WHERE user_id = ?")
                        );
                        userStmt->setInt(1, currentUserId);
                        std::unique_ptr<sql::ResultSet> userRes(userStmt->executeQuery());
                        
                        std::string username;
                        if (userRes->next()) {
                            username = userRes->getString("username");
                        }
                        
                        // 메시지 로그 삭제 (외래 키 제약조건 때문에 먼저 삭제)
                        std::unique_ptr<sql::PreparedStatement> deleteMessagesStmt(
                            con->prepareStatement("DELETE FROM message_log WHERE sender_id = ?")
                        );
                        deleteMessagesStmt->setInt(1, currentUserId);
                        deleteMessagesStmt->executeUpdate();
                        
                        // 세션 정보는 cascade 옵션으로 자동 삭제될 것임
                        
                        // 사용자 삭제
                        std::unique_ptr<sql::PreparedStatement> deleteUserStmt(
                            con->prepareStatement("DELETE FROM users WHERE user_id = ?")
                        );
                        deleteUserStmt->setInt(1, currentUserId);
                        int result = deleteUserStmt->executeUpdate();
                        
                        if (result > 0) {
                            std::string response = "회원 탈퇴가 완료되었습니다. 이용해주셔서 감사합니다.";
                            send(clientSocket, response.c_str(), response.length(), 0);
                            
                            // 로그아웃 처리
                            isAuthenticated = false;
                            currentUserId = -1;
                            
                            // 연결 종료를 위한 메시지
                            std::string disconnectMsg = "/disconnect";
                            send(clientSocket, disconnectMsg.c_str(), disconnectMsg.length(), 0);
                            
                            // 연결 종료
                            break;
                        } else {
                            std::string response = "회원 탈퇴 처리 중 오류가 발생했습니다.";
                            send(clientSocket, response.c_str(), response.length(), 0);
                        }
                    }
                    catch (sql::SQLException& e) {
                        std::cerr << "회원 삭제 중 오류: " << e.what() << std::endl;
                        std::string response = "서버 오류가 발생했습니다: " + std::string(e.what());
                        send(clientSocket, response.c_str(), response.length(), 0);
                    }
                }
                // 로그인 명령 처리
                else if (tokens[0] == "/login" && tokens.size() >= 3) {
                    std::string username = tokens[1];
                    std::string password = tokens[2];
                    
                    try {
                        std::unique_ptr<sql::PreparedStatement> pstmt(
                            con->prepareStatement("SELECT user_id, password FROM users WHERE username = ?")
                        );
                        pstmt->setString(1, username);
                        
                        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
                        
                        if (res->next()) {
                            std::string stored_password = res->getString("password");
                            
                            if (stored_password == password) {
                                currentUserId = res->getInt("user_id");
                                isAuthenticated = true;
                                
                                std::string response = "로그인 성공! 환영합니다, " + username + "!";
                                send(clientSocket, response.c_str(), response.length(), 0);
                            } else {
                                std::string response = "비밀번호가 일치하지 않습니다.";
                                send(clientSocket, response.c_str(), response.length(), 0);
                            }
                        } else {
                            std::string response = "존재하지 않는 사용자입니다.";
                            send(clientSocket, response.c_str(), response.length(), 0);
                        }
                    }
                    catch (sql::SQLException& e) {
                        std::cerr << "로그인 처리 중 오류: " << e.what() << std::endl;
                        std::string response = "서버 오류가 발생했습니다.";
                        send(clientSocket, response.c_str(), response.length(), 0);
                    }
                }
                // 회원가입 명령 처리
                else if (tokens[0] == "/register" && tokens.size() >= 3) {
                    std::string username = tokens[1];
                    std::string password = tokens[2];
                    
                    try {
                        // 이미 존재하는 사용자인지 확인
                        std::unique_ptr<sql::PreparedStatement> checkStmt(
                            con->prepareStatement("SELECT COUNT(*) as count FROM users WHERE username = ?")
                        );
                        checkStmt->setString(1, username);
                        std::unique_ptr<sql::ResultSet> checkRes(checkStmt->executeQuery());
                        
                        if (checkRes->next() && checkRes->getInt("count") > 0) {
                            std::string response = "이미 존재하는 사용자명입니다.";
                            send(clientSocket, response.c_str(), response.length(), 0);
                        } else {
                            // 새 사용자 등록
                            std::unique_ptr<sql::PreparedStatement> pstmt(
                                con->prepareStatement("INSERT INTO users (username, password) VALUES (?, ?)")
                            );
                            pstmt->setString(1, username);
                            pstmt->setString(2, password);
                            pstmt->executeUpdate();
                            
                            std::string response = "회원가입이 완료되었습니다. 이제 로그인할 수 있습니다.";
                            send(clientSocket, response.c_str(), response.length(), 0);
                        }
                    }
                    catch (sql::SQLException& e) {
                        std::cerr << "회원가입 처리 중 오류: " << e.what() << std::endl;
                        std::string response = "서버 오류가 발생했습니다.";
                        send(clientSocket, response.c_str(), response.length(), 0);
                    }
                }
                // 알 수 없는 명령
                else {
                    std::string response = "알 수 없는 명령입니다.";
                    send(clientSocket, response.c_str(), response.length(), 0);
                }
            }
            // 일반 채팅 메시지 처리
            else if (isAuthenticated) {
                try {
                    // 메시지를 데이터베이스에 저장
                    std::unique_ptr<sql::PreparedStatement> pstmt(
                        con->prepareStatement("INSERT INTO message_log (sender_id, content, sent_at) VALUES (?, ?, NOW())")
                    );
                    pstmt->setInt(1, currentUserId);
                    pstmt->setString(2, message);
                    pstmt->executeUpdate();
                    
                    // 에코 메시지 보내기
                    send(clientSocket, buffer, strlen(buffer), 0);
                }
                catch (sql::SQLException& e) {
                    std::cerr << "메시지 저장 중 오류: " << e.what() << std::endl;
                    std::string response = "메시지 처리 중 오류가 발생했습니다.";
                    send(clientSocket, response.c_str(), response.length(), 0);
                }
            }
            // 인증되지 않은 상태에서 일반 메시지 처리
            else {
                std::string response = "먼저 로그인해주세요. (/login 사용자명 비밀번호)";
                send(clientSocket, response.c_str(), response.length(), 0);
            }
        }

        // 클라이언트 소켓 닫기
        close(clientSocket);
    }
    
    // 서버 소켓 닫기
    close(serverSocket);
}
