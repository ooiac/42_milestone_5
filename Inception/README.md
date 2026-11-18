*42 Common Core project by `caida-si`.*

# Inception

A WordPress stack built with Docker Compose. Each service has its own Dockerfile based on `debian:bookworm` rather than using prebuilt WordPress, Nginx, or MariaDB service images. The mandatory stack is Nginx, WordPress/PHP-FPM, and MariaDB; additional services extend it.

## Services

| Container | Role |
| --- | --- |
| **Nginx** | HTTPS entry point and reverse proxy, using a local self-signed TLS certificate. |
| **WordPress + PHP-FPM** | Runs the site and forwards its database queries to MariaDB. |
| **MariaDB** | Stores WordPress content and account data. |
| **Redis** | WordPress object cache (bonus). |
| **vsftpd** | FTP access to the WordPress files (bonus). |
| **Static site** | An HTML/CSS portfolio served separately (bonus). |
| **Adminer** | Database management interface (bonus). |
| **File Browser** | Browser-based file management (bonus). |

```text
HTTPS :443 -> Nginx -> WordPress / PHP-FPM :9000 -> MariaDB :3306
                  |                 |
                  |                 +-> Redis :6379
                  +-> Adminer / static site
FTP :21, passive ports -> vsftpd -> WordPress volume
HTTP :8080 -> File Browser
```

Services resolve each other's names through the Compose bridge network (`inception`). Database and site files are persisted in named volumes backed by host directories under `/home/caida-si/data/`, so deleting a container doesn't automatically delete the data. The host directory and hostname are specific to the project setup; adapt them if you're running it elsewhere.

## Setup

Requires Docker and Docker Compose v2 on Linux. Docker commands need to work for your user; membership in the `docker` group is one option, but that group grants root-equivalent Docker access.

1. Add the local hostname:

   ```sh
   echo '127.0.0.1 caida-si.42.fr' | sudo tee -a /etc/hosts
   ```

2. Create `srcs/.env` with the variables required by the Compose configuration. The expected entries are described in [`DEV_DOC.md`](DEV_DOC.md). Don't commit real credentials.

3. Create the secret files expected by the services:

   ```sh
   mkdir -p secrets
   printf '%s\n' '<root_password>' > secrets/db_root_password.txt
   printf '%s\n' '<db_password>'   > secrets/db_password.txt
   printf '%s\n' 'wp_master:<password>' > secrets/credentials.txt
   ```

   Replace the placeholders with real values, and keep these files private. Docker mounts them under `/run/secrets/` in the relevant containers rather than exposing their contents through ordinary environment variables.

4. Build and start:

   ```sh
   make
   ```

5. Open `https://caida-si.42.fr` and accept the warning for the local self-signed certificate.

The main Makefile drives Docker Compose; `USER_DOC.md` covers day-to-day use, while `DEV_DOC.md` explains the configuration, secrets, and implementation.

## A few implementation details

- Nginx is the HTTPS entry point for the WordPress site and reverse-proxied services. TLS terminates there before traffic reaches internal containers.
- WordPress runs through PHP-FPM rather than an Apache image. MariaDB has its own initialization and persistent data directory.
- Redis caches WordPress objects; it is not a replacement for the database.
- FTP, Adminer, and File Browser are optional administrative interfaces, with separate access and security considerations.
- Inside the containers, network service names such as `mariadb` are resolved by Compose DNS. `localhost` would refer to the current container, not the database container.

**Be careful with cleanup targets.** This Makefile has host-wide Docker cleanup commands, including volume cleanup. `make clean` or `make fclean` can affect unrelated Docker resources on the machine. Inspect those targets before using them on a system with other containers or important volumes.

## References and AI use

- [Docker Compose](https://docs.docker.com/compose/)
- [Nginx documentation](https://nginx.org/en/docs/)
- [MariaDB knowledge base](https://mariadb.com/kb/en/)
- [WP-CLI](https://wp-cli.org/)

Claude was used as a learning and debugging tool, particularly for Docker networking, Nginx proxying, secrets, and MariaDB initialization. One specific issue involved `MYSQL_HOST`: it made the client use TCP despite an explicit socket setting, so the initialization path was adjusted to use the local Unix socket. The code and Compose configuration were written and reviewed by `caida-si`.
