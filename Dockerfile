FROM ubuntu:latest

# Set non-interactive mode for apt-get
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y sudo cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential python3 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /project

CMD ["bash", "-c", "cmake -S . -B build && cmake --build build"]
