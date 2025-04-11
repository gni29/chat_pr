//
//  signin.hpp
//  db_socket_server
//
//  Created by Pyoung Jin Ji on 4/10/25.
//

#ifndef signin_hpp
#define signin_hpp
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mysql/jdbc.h>
void createMember(std::unique_ptr<sql::Connection>& con);
void updatePassword(std::unique_ptr<sql::Connection>& con);
bool login(std::unique_ptr<sql::Connection>& con);
void updateMember(std::unique_ptr<sql::Connection>& con);
void deleteMember(std::unique_ptr<sql::Connection>& con);
#endif /* signin_hpp */
