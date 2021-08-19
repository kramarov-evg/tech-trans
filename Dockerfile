FROM ubuntu:latest as build

WORKDIR /app_build
RUN groupadd -r estimation && useradd -r -g estimation estimator
RUN su estimator
ENV TZ=Etc/UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && \
    echo $TZ > /etc/timezone && \
    apt-get update && \
    apt-get install -y cmake libopencv-dev build-essential

ADD ./ ./
RUN cmake . && cmake --build .

ENTRYPOINT ["./EstimateDirection"]