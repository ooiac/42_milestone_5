#!/bin/bash
set -e

DB_PASSWORD=$(cat /run/secrets/db_password 2>/dev/null || echo "${MYSQL_PASSWORD}")
CREDS=$(cat /run/secrets/credentials 2>/dev/null || echo "${WP_ADMIN_USER}:${WP_ADMIN_PASSWORD}")
ADMIN_USER=$(echo "${CREDS}" | cut -d: -f1)
ADMIN_PASSWORD=$(echo "${CREDS}" | cut -d: -f2)

until mysql -h"${MYSQL_HOST}" -u"${MYSQL_USER}" -p"${DB_PASSWORD}" "${MYSQL_DATABASE}" \
    --skip-ssl -e "SELECT 1;" > /dev/null 2>&1; do
    echo "Waiting for MariaDB..."
    sleep 2
done

if [ ! -f /var/www/html/wp-config.php ]; then
    cd /var/www/html

    wp core download --allow-root --locale=en_US

    wp config create \
        --dbname="${MYSQL_DATABASE}" \
        --dbuser="${MYSQL_USER}" \
        --dbpass="${DB_PASSWORD}" \
        --dbhost="${MYSQL_HOST}" \
        --allow-root

    wp core install \
        --url="https://${DOMAIN_NAME}" \
        --title="${WP_TITLE}" \
        --admin_user="${ADMIN_USER}" \
        --admin_password="${ADMIN_PASSWORD}" \
        --admin_email="${WP_ADMIN_EMAIL}" \
        --skip-email \
        --allow-root

    wp user create "${WP_USER}" "${WP_EMAIL}" \
        --role=author \
        --user_pass="${WP_PASSWORD}" \
        --allow-root

    wp config set WP_CACHE true --raw --allow-root
    wp config set WP_REDIS_HOST "${REDIS_HOST}" --allow-root
    wp config set WP_REDIS_PORT "${REDIS_PORT}" --raw --allow-root
    wp plugin install redis-cache --activate --allow-root
    wp redis enable --allow-root || true
fi

chown -R www-data:www-data /var/www/html
exec php-fpm8.2 -F