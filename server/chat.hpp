//
//  chat.hpp
//  db_socket_server
//
//  Created by Pyoung Jin Ji on 4/10/25.
//

#ifndef chat_hpp
#define chat_hpp

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mysql/jdbc.h>
void chat(std::unique_ptr<sql::Connection>& con);

#endif /* chat_hpp */
