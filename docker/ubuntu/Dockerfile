FROM ubuntu:latest

RUN apt-get update && \
	apt-get install -y build-essential gcc tini curl cmake libboost-all-dev vim

WORKDIR /app

CMD ["/usr/bin/tini", "--", "/bin/bash", "-c", "sleep infinity"]