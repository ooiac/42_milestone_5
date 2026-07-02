# User Documentation

## Starting and stopping the stack

```bash
make          # build and start all containers
make down     # stop and remove containers (data is preserved)
make stop     # pause containers without removing them
make start    # resume paused containers
make logs     # follow live logs from all containers
```

## Accessing the services

### WordPress website
- **URL**: `https://caida-si.42.fr`
- Accept the self-signed certificate warning in your browser
- The site loads directly — no installation page should appear

### WordPress admin panel
- **URL**: `https://caida-si.42.fr/wp-admin`
- **Admin user**: `wp_master` (password set in `srcs/.env` → `WP_ADMIN_PASSWORD`)
- **Regular user**: `wp_reader` (password set in `srcs/.env` → `WP_PASSWORD`)

### Adminer (database UI)
- **URL**: `https://caida-si.42.fr/adminer/`
- **System**: MySQL
- **Server**: `mariadb`
- **Username**: `wp_user`
- **Password**: value of `MYSQL_PASSWORD` in `srcs/.env`
- **Database**: `wordpress`

### Static portfolio site
- **URL**: `https://caida-si.42.fr/portfolio/`

### File Browser (WordPress file manager)
- **URL**: `http://localhost:8080`
- **Username**: value of `FB_ADMIN_USER` in `srcs/.env` (default: `fb_admin`)
- **Password**: value of `FB_ADMIN_PASSWORD` in `srcs/.env`
- Provides read-only access to the WordPress file volume

### FTP server
```bash
ftp caida-si.42.fr
# Username: value of FTP_USER in srcs/.env (default: ftp_user)
# Password: value of FTP_PASSWORD in srcs/.env
```
Passive mode ports: 21000–21010.

## Managing credentials

All credentials are stored in `srcs/.env` (not committed to git). To change a password:
1. Edit `srcs/.env`
2. Run `make re` to rebuild and restart with the new credentials

Secrets used internally by Docker are in `secrets/` (also not committed to git).

## Basic health checks

```bash
# Check all containers are running
docker compose -f srcs/docker-compose.yml ps

# Check a specific service's logs
docker logs wordpress
docker logs mariadb
docker logs nginx

# Test HTTPS is working
curl -sk https://caida-si.42.fr | grep '<title>'

# Verify TLS version
openssl s_client -connect caida-si.42.fr:443 2>/dev/null | grep Protocol

# Confirm port 80 is rejected (expected: Connection refused)
curl caida-si.42.fr:80

# Connect to MariaDB directly
docker exec -it mariadb mysql -u wp_user -p wordpress
```

## Data persistence

WordPress files and database data are stored on the host at:
- `/home/caida-si/data/wordpress/` — WordPress files
- `/home/caida-si/data/mariadb/` — MariaDB database files

This data survives container restarts and `make down`. Only `make fclean` removes it.

After a system reboot, run `make start` (if containers still exist) or `make up` to bring everything back. All previous content will be intact.
