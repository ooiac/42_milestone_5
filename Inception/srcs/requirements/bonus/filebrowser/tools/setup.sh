#!/bin/bash
set -e

if [ ! -f /database.db ]; then
    filebrowser config init
    filebrowser config set --address 0.0.0.0 --port 8082 --root /srv --log stdout
    filebrowser users add "${FB_ADMIN_USER}" "${FB_ADMIN_PASSWORD}" --perm.admin
fi

exec filebrowser --config /.filebrowser.json