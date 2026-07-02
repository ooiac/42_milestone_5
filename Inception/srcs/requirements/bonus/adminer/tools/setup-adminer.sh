#!/bin/bash
set -e

mkdir -p /tmp/php-sessions
exec php8.2 -S 0.0.0.0:8080 -t /var/www/adminer -d session.save_path=/tmp/php-sessions