FROM alpine:3.21.2

RUN apk add --no-cache gcc-arm-none-eabi g++-arm-none-eabi build-base cmake python3 py3-jinja2
WORKDIR /deskhop
CMD ["sh", "-c", "cmake -S . -B build && cmake --build build"]
