FROM debian:bookworm

RUN apt-get update && apt-get install -y \
    nginx \
    && rm -rf /var/lib/apt/lists/*

COPY site/ /var/www/html/
COPY conf/site.conf /etc/nginx/conf.d/default.conf

RUN rm -f /etc/nginx/sites-enabled/default

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]