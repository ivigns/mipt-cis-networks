ssh-keygen -q -t rsa -N '' -f ./client/id_rsa
docker-compose build && docker-compose up
