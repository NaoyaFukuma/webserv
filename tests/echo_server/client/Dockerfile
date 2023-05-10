FROM debian:latest

RUN apt-get update && \
    apt-get install -y build-essential gcc cmake curl

ENTRYPOINT ["/app/tests/echo_server/client/docker-entrypoint.sh"]
CMD ["/app/tests/echo_server/client/client"]
