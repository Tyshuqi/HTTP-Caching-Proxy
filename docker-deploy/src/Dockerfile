FROM gcc:9

ARG BOOST_VERSION=1.80.0
ENV DEBIAN_FRONTEND=noninteractive
RUN cd /tmp && \
    BOOST_VERSION_MOD=$(echo $BOOST_VERSION | tr . _) && \
    wget https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_MOD}.tar.bz2 && \
    tar --bzip2 -xf boost_${BOOST_VERSION_MOD}.tar.bz2 && \
    cd boost_${BOOST_VERSION_MOD} && \
    ./bootstrap.sh --prefix=/usr/local && \
    ./b2 install && \
    rm -rf /tmp/*


RUN mkdir /var/log/erss
RUN chmod 777 /var/log/erss
ADD . /
WORKDIR /
RUN chmod 777 run.sh

ENTRYPOINT ["./main"]