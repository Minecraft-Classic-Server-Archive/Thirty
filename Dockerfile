FROM debian:stable AS build

RUN \
  export DEBIAN_FRONTEND=noninteractive && \
  export TZ=Etc/UTC && \
  apt-get update && \
  apt-get -y upgrade && \
  apt-get -y install --option=Dpkg::Options::=--force-confdef \
    build-essential mercurial mercurial-evolve zlib1g-dev meson ninja-build

RUN hg clone https://dev.firestick.games/sean/thirty
RUN \
  cd thirty && \
  meson setup build --buildtype=release --prefix=/thirty-install && \
  meson compile -C build && \
  meson install -C build

FROM debian:stable-slim

RUN \
  export DEBIAN_FRONTEND=noninteractive && \
  export TZ=Etc/UTC && \
  apt-get update && \
  apt-get -y upgrade && \
  apt-get -y install --option=Dpkg::Options::=--force-confdef \
    zlib1g-dev

COPY --from=build /thirty-install/bin/thirty /usr/bin/thirty

VOLUME /data
WORKDIR /data
ENTRYPOINT /usr/bin/thirty -c /data/settings.ini
