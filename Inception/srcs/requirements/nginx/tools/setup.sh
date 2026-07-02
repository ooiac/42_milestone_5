#!/bin/bash
set -e

if [ ! -f /etc/nginx/ssl/nginx.crt ]; then
    openssl req -x509 -nodes -days 365 -newkey rsa:4096 \
        -keyout /etc/nginx/ssl/nginx.key \
        -out /etc/nginx/ssl/nginx.crt \
        -subj "/C=BR/ST=SC/L=Florianopolis/O=42/OU=Inception/CN=${DOMAIN_NAME}"
fi

sed -i "s/DOMAIN_PLACEHOLDER/${DOMAIN_NAME}/g" /etc/nginx/conf.d/default.conf

exec nginx -g "daemon off;"