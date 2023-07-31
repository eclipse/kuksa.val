# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php

FROM alpine:3.11 as build

RUN apk update && apk add cmake wget alpine-sdk linux-headers openssl-dev libstdc++ mosquitto-dev

ADD . /kuksa.val
RUN  rm -rf /kuksa.val/kuksa-val-server/build  && mkdir  /kuksa.val/kuksa-val-server/build
WORKDIR /kuksa.val/kuksa-val-server/build
RUN cmake ..
RUN make -j 4
RUN ls
RUN /kuksa.val/kuksa-val-server/docker/collect-deployment-artifacts.sh


FROM alpine:3.11

COPY --from=build /deploy /kuksa.val
WORKDIR /kuksa.val

ENV LOG_LEVEL=INFO
#Usually you want this to be 0.0.0.0 when using porter port expose via -p.
ENV BIND_ADDRESS=0.0.0.0

EXPOSE 8090/tcp

CMD /kuksa.val/startkuksaval.sh
