FROM mbenabda/esp8266-rtos-sdk:latest

RUN apt-get update && apt-get install -y apt-utils && DEBIAN_FRONTEND=noninteractive apt-get install -y udev cmake
RUN useradd -u 1000 -U -G dialout -d /home/dev -m -s /bin/bash dev
RUN su dev -c '/usr/bin/python -m pip install --user -r /opt/sdk/requirements.txt'

COPY sdk_patches /opt/sdk_patches
RUN patch -d /opt/sdk -p 1 </opt/sdk_patches/001_idf_monitor.patch
