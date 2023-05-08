FROM debian:latest

RUN apt-get update && \
	apt-get install -y build-essential gcc tini libboost-test-dev

WORKDIR /app

CMD ["/usr/bin/tini", "--", "/bin/bash", "-c", "sleep infinity"]
