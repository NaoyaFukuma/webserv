FROM debian:latest

RUN apt-get update && \
    apt-get install -y build-essential gcc cmake curl

WORKDIR	/app/tests/echo_server

ENTRYPOINT ["./docker-entrypoint.sh"]
CMD ["/bin/bash", "/app/tests/echo_server/test.sh"]
