FROM ubuntu:latest
RUN apt-get update -q \
    && apt-get install -qy unzip make wget binutils-avr git python3 pipenv
# COPY avr32studio-ide-2.6.0
ARG TOOLCHAIN_LINK=
RUN cd / && wget ${TOOLCHAIN_LINK} -O toolchain.zip -q && \
    unzip -q toolchain.zip && \
    ln -s /as4e-ide-offical32/plugins/com.atmel.avr.toolchains.linux.x86_3.0.0.201009140852/os/linux/x86/bin/avr32-gcc
WORKDIR /app

# CMD cd /app/src && make CC=/avr32-gcc
# sudo docker build . -t nk-storage-build --build-arg TOOLCHAIN_LINK=
