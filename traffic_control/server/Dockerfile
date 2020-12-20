FROM debian
WORKDIR /work

RUN apt update -qq
RUN apt install -yqq iperf openssh-server sudo

RUN useradd -rm -d /home/test -s /bin/bash -g root -G sudo -u 1000 test 
RUN echo 'test:test' | chpasswd
RUN mkdir -p -m 0600 /home/test/.ssh
COPY ./client/id_rsa.pub /home/test/.ssh/id_rsa.pub
RUN chown -R test /home/test/.ssh

ADD ./server/ /work
CMD [ "bash", "run.sh" ]
