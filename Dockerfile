FROM alpine:3.21.2

RUN apk add --no-cache gcc-arm-none-eabi=14.2.0-r0 g++-arm-none-eabi=14.2.0-r1 build-base=0.5-r3 cmake=3.31.1-r0 python3=3.12.8-r1 py3-jinja2=3.1.5-r0
WORKDIR /deskhop
CMD ["sh", "-c", "cmake -S . -B build && cmake --build build"]
