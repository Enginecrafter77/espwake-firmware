FROM mbenabda/esp8266-rtos-sdk:latest

RUN apt-get update && apt-get install -y apt-utils && DEBIAN_FRONTEND=noninteractive apt-get install -y udev cmake

COPY sdk_patches /opt/sdk_patches

RUN patch -d /opt/sdk -p 1 </opt/sdk_patches/001_idf_monitor.patch
