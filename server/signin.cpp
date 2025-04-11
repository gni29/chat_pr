//
//  signin.cpp
//  db_socket_server
//
//  Created by Pyoung Jin Ji on 4/10/25.
//

#include "signin.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mysql/jdbc.h>

void createMember(std::unique_ptr<sql::Connection>& con) {
    std::string name, password;
    std::cin.ignore();
    std::cout << "[Member register]\n name : ";
    getline(std::cin, name);
    std::cout << "Password : ";
    getline(std::cin, password);

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("INSERT INTO users (username, password) VALUES (?, ?)")
        );
        pstmt->setString(1, name);
        pstmt->setString(2, password);
        pstmt->executeUpdate();
        std::cout << "Register complete" << std::endl;
    }
    catch (sql::SQLException& e) {
        std::cerr << "»∏ø¯ µÓ∑œ ø¿∑˘: " << e.what() << std::endl;
    }
}

void updatePassword(std::unique_ptr<sql::Connection>& con) {
    std::string name, password;
    
    std::cin.ignore();
    std::cout << "Name: ";
    std::getline(std::cin, name);
    std::cout << "password: ";
    std::getline(std::cin, password);

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("UPDATE members SET name=?, password=? WHERE user_id=?")
        );
        pstmt->setString(1, name);
        pstmt->setString(2, password);
        int result = pstmt->executeUpdate();
        std::cout << "Update complete" << std::endl;
    }
    catch (sql::SQLException& e) {
        std::cerr << "Update error: " << e.what() << std::endl;
    }
}

void deleteMember(std::unique_ptr<sql::Connection>& con) {
    int user_id;
    std::cout << "Delete user ID: ";
    std::cin >> user_id;

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("DELETE FROM users WHERE user_id = ?")
        );
        pstmt->setInt(1, user_id);
        int result = pstmt->executeUpdate();
        std::cout << "Delete complete" << std::endl;
    }
    catch (sql::SQLException& e) {
        std::cerr << "Delete error: " << e.what() << std::endl;
    }
}

bool login(std::unique_ptr<sql::Connection>& con) {
    std::string username, input_password;
    std::cin.ignore();
    std::cout << "ID : ";
    getline(std::cin, username);
    std::cout << "Password : ";
    getline(std::cin, input_password);

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("SELECT password FROM users WHERE username = ?")
        );
        pstmt->setString(1, username);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            std::string stored_password = res->getString("password");
            
            if (stored_password == input_password) {
                std::cout << "로그인 성공!" << std::endl;
                return true; // 로그인 성공 시 true 반환
            } else {
                std::cout << "비밀번호가 일치하지 않습니다." << std::endl;
            }
        } else {
            std::cout << "존재하지 않는 사용자입니다." << std::endl;
        }
    }
    catch (sql::SQLException& e) {
        std::cerr << "로그인 중 오류: " << e.what() << std::endl;
    }
    
    return false; // 로그인 실패 시 false 반환
}
