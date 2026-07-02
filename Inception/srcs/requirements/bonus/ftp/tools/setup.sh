#!/bin/bash
set -e

FTP_PASSWORD=$(cat /run/secrets/credentials 2>/dev/null | cut -d: -f2 || echo "${FTP_PASSWORD}")

if ! id -u "$FTP_USER" > /dev/null 2>&1; then
    useradd -m -d /home/ftpuser/ftp -s /bin/sh "${FTP_USER}"
    echo "${FTP_USER}:${FTP_PASSWORD}" | chpasswd
fi

mkdir -p /var/run/vsftpd/empty

exec vsftpd /etc/vsftpd.conf
