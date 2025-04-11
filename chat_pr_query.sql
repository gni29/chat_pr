-- 데이터베이스 자체를 UTF-8mb4로 생성
DROP DATABASE IF EXISTS login_db;
CREATE DATABASE login_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE login_db;

-- 나머지 테이블 생성 시 문자셋 지정
CREATE TABLE IF NOT EXISTS users(
  user_id INT AUTO_INCREMENT PRIMARY KEY,
  username VARCHAR(50) NOT NULL UNIQUE,
  password VARCHAR(100) NOT NULL,
  status ENUM("active","suspended") DEFAULT "suspended",
  created_at DATETIME DEFAULT NOW()
) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

CREATE TABLE message_log(
  message_id INT AUTO_INCREMENT PRIMARY KEY,
  sender_id INT,
  content TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  sent_at DATETIME DEFAULT NOW(),
  FOREIGN KEY (sender_id) REFERENCES users (user_id)
  ON DELETE SET NULL
) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

CREATE TABLE user_sessions(
  session_id INT AUTO_INCREMENT PRIMARY KEY,
  user_id INT,
  login_time DATETIME NOT NULL,
  logout_time DATETIME,
  ip_address VARCHAR(1000),
  FOREIGN KEY (user_id) REFERENCES users (user_id)
  ON DELETE CASCADE
) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;