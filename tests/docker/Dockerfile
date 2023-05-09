FROM debian:latest

RUN apt-get update && \
    	apt-get install -y build-essential gcc tini cmake curl

RUN			mkdir -p /root/workdir

WORKDIR		/app

COPY ./tests/docker/docker-entrypoint.sh /app/docker-entrypoint.sh
RUN chmod +x /app/docker-entrypoint.sh

ENTRYPOINT 			["/app/docker-entrypoint.sh"]