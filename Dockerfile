FROM ubuntu:latest
RUN apt-get update -q \
    && apt-get install -qy unzip make wget git python3 pipenv
ARG TOOLCHAIN_LINK=
RUN cd / && wget ${TOOLCHAIN_LINK} -O toolchain.zip -q && \
    unzip -q toolchain.zip
RUN cd / && ln -s /as4e-ide/plugins/com.atmel.avr.toolchains.linux.x86_64_3.0.0.201009140852/os/linux/x86_64/bin/avr32-gcc
COPY pm_240.h /app/
RUN cp /app/pm_240.h /as4e-ide/plugins/com.atmel.avr.toolchains.linux.x86_64_3.0.0.201009140852/os/linux/x86_64/avr32/include/avr32/pm_231.h
RUN pip3 install urllib3 pipenv -U
WORKDIR /app

# usage example
# $ sudo docker build . -t nk-storage-build --build-arg TOOLCHAIN_LINK=
# or with podman
# $ podman build . -t storage --build-arg=TOOLCHAIN_LINK=https://ww1.microchip.com/downloads/archive/avr32studio-ide-2.6.0.753-linux.gtk.x86_64.zip
# $ podman run -it -v $PWD:/app:z --rm storage
# (inside container) $ cd /app/src && make clean && make CC=/avr32-gcc
# 	-> built firmware hex files are in the ./src directory
