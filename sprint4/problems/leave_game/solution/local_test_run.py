ps@ps-pc:cpp-backend$ git pull
remote: Enumerating objects: 50, done.
remote: Counting objects: 100% (50/50), done.
remote: Compressing objects: 100% (21/21), done.
remote: Total 50 (delta 23), reused 47 (delta 21), pack-reused 0 (from 0)
Unpacking objects: 100% (50/50), 2.04 MiB | 4.00 MiB/s, done.
From github.com:p-sh07/cpp-backend
 * [new branch]      final4      -> origin/final4
 * [new branch]      final_task  -> origin/final_task
 * [new branch]      leave_game3 -> origin/leave_game3
   8163eae..af0a0c7  main        -> origin/main
Already up to date.
ps@ps-pc:cpp-backend$ git checkout final4
M	cpp-backend-tests-practicum
Branch 'final4' set up to track remote branch 'final4' from 'origin'.
Switched to a new branch 'final4'
ps@ps-pc:cpp-backend$ cd cpp-backend-tests-practicum/scripts/sprint4/
ps@ps-pc:sprint4$ sudo ./leave_game/run.sh 
[sudo] password for ps: 
[+] Building 33.2s (19/19) FINISHED                              docker:default
 => [internal] load build definition from Dockerfile                       0.0s
 => => transferring dockerfile: 1.84kB                                     0.0s
 => [internal] load metadata for docker.io/library/ubuntu:22.04            1.2s
 => [internal] load metadata for docker.io/library/gcc:11.3                1.2s
 => [internal] load .dockerignore                                          0.0s
 => => transferring context: 2B                                            0.0s
 => [internal] load build context                                          0.0s
 => => transferring context: 11.44kB                                       0.0s
 => [build 1/8] FROM docker.io/library/gcc:11.3@sha256:396a90d889629f292b  0.0s
 => [run 1/5] FROM docker.io/library/ubuntu:22.04@sha256:0e5e4a57c2499249  0.0s
 => CACHED [build 2/8] RUN apt update &&     apt install -y       python3  0.0s
 => CACHED [build 3/8] COPY conanfile.txt /app/                            0.0s
 => CACHED [build 4/8] RUN mkdir /app/build && cd /app/build &&     conan  0.0s
 => [build 5/8] COPY ./src /app/src                                        0.1s
 => [build 6/8] COPY ./tests /app/tests                                    0.1s
 => [build 7/8] COPY CMakeLists.txt /app/                                  0.1s
 => [build 8/8] RUN cd /app/build &&     cmake -DCMAKE_BUILD_TYPE=Debug   30.9s
 => CACHED [run 2/5] RUN groupadd -r www && useradd -r -g www www          0.0s 
 => [run 3/5] COPY --from=build /app/build/bin/game_server /app/           0.1s 
 => [run 4/5] COPY ./data /app/data                                        0.1s 
 => [run 5/5] COPY ./static /app/static                                    0.1s 
 => exporting to image                                                     0.2s 
 => => exporting layers                                                    0.2s
 => => writing image sha256:bad3ea55c1f2b86b0c9e5b7ddd33e058a77c18b49e27e  0.0s
 => => naming to docker.io/library/leave_game                              0.0s
Error response from daemon: No such container: postgres
============================= test session starts ==============================
platform linux -- Python 3.10.4, pytest-7.1.2, pluggy-1.0.0
rootdir: /home/ps/cpp-backend/cpp-backend-tests-practicum
plugins: quickcheck-0.9.0, parallel-0.1.1, xprocess-0.20.0
collected 52 items                                                             
pytest-parallel: 4 workers (processes), 1 test per worker (thread)
........................................................................................................
=============================== warnings summary ===============================
../../../.venv/lib/python3.10/site-packages/pytest_quickcheck/plugin.py:112
  /home/ps/cpp-backend/.venv/lib/python3.10/site-packages/pytest_quickcheck/plugin.py:112: PytestDeprecationWarning: A private pytest class or function was used.
    _randomize = Mark("randomize", args, {})

-- Docs: https://docs.pytest.org/en/stable/how-to/capture-warnings.html
----------- generated xml file: /home/ps/cpp-backend/leave_game.xml ------------
======================== 52 passed, 1 warning in 55.27s ========================
