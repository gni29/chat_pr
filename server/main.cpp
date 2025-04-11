#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <locale>
#include "chat.hpp"
#include "signin.hpp"

void showMenu() {
    std::cout << "\n==== coding on chat ====\n"
        << "1. 서버 시작    2. 종료" << std::endl
        << "선택: ";
}

int main() {
    // Mac UTF-8 support
    std::locale::global(std::locale("en_US.UTF-8"));
    
    sql::mysql::MySQL_Driver* driver;
    std::unique_ptr<sql::Connection> con;
    
    try {
        driver = sql::mysql::get_mysql_driver_instance();
        con.reset(driver->connect("tcp://127.0.0.1:3306", "root", ""));
        con->setSchema("login_db");
        // 문자셋 설정 추가
        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        stmt->execute("SET NAMES utf8mb4");
        stmt->execute("SET CHARACTER SET utf8mb4");
        stmt->execute("SET character_set_connection=utf8mb4");
        int choice;
        while (true) {
            showMenu();
            std::cin >> choice;

            switch (choice) {
                case 1:
                    // 채팅 서버 시작 - 로그인 절차는 서버 내에서 처리
                    chat(con);
                    break;
                case 2:
                    std::cout << "프로그램 종료" << std::endl;
                    return 0;
                default:
                    std::cout << "잘못된 입력" << std::endl;
                    break;
            }
        }
    }
    catch (sql::SQLException& e) {
        std::cerr << "DB 연결 실패: " << e.what() << std::endl;
    }
}
