FROM alpine:latest
RUN apk add --update \
    curl \
    && rm -rf /var/cache/apk/* \
    && curl -fLO https://glare.now.sh/tindy2013/subconverter/subconverter_linux64.tar.gz \
    && mkdir -p /opt/ \
    && tar zxvf subconverter_linux64.tar.gz -C  /opt/  \
    && rm -rf subconverter_linux64.tar.gz
CMD cd /opt/subconverter && ./subconverter
