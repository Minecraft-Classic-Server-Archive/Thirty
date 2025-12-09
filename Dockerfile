ARG ARCH

FROM registry.firestick.games/sean/thirty-ci-images/tumbleweed-$ARCH:latest AS build

COPY . /thirty

RUN \
  cd /thirty && \
  meson setup docker-build --buildtype=release  --prefix=/thirty-install && \
  meson compile -C docker-build && \
  meson install -C docker-build

FROM registry.opensuse.org/opensuse/tumbleweed:latest

RUN \
  zypper -n ref && \
  zypper -n install libz-ng2 libcurl4 libgomp1 && \
  zypper -n clean -a && \
  rm -rf /var/log/{lastlog,tallylog,zypper.log,zypp/history,YaST2}

COPY --from=build /thirty-install/bin/thirty /usr/bin/thirty

VOLUME /data
WORKDIR /data
ENTRYPOINT /usr/bin/thirty -c /data/settings.ini
