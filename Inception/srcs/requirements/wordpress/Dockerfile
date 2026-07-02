FROM debian:bookworm

RUN apt-get update && apt-get install -y \
    php8.2-fpm \
    php8.2-mysql \
    php8.2-redis \
    wget \
    curl \
    mariadb-client \
    && rm -rf /var/lib/apt/lists/*

RUN wget -q https://raw.githubusercontent.com/wp-cli/builds/gh-pages/phar/wp-cli.phar \
    && chmod +x wp-cli.phar \
    && mv wp-cli.phar /usr/local/bin/wp

COPY conf/www.conf /etc/php/8.2/fpm/pool.d/www.conf
COPY tools/setup-wordpress.sh /setup-wordpress.sh
RUN chmod +x /setup-wordpress.sh

RUN mkdir -p /var/www/html && chown -R www-data:www-data /var/www/html
RUN mkdir -p /run/php

EXPOSE 9000

ENTRYPOINT ["/setup-wordpress.sh"]