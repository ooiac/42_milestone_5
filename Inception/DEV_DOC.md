# Developer Documentation

## Prerequisites

- Docker Engine 24+ and Docker Compose v2
- User added to the `docker` group:
  ```bash
  sudo usermod -aG docker $USER
  # Log out and back in, or run: newgrp docker
  ```
- Domain in `/etc/hosts`:
  ```bash
  echo "127.0.0.1 caida-si.42.fr" | sudo tee -a /etc/hosts
  ```

## Project structure

```
Inception/
├── Makefile
├── README.md
├── USER_DOC.md
├── DEV_DOC.md
├── secrets/                        # Docker secrets — NOT committed to git
│   ├── db_root_password.txt        # MariaDB root password
│   ├── db_password.txt             # WordPress DB user password
│   └── credentials.txt            # WordPress admin: "user:password"
└── srcs/
    ├── .env                        # Environment variables — NOT committed to git
    ├── docker-compose.yml
    └── requirements/
        ├── mariadb/
        │   ├── Dockerfile
        │   ├── conf/50-server.cnf
        │   └── tools/init-db.sh
        ├── nginx/
        │   ├── Dockerfile
        │   ├── conf/nginx.conf
        │   ├── conf/default.conf
        │   └── tools/setup.sh
        ├── wordpress/
        │   ├── Dockerfile
        │   ├── conf/www.conf
        │   └── tools/setup-wordpress.sh
        └── bonus/
            ├── adminer/
            ├── filebrowser/
            ├── ftp/
            ├── redis/
            └── static-site/
```

## Environment variables (`srcs/.env`)

```env
DOMAIN_NAME=caida-si.42.fr
DATA_DIR=/home/caida-si/data

# MariaDB
MYSQL_ROOT_PASSWORD=<root_password>
MYSQL_DATABASE=wordpress
MYSQL_USER=wp_user
MYSQL_PASSWORD=<db_password>
MYSQL_HOST=mariadb

# WordPress
WP_TITLE=Inception - caida-si
WP_ADMIN_USER=wp_master
WP_ADMIN_PASSWORD=<admin_password>
WP_ADMIN_EMAIL=<email>
WP_USER=wp_reader
WP_PASSWORD=<reader_password>
WP_EMAIL=<reader_email>

# Redis
REDIS_HOST=redis
REDIS_PORT=6379

# FTP
FTP_USER=ftp_user
FTP_PASSWORD=<ftp_password>

# File Browser
FB_ADMIN_USER=fb_admin
FB_ADMIN_PASSWORD=<fb_password>
```

## Secrets setup

```bash
mkdir -p secrets
echo "<root_password>"      > secrets/db_root_password.txt
echo "<db_password>"        > secrets/db_password.txt
echo "wp_master:<password>" > secrets/credentials.txt
```

Secrets are mounted inside containers at `/run/secrets/<name>`. Init scripts read from there with a fallback to the corresponding env var.

## Makefile

| Target | What it does |
|---|---|
| `make` / `make up` | `mkdir -p` data dirs + `docker compose up --build -d` |
| `make down` | `docker compose down` |
| `make stop` | `docker compose stop` |
| `make start` | `docker compose start` |
| `make logs` | `docker compose logs -f` |
| `make clean` | `down` + `docker system prune -af` |
| `make fclean` | `clean` + remove all volumes + `sudo rm -rf /home/caida-si/data` |
| `make re` | `fclean` + `up` |

## Docker Compose

Run from the repo root (where the Makefile is). The Compose file is at `srcs/docker-compose.yml` and the env file at `srcs/.env`.

```bash
# Equivalent to make up:
docker compose -f srcs/docker-compose.yml --env-file srcs/.env up --build -d

# Rebuild a single service:
docker compose -f srcs/docker-compose.yml --env-file srcs/.env up --build -d <service>

# View logs for one service:
docker compose -f srcs/docker-compose.yml --env-file srcs/.env logs -f <service>
```

## Data persistence

Volumes use the local bind-mount driver pointing to the host:

| Volume | Host path | Container path |
|---|---|---|
| `wp_db` | `/home/caida-si/data/mariadb` | `/var/lib/mysql` |
| `wp_files` | `/home/caida-si/data/wordpress` | `/var/www/html` |

The `make up` target creates these directories automatically with `sudo mkdir -p`.

After `make fclean`, all data is wiped. `make re` performs a full rebuild + fresh WordPress installation.

## Key implementation notes

**MariaDB init**: On `debian:bookworm`, MariaDB 10.11 uses `unix_socket` auth for root by default. The init script starts `mysqld` in the background, waits for it with `mysqladmin ping -hlocalhost`, then runs SQL via the socket using `mysql -hlocalhost --socket=...`. The `-hlocalhost` flag is critical — without it, the `MYSQL_HOST=mariadb` env var (injected by `env_file: .env`) causes the client to connect via TCP, which fails.

**nginx TLS**: A self-signed certificate is generated at container startup via `openssl req -x509`. TLS 1.2 and 1.3 are the only accepted protocols.

**WordPress setup**: WP-CLI downloads and configures WordPress on first run. A `wp-config.php` presence check prevents re-installation on restart.

**Redis**: `protected-mode no` is required in `redis.conf` because the WordPress container connects from a different IP on the Docker network.
