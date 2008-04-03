--
-- create databse and granting
--
DROP DATABASE IF EXISTS manager_db;
CREATE DATABASE IF NOT EXISTS manager_db;
GRANT CREATE,ALTER,DROP,SELECT,INSERT,UPDATE,DELETE ON manager_db.* 
  TO dbadmin@localhost IDENTIFIED BY 'dbadmin';
use manager_db;
--
-- users table ---
--
CREATE TABLE IF NOT EXISTS users(
	user_id		INT		NOT NULL AUTO_INCREMENT,
	user_name	CHAR(64)	NOT NULL,
	user_password	VARCHAR(64)	NOT NULL,
	user_desc	VARCHAR(255)	NOT NULL,
	user_email	VARCHAR(128)	NOT NULL,
	user_datetime	DATETIME	NOT NULL,
	PRIMARY KEY(user_id),
	UNIQUE(user_name),
	INDEX(user_name),
	INDEX(user_email),
	INDEX(user_datetime)
	)AUTO_INCREMENT=1000000;
--
-- permissions --
--
CREATE TABLE IF NOT EXISTS permissions(
	permission_id		INT		NOT NULL AUTO_INCREMENT,
	permission_name		CHAR(64)	NOT NULL,
	permission_desc		VARCHAR(255)	NOT NULL,
	PRIMARY KEY(permission_id),
	INDEX(permission_name)
	)AUTO_INCREMENT=10000;
--
-- user permissions ---
--
CREATE TABLE IF NOT EXISTS user_permissions(
	permission_id		INT		NOT NULL,
	permission_name		CHAR(32)	NOT NULL,
	user_id			INT		NOT NULL,
	user_name		CHAR(32)	NOT NULL,
	datetime		DATETIME	NOT NULL,
	UNIQUE(permission_id,user_id),
	INDEX(permission_id),
	INDEX(user_id),
	INDEX(permission_name),
	INDEX(user_name),
	INDEX(datetime)
	)Type=MyISAM;
--
-- clients table ---
--
CREATE TABLE IF NOT EXISTS clients(
	client_id		INT		NOT NULL AUTO_INCREMENT,
	client_name	CHAR(64)	NOT NULL,
	client_password	VARCHAR(64)	NOT NULL,
	client_phone	VARCHAR(32)	NOT NULL,
	client_email	    VARCHAR(128)	NOT NULL,
	client_desc	    VARCHAR(255)	NOT NULL,
	client_datetime	DATETIME	NOT NULL,
	PRIMARY KEY(client_id),
	UNIQUE(client_name),
	INDEX(client_name),
	INDEX(client_email),
	INDEX(client_datetime)
	)AUTO_INCREMENT=2000000;

--
-- action log for security ----
--
CREATE TABLE IF NOT EXISTS action_log(
	user_name	VARCHAR(64) BINARY	NOT NULL,
	action		VARCHAR(255) BINARY	NOT NULL,
	datetime	DATETIME		NOT NULL,
	ip		VARCHAR(16)		NOT NULL,
	INDEX(user_name),
	INDEX(action),
	INDEX(datetime),
	INDEX(ip)
	)Type=MyISAM;

INSERT users(user_name, user_password) VALUES('admin', md5('admin'));
INSERT permissions(permission_name) VALUES('admin');
INSERT user_permissions(permission_id, permission_name, user_id, user_name)
	VALUES(10000, 'admin', 1000000, 'admin');
INSERT user_permissions(permission_id, permission_name, user_id, user_name)
	VALUES(10001, 'data_admin', 1000000, 'admin');

