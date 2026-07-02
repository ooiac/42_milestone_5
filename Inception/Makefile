COMPOSE_FILE	= srcs/docker-compose.yml
ENV_FILE		= srcs/.env

DOMAIN_LOGIN	= $(shell grep -E '^DOMAIN_NAME' $(ENV_FILE) | cut -d= -f2 | cut -d. -f1)
DATA_DIR = /home/$(DOMAIN_LOGIN)/data

all: up

up:
	@sudo mkdir -p $(DATA_DIR)/mariadb $(DATA_DIR)/wordpress
	@sudo chown -R $(USER):$(USER) /home/$(DOMAIN_LOGIN)
	@docker compose -f $(COMPOSE_FILE) --env-file $(ENV_FILE) up --build -d

down:
	@docker compose -f $(COMPOSE_FILE) --env-file $(ENV_FILE) down

stop:
	@docker compose -f $(COMPOSE_FILE) --env-file $(ENV_FILE) stop

start:
	@docker compose -f $(COMPOSE_FILE) --env-file $(ENV_FILE) start

logs:
	@docker compose -f $(COMPOSE_FILE) --env-file $(ENV_FILE) logs -f

clean:
	@docker compose -f $(COMPOSE_FILE) --env-file $(ENV_FILE) down
	@docker system prune -af

fclean: clean
	@docker volume rm -f $$(docker volume ls -q 2>/dev/null) 2>/dev/null; true
	@sudo rm -rf $(DATA_DIR) 2>/dev/null; true

re: fclean up

.PHONY: all up down stop start logs clean fclean re