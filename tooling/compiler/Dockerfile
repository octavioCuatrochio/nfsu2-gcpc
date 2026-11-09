# GCC 2.95.3 + binutils 2.16.1 cross toolchain for powerpc-eabi (GameCube), matching Speed.elf's compiler.
FROM debian/eol:sarge
RUN apt-get update && apt-get install -y --force-yes gcc g++ make bison flex libc6-dev patch bzip2 && apt-get clean
COPY src/ /src/
WORKDIR /build
RUN tar xjf /src/binutils-2.16.1.tar.bz2 && tar xzf /src/gcc-core-2.95.3.tar.gz && tar xzf /src/gcc-g++-2.95.3.tar.gz
RUN mkdir b-bin && cd b-bin && CC="gcc -static" ../binutils-2.16.1/configure --target=powerpc-eabi --prefix=/opt/gcc295 --disable-nls \
 && make && make install
ENV PATH=/opt/gcc295/bin:$PATH
RUN mkdir b-gcc && cd b-gcc && CC="gcc -static" ../gcc-2.95.3/configure --host=i686-pc-linux-gnu --build=i686-pc-linux-gnu --target=powerpc-eabi --prefix=/opt/gcc295 --enable-languages=c,c++ --with-newlib --disable-nls --disable-multilib \
 && make all-gcc && make install-gcc
