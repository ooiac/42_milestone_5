*This project has been created as part of the 42 curriculum by caida-si*

# Inception

## Description

A Docker-based infrastructure running a full WordPress stack with bonus services, built from scratch. Every service runs in its own container built from `debian:bookworm`. No pre-built images from Docker Hub are used.

**Services:**
- **Nginx** — reverse proxy, TLS 1.2/1.3 (self-signed), port 443 only
- **WordPress** — WordPress 7 + PHP-FPM 8.2
- **MariaDB** — database for WordPress
- **Redis** — object cache for WordPress *(bonus)*
- **FTP** — vsftpd pointing to the WordPress volume *(bonus)*
- **Static site** — portfolio page in HTML/CSS, no PHP *(bonus)*
- **Adminer** — database management UI *(bonus)*
- **File Browser** — web-based file manager for the WordPress volume *(bonus)*

All containers communicate over a user-defined Docker bridge network (`inception`). Data is persisted via named volumes bound to `/home/caida-si/data/` on the host.

```
  HTTPS :443 ──► nginx ──► wordpress (php-fpm :9000)
                    │   └──► adminer (:8080)
                    │   └──► static-site (:80)
                    │
  FTP :21 ─────────► ftp
  :21000–21010       │
                    │   wordpress ──► mariadb (:3306)
                    │   wordpress ──► redis (:6379)
  HTTP :8080 ──────► filebrowser
```

## Instructions

### Prerequisites

- Docker and Docker Compose v2
- User in the `docker` group (`sudo usermod -aG docker $USER`, then re-login)

### Setup

1. Add the domain to `/etc/hosts`:
   ```bash
   echo "127.0.0.1 caida-si.42.fr" | sudo tee -a /etc/hosts
   ```

2. Create `srcs/.env` with the required variables (see `DEV_DOC.md`)

3. Create the secrets:
   ```bash
   mkdir -p secrets
   echo "<root_password>"      > secrets/db_root_password.txt
   echo "<db_password>"        > secrets/db_password.txt
   echo "wp_master:<password>" > secrets/credentials.txt
   ```

4. Build and start:
   ```bash
   make
   ```

5. Open `https://caida-si.42.fr` in your browser (accept the self-signed certificate warning)

For full usage and developer instructions see `USER_DOC.md` and `DEV_DOC.md`.

## Resources

### References

- [Docker documentation](https://docs.docker.com/)
- [Docker Compose documentation](https://docs.docker.com/compose/)
- [MariaDB knowledge base](https://mariadb.com/kb/en/)
- [WP-CLI documentation](https://wp-cli.org/)
- [Nginx documentation](https://nginx.org/en/docs/)
- [vsftpd documentation](https://security.appspot.com/vsftpd.html)
- [File Browser documentation](https://filebrowser.org/)

### AI usage

Claude (Anthropic) was used as a learning and debugging assistant throughout this project:

- Explaining how Docker bridge networking and inter-container DNS resolution work
- Identifying the root cause of MariaDB connection failures: the `MYSQL_HOST` environment variable causes the `mysql` client to use TCP even when `--socket` is specified; fixed by adding `-hlocalhost` to force unix socket mode
- Understanding MariaDB `unix_socket` authentication on Debian bookworm (root connects via socket, no password required for local init)
- Explaining the difference between Docker secrets (`/run/secrets/`) and environment variables in terms of visibility via `docker inspect`
- Structuring nginx as a reverse proxy for multiple backend services on different paths

All code was written and understood by caida-si. AI was used to explain concepts and debug issues, not to generate code blindly.
