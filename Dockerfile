# docker build -t deskhop-build .
# docker run --rm -it -v .:/mnt deskhop-build \
#   bash -c 'cd /mnt && rm -rf build && cmake -S . -B build && cmake --build build'

FROM ubuntu

RUN set -ex && \
	export DEBIAN_FRONTEND=noninteractive && \
	apt --assume-yes update && \
	apt --assume-yes install \
		build-essential \
		cmake \
		gcc-arm-none-eabi \
		libnewlib-arm-none-eabi \
		python3
