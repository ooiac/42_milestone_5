FROM debian:bookworm

RUN apt-get update && apt-get install -y \
    php8.2-cli \
    php8.2-mysql \
    wget \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /var/www/adminer \
    && wget -q https://github.com/vrana/adminer/releases/download/v4.8.1/adminer-4.8.1.php \
        -O /var/www/adminer/adminer.php \
    && echo '<?php require "adminer.php"; ?>' > /var/www/adminer/index.php

COPY tools/setup-adminer.sh /setup-adminer.sh
RUN chmod +x /setup-adminer.sh

EXPOSE 8080

ENTRYPOINT ["/setup-adminer.sh"]