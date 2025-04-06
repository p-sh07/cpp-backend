# Создать образ на основе базового слоя gcc (там будет ОС и сам компилятор).
# 11.3 — используемая версия gcc.
FROM gcc:11.3 AS build

# Выполнить установку зависимостей внутри контейнера.
RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.64.1

# копируем conanfile.txt в контейнер и запускаем conan install
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing -s compiler.libcxx=libstdc++11 -s build_type=Debug

# только после этого копируем остальные иходники
COPY ./src /app/src
COPY ./tests /app/tests
COPY CMakeLists.txt /app/

RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Debug .. && \
    cmake --build . -j4


# Второй контейнер в том же докерфайле
FROM ubuntu:22.04 AS run

# Создадим пользователя www
RUN groupadd -r www && useradd -r -g www www
USER www

# Скопируем приложение со сборочного контейнера в директорию /app.
# Не забываем также папку data, она пригодится.
COPY --from=build /app/build/bin/game_server /app/
COPY ./data /app/data
COPY ./static /app/static
#COPY ./state /volume/state

#Env var for db
ENV DEBUG_DB_URL=postgres://postgres:Mys3Cr3t@172.17.0.2:5432/playerdb

# Запускаем игровой сервер
ENTRYPOINT ["/app/game_server", "--config-file", "/app/data/config.json", "--www-root", "/app/static"]

#Commands to launch:
#sudo docker build -t my_http_server .
#sudo docker run --rm -p 80:8080 my_http_server
#
#создать контейнер docker c постгрес
#docker run -d --name postgres-container -p 30432:5432 -e TZ=UTC -e POSTGRES_PASSWORD=Mys3Cr3t ubuntu/postgres:14-22.04_beta

#Посмотреть ID имеющихся контейнеров
#docker ps -a

#запустить контейнер
#docker start 9d87fa736f29

#контейнер с game_server
#docker run --rm -a STDERR -e GAME_DB_URL=postgres://postgres:Mys3Cr3t@172.17.0.2:5432/game_server -p 80:8080 sprint4_leave_game_test