# Inception

**by caida-si**

42 school project — a Docker-based infrastructure running a full WordPress stack with bonus services, built from scratch using only `debian:bookworm` base images.

## Architecture

```
                          ┌─────────────────────────────────────────┐
                          │              Docker Network              │
                          │               (inception)                │
                          │                                          │
  HTTPS :443 ─────────► nginx ──────► wordpress (php-fpm :9000)    │
                          │    └──────► adminer (:8080)              │
                          │    └──────► static-site (:80)            │
                          │                                          │
  FTP :21 ───────────────► ftp                                       │
  :21000–21010            │                                          │
                          │  wordpress ──► mariadb (:3306)           │
                          │  wordpress ──► redis (:6379)             │
                          │                                          │
  HTTP :8080 ────────────► filebrowser                               │
                          └─────────────────────────────────────────┘
```

All containers are built from `debian:bookworm`. No pre-built images (nginx, wordpress, mariadb official images) are used.

## Services

| Service | Description | Access |
|---|---|---|
| **nginx** | Reverse proxy with TLS (self-signed) | `https://caida-si.42.fr` |
| **wordpress** | WordPress 7 + PHP-FPM 8.2 | `https://caida-si.42.fr` |
| **mariadb** | MariaDB 10.11 database | Internal port 3306 |
| **redis** | Object cache for WordPress | Internal port 6379 |
| **adminer** | Database web UI (PHP built-in server) | `https://caida-si.42.fr/adminer/` |
| **static-site** | Portfolio site (nginx) | `https://caida-si.42.fr/portfolio/` |
| **ftp** | vsftpd FTP server (passive mode) | `ftp://caida-si.42.fr:21` |
| **filebrowser** | Web file manager for WordPress files | `http://localhost:8080` |

## Project Structure

```
Inception/
├── Makefile
├── secrets/                        # Docker secrets (gitignored)
│   ├── db_root_password.txt
│   ├── db_password.txt
│   └── credentials.txt
└── srcs/
    ├── .env                        # Environment variables (gitignored)
    ├── docker-compose.yml
    └── requirements/
        ├── mariadb/
        ├── nginx/
        ├── wordpress/
        └── bonus/
            ├── adminer/
            ├── filebrowser/
            ├── ftp/
            ├── redis/
            └── static-site/
```

## Setup

### 1. Add domain to /etc/hosts

```bash
echo "127.0.0.1 caida-si.42.fr" | sudo tee -a /etc/hosts
```

### 2. Create .env file

Create `srcs/.env`:

```env
DOMAIN_NAME=caida-si.42.fr
DATA_DIR=/home/caida-si/data
MYSQL_ROOT_PASSWORD=<root_password>
MYSQL_DATABASE=wordpress
MYSQL_USER=wp_user
MYSQL_PASSWORD=<db_password>
MYSQL_HOST=mariadb
WP_TITLE=Inception - caida-si
WP_ADMIN_USER=wp_master
WP_ADMIN_PASSWORD=<admin_password>
WP_ADMIN_EMAIL=<email>
WP_USER=wp_reader
WP_PASSWORD=<reader_password>
WP_EMAIL=<reader_email>
REDIS_HOST=redis
REDIS_PORT=6379
FTP_USER=ftp_user
FTP_PASSWORD=<ftp_password>
FB_ADMIN_USER=fb_admin
FB_ADMIN_PASSWORD=<fb_password>
```

### 3. Create secrets

```bash
mkdir -p secrets
echo "<root_password>"       > secrets/db_root_password.txt
echo "<db_password>"         > secrets/db_password.txt
echo "wp_master:<password>"  > secrets/credentials.txt
```

### 4. Create data directories

```bash
sudo mkdir -p /home/caida-si/data/mariadb /home/caida-si/data/wordpress
sudo chown -R $USER:$USER /home/caida-si
```

### 5. Run

```bash
make up
```

## Makefile Targets

| Target | Description |
|---|---|
| `make up` | Build images and start all containers (detached) |
| `make down` | Stop and remove containers |
| `make logs` | Follow logs from all containers |
| `make stop` | Stop containers without removing |
| `make start` | Start stopped containers |
| `make clean` | Remove containers + prune all Docker images/cache |
| `make fclean` | `clean` + remove volumes and data directory |
| `make re` | Full rebuild from scratch (`fclean` + `up`) |

## Accessing Services

### WordPress
- URL: `https://caida-si.42.fr` (accept the self-signed certificate)
- Admin: `https://caida-si.42.fr/wp-admin`
- Login: `wp_master` / password from `.env`

### Adminer (Database UI)
- URL: `https://caida-si.42.fr/adminer/`
- System: MySQL
- Server: `mariadb`
- User: `wp_user`
- Password: from `.env` (`MYSQL_PASSWORD`)
- Database: `wordpress`

### File Browser
- URL: `http://localhost:8080`
- Login: `fb_admin` / password from `.env` (`FB_ADMIN_PASSWORD`)
- Serves WordPress files at `/home/caida-si/data/wordpress`

### FTP
```bash
ftp caida-si.42.fr
# user: ftp_user, password from .env (FTP_PASSWORD)
# or
lftp -u ftp_user,<password> ftp://caida-si.42.fr
```

## Technical Notes

- **TLS**: Self-signed certificate generated at runtime by openssl in the nginx container. TLS 1.2 and 1.3 only.
- **MariaDB auth**: Root uses `unix_socket` auth by default on bookworm. The init script connects via socket (`-hlocalhost`) to avoid conflicts with the `MYSQL_HOST` env var.
- **Redis**: `protected-mode no` is required since WordPress connects from a different container (different IP). No password configured.
- **Data persistence**: WordPress files and MariaDB data are stored in bind-mount volumes at `/home/caida-si/data/` on the host.
- **Docker secrets**: Sensitive credentials are passed via Docker secrets (files in `/run/secrets/` inside containers), with `.env` values as fallback.
- **FTP passive mode**: Ports 21000–21010 are open for passive data connections.
