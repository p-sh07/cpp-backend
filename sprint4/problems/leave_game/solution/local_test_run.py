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
postgres
ps@ps-pc:sprint4$ sudo ./leave_game/run.sh 
[+] Building 1.3s (19/19) FINISHED                                                          docker:default
 => [internal] load build definition from Dockerfile                                                  0.0s
 => => transferring dockerfile: 1.84kB                                                                0.0s
 => [internal] load metadata for docker.io/library/ubuntu:22.04                                       1.1s
 => [internal] load metadata for docker.io/library/gcc:11.3                                           1.0s
 => [internal] load .dockerignore                                                                     0.0s
 => => transferring context: 2B                                                                       0.0s
 => [internal] load build context                                                                     0.0s
 => => transferring context: 3.07kB                                                                   0.0s
 => [run 1/5] FROM docker.io/library/ubuntu:22.04@sha256:0e5e4a57c2499249aafc3b40fcd541e9a456aab7296  0.0s
 => [build 1/8] FROM docker.io/library/gcc:11.3@sha256:396a90d889629f292b11b4e51536c79e3a8f0f4dfcdb7  0.0s
 => CACHED [run 2/5] RUN groupadd -r www && useradd -r -g www www                                     0.0s
 => CACHED [build 2/8] RUN apt update &&     apt install -y       python3-pip       cmake     &&      0.0s
 => CACHED [build 3/8] COPY conanfile.txt /app/                                                       0.0s
 => CACHED [build 4/8] RUN mkdir /app/build && cd /app/build &&     conan install .. --build=missing  0.0s
 => CACHED [build 5/8] COPY ./src /app/src                                                            0.0s
 => CACHED [build 6/8] COPY ./tests /app/tests                                                        0.0s
 => CACHED [build 7/8] COPY CMakeLists.txt /app/                                                      0.0s
 => CACHED [build 8/8] RUN cd /app/build &&     cmake -DCMAKE_BUILD_TYPE=Debug .. &&     cmake --bui  0.0s
 => CACHED [run 3/5] COPY --from=build /app/build/bin/game_server /app/                               0.0s
 => CACHED [run 4/5] COPY ./data /app/data                                                            0.0s
 => CACHED [run 5/5] COPY ./static /app/static                                                        0.0s
 => exporting to image                                                                                0.0s
 => => exporting layers                                                                               0.0s
 => => writing image sha256:bad3ea55c1f2b86b0c9e5b7ddd33e058a77c18b49e27e43d9fe5d82d877f3232          0.0s
 => => naming to docker.io/library/leave_game                                                         0.0s
Error response from daemon: No such container: postgres
=========================================== test session starts ===========================================
platform linux -- Python 3.10.4, pytest-7.1.2, pluggy-1.0.0
rootdir: /home/ps/cpp-backend/cpp-backend-tests-practicum
plugins: quickcheck-0.9.0, parallel-0.1.1, xprocess-0.20.0
collected 52 items                                                                                        
pytest-parallel: 4 workers (processes), 1 test per worker (thread)
........................................FF..............................................................
================================================ FAILURES =================================================
________________________________ test_two_sequential_tribes_records[map1] _________________________________

postgres_server = <cpp_server_api.CppServer object at 0x746481f5e1d0>, map_id = 'map1'

    def test_two_sequential_tribes_records(postgres_server, map_id):
        red_foxes = Tribe(postgres_server, map_id, num_of_players=50, prefix='Red fox')
        r_time = get_retirement_time()
    
        for _ in range(0, random.randint(10, 35)):
            red_foxes.randomized_move()
    
        red_foxes.update_scores()
        red_foxes.stop()
    
        tick_seconds(postgres_server, r_time)
        red_foxes.add_time(r_time)
        tribe_records = red_foxes.get_list()
        records = get_records(postgres_server)
        compare(records, tribe_records)
    
        orange_raccoons = Tribe(postgres_server, map_id, num_of_players=50, prefix='Orange Raccoon')
    
        for _ in range(0, random.randint(10, 35)):
>           orange_raccoons.randomized_move()

../../tests/test_s04_leave_game.py:377: 
_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _

self = <test_s04_leave_game.Tribe object at 0x746481fc3f70>

    def randomized_move(self):
        r_time = get_retirement_time()
        self.randomized_turn()
        ticks = random.randint(100, min(10000, int(r_time*900)))
        seconds = ticks / 1000
        self.add_time(seconds)
        # print("tick->", seconds)
>       tick_seconds(self.server, seconds)

../../tests/test_s04_leave_game.py:173: 
_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _

server = <cpp_server_api.CppServer object at 0x746481f5e1d0>, seconds = 8.736

    def tick_seconds(server, seconds: float):
>       server.tick(int(seconds*1_000))

../../tests/test_s04_leave_game.py:189: 
_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _

self = <cpp_server_api.CppServer object at 0x746481f5e1d0>, ticks = 8736

    def tick(self, ticks: int):
        request = 'api/v1/game/tick'
        header = {'content-type': 'application/json'}
        data = {"timeDelta": ticks}
        res = self.request('POST', header, request, json=data)
>       self.validate_response(res)

../../tests/cpp_server_api.py:319: 
_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _

res = None

    @staticmethod
    def validate_response(res: requests.Response):
>       if res.status_code != 200:
E       AttributeError: 'NoneType' object has no attribute 'status_code'

../../tests/cpp_server_api.py:342: AttributeError
------------------------------------------ Captured stdout call -------------------------------------------
('Connection aborted.', RemoteDisconnected('Remote end closed connection without response'))
============================================ warnings summary =============================================
../../../.venv/lib/python3.10/site-packages/pytest_quickcheck/plugin.py:112
  /home/ps/cpp-backend/.venv/lib/python3.10/site-packages/pytest_quickcheck/plugin.py:112: PytestDeprecationWarning: A private pytest class or function was used.
    _randomize = Mark("randomize", args, {})

-- Docs: https://docs.pytest.org/en/stable/how-to/capture-warnings.html
------------------------- generated xml file: /home/ps/cpp-backend/leave_game.xml -------------------------
========================================= short test summary info =========================================
FAILED ../../tests/test_s04_leave_game.py::test_two_sequential_tribes_records[map1] - AttributeError: 'N...
================================ 1 failed, 51 passed, 1 warning in 49.23s =================================
postgres
