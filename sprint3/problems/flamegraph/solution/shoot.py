import argparse
import subprocess
import time
import random
import shlex

RANDOM_LIMIT = 1000
SEED = 123456789
random.seed(SEED)

AMMUNITION = [
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1


def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server


def run(command, output=None):
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.DEVNULL)
    return process

def run_perf(server_pid):
    print(server_pid)
    perf = subprocess.Popen(["perf", "record", "-g", "-p", server_pid, "-o", "perf.data"])
    return perf

def stop(process, wait=False):
    if process.poll() is None and wait:
        process.wait()
    process.terminate()


def shoot(ammo):
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')

def make_graph():
    graph_file = open("graph.svg", "w")

    perf = subprocess.Popen(["perf", "script", "-i", "perf.data"], stdout=subprocess.PIPE) 
    fgp1 = subprocess.Popen(["./FlameGraph/stackcollapse-perf.pl"], stdin=perf.stdout, stdout=subprocess.PIPE)
    subprocess.Popen(["./FlameGraph/flamegraph.pl"], stdin=fgp1.stdout, stdout=graph_file)
    
    time.sleep(1)
    graph_file.close()
    print("Done making graph")


server = run(start_server())
perf = run_perf(str(server.pid))

make_shots()

stop(server)
time.sleep(1)

stop(perf)
time.sleep(5)

print('make graph.svg')
make_graph()

print('Job done')
time.sleep(5)


    #Alt method for running a shell cmd, not recommended:
    #cmd = "sudo perf script -i perf.data | ./FlameGraph/stackcollapse-perf.pl | ./FlameGraph/flamegraph.pl > graph.svg"
    #ps = subprocess.Popen(cmd,shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    #output = ps.communicate()[0]
    #print(output)