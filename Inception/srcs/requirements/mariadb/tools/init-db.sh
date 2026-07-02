#!/bin/bash
set -e

DB_ROOT_PASSWORD=$(cat /run/secrets/db_root_password 2>/dev/null || echo "${MYSQL_ROOT_PASSWORD}")
DB_PASSWORD=$(cat /run/secrets/db_password 2>/dev/null || echo "${MYSQL_PASSWORD}")

if [ ! -d "/var/lib/mysql/mysql" ]; then
    echo "=== Initializing MariaDB data directory ==="
    mysql_install_db --user=mysql --datadir=/var/lib/mysql > /dev/null 2>&1
fi

DB_EXISTS=0
if [ -d "/var/lib/mysql/${MYSQL_DATABASE}" ]; then
    DB_EXISTS=1
fi

echo "=== Starting MariaDB ==="
mysqld --user=mysql &
MYSQLD_PID=$!

echo "Waiting for MariaDB to start..."
READY=0
for i in $(seq 30 -1 1); do
    if mysqladmin --socket=/run/mysqld/mysqld.sock -hlocalhost ping --silent 2>/dev/null; then
        READY=1
        break
    fi
    sleep 1
done

if [ "$READY" = "0" ]; then
    echo "ERROR: MariaDB failed to start in 30 seconds"
    exit 1
fi

if [ "$DB_EXISTS" = "0" ]; then
    echo "=== Creating WordPress database and user ==="
    mysql --socket=/run/mysqld/mysqld.sock -hlocalhost -u root << EOF
ALTER USER 'root'@'localhost' IDENTIFIED BY '${DB_ROOT_PASSWORD}';
DELETE FROM mysql.user WHERE User='';
DELETE FROM mysql.user WHERE User='root' AND Host NOT IN ('localhost', '127.0.0.1', '::1');
CREATE DATABASE IF NOT EXISTS \`${MYSQL_DATABASE}\`;
CREATE USER IF NOT EXISTS '${MYSQL_USER}'@'%' IDENTIFIED BY '${DB_PASSWORD}';
GRANT ALL PRIVILEGES ON \`${MYSQL_DATABASE}\`.* TO '${MYSQL_USER}'@'%';
FLUSH PRIVILEGES;
EOF
    echo "=== MariaDB initialized successfully ==="
fi

wait "$MYSQLD_PID"
