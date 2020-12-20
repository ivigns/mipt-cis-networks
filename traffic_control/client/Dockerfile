FROM debian
WORKDIR /work

RUN apt update -qq
RUN apt install -yqq iperf openssh-client sshpass

ADD ./ /work
CMD [ "bash", "run.sh" ]
