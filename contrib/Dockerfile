FROM amd64/ubuntu:latest AS base

LABEL MAINTAINER cade <cade.call@mediciventures.com>

EXPOSE 8323/tcp
EXPOSE 8767/tcp

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && \
	apt install -y software-properties-common
	

#Install runtime dependencies
RUN apt update && \
	apt install -y --no-install-recommends \
	bash net-tools libminiupnpc10 \
	libevent-2.1 libevent-pthreads-2.1 \
	libdb5.3 libdb5.3++ \
	libboost-system1.84 libboost-filesystem1.84 libboost-chrono1.84 \
	libboost-program-options1.84 libboost-thread1.84 \
	libzmq5 && \
	apt-get clean

FROM base AS build

#Install build dependencies
RUN apt update && \
	apt install -y --no-install-recommends \
	bash net-tools build-essential libtool autotools-dev automake \
	pkg-config libssl-dev libevent-dev bsdmainutils python3 \
	libboost-system1.84-dev libboost-filesystem1.84-dev libboost-chrono1.84-dev \
	libboost-program-options1.84-dev libboost-test1.84-dev libboost-thread1.84-dev \
	libzmq3-dev libminiupnpc-dev libdb5.3-dev libdb5.3++-dev && \
	apt-get clean

#Build Soteria from source
COPY . /home/soteria/build/Soteria/
WORKDIR /home/soteria/build/Soteria
RUN ./autogen.sh && ./configure --disable-tests --with-gui=no && make

FROM base AS final

#Add our service account user
RUN useradd -ms /bin/bash soteria && \
	mkdir /var/lib/soteria && \
	chown soteria:soteria /var/lib/soteria && \
	ln -s /var/lib/soteria /home/soteria/.soteria && \
	chown -h soteria:soteria /home/soteria/.soteria

VOLUME /var/lib/soteria

#Copy the compiled binaries from the build
COPY --from=build /home/soteria/build/Soteria/src/soteriad /usr/local/bin/soteriad
COPY --from=build /home/soteria/build/Soteria/src/soteria-cli /usr/local/bin/soteria-cli

WORKDIR /home/soteria
USER soteria

CMD /usr/local/bin/soteriad -datadir=/var/lib/soteria -printtoconsole -onlynet=ipv4
