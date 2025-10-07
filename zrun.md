### 数据库

数据库是一个有组织地存储、管理和检索数据的系统。

关系型数据库使用表格来组织数据，通过行和列存储信息，典型代表有MySQL、PostgreSQL、Oracle和SQL Server，它们使用SQL语言进行操作，适合处理结构化数据和复杂的关联查询。非关系型数据库（NoSQL）则更加灵活多样，包括文档数据库如MongoDB，键值数据库如Redis，列族数据库如Cassandra，以及图数据库如Neo4j，这些数据库适合处理海量数据、高并发场景或特定的数据结构需求。

#### MySQL

每一个对象对应一行，每一列对应一个属性
SQL语言：结构化查询语言，用于管理关系型数据库

```bash
# 卸载MySQL 参考链接：https://blog.csdn.net/iehadoop/article/details/82961264
dpkg --list|grep mysql
sudo apt-get remove mysql-common
sudo apt-get autoremove --purge mysql-server-5.7
dpkg -l|grep ^rc|awk '{print$2}'|sudo xargs dpkg -P
dpkg --list|grep mysql
sudo apt-get autoremove --purge mysql-apt-config

# 安装MySQL 参考链接：https://blog.csdn.net/iehadoop/article/details/82952702
sudo apt-get install mysql-server
sudo apt install mysql-client
sudo apt install libmysqlclient-dev
sudo netstat -tap | grep mysql
mysql -u root -p

# 问题：Atlas 200i DK A2+ubuntu2204安装mysql8服务后，设备每次重启后无法启动mysql服务 
# 参考链接：https://www.hiascend.com/developer/blog/details/0286157349564895422
# mysql -uroot -p
# Enter password: 
# ERROR 2002 (HY000): Can't connect to local MySQL server through socket '/var/run/mysqld/mysqld.sock' (2)
# 解决方法：
vim /etc/mysql/mysql.conf.d/mysqld.cnf # log_error = /var/lib/mysql/error.log  # 将原本的错误日志文件移动到mysql存放数据库数据内容的默认位置处

cd /var/lib/mysql/error.log  # 移动到/var/lib/mysql/位置
touch error.log              # 创建error.log文件

#依次执行即可将/var/lib/mysql读写权限赋予所有用户
sudo chmod 777 /var
sudo chown -R mysql /var/lib/mysql
sudo chgrp -R mysql /var/lib/mysql

sudo systemctl restart mysql # 重启mysql服务

```