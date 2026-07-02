FROM debian:bookworm

RUN apt-get update && apt-get install -y \
    curl \
    && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL \
    https://github.com/filebrowser/filebrowser/releases/download/v2.27.0/linux-amd64-filebrowser.tar.gz \
    | tar -xzC /usr/local/bin filebrowser \
    && chmod +x /usr/local/bin/filebrowser

COPY conf/filebrowser.json /.filebrowser.json
COPY tools/setup.sh /setup.sh
RUN chmod +x /setup.sh

EXPOSE 8082

ENTRYPOINT ["/setup.sh"]
