FROM debian:latest

RUN apt-get update && \
    apt-get install -y build-essential gcc cmake curl

ENTRYPOINT ["/app/tests/echo_server/server/docker-entrypoint.sh"]
CMD ["/app/webserv", "/app/tests/echo_server/server/config.txt"]
