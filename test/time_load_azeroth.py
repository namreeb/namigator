#!/usr/bin/python3

import time
import sys
import os
import math

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'lib')))

import pathfind

# Depends on the psutil python package:
# pip install psutil
def try_print_mem_usage():
    try:
        import psutil
        mem_gb = psutil.Process().memory_info().rss / (1024.0**3)
        print('Memory usage: %.2f Gb' % mem_gb)
    except ModuleNotFoundError:
        return

def time_load():
    start = time.time()
    map = pathfind.Map(r'f:\nav_vanilla', 'Azeroth')
    load_time = time.time() - start

    print('Loaded Azeroth map in %.2f seconds' % load_time)

    start = time.time()
    count = map.load_all_adts()
    load_time = time.time() - start

    print('Loaded entire Azeroth mesh (%d tiles) in %d seconds' % (count, load_time))

    return map

def path_length(path):
    result = 0
    for i in range(1, len(path)):
        delta_x = path[i-1][0] - path[i][0]
        delta_y = path[i-1][1] - path[i][1]
        delta_z = path[i-1][2] - path[i][2]

        result += delta_x * delta_x + delta_y * delta_y + delta_z * delta_z

    return math.sqrt(result)

if __name__ == '__main__':
    try:
        map = time_load()

        start = time.time()
        test_path = map.find_path(-8949.95, -132.493, 83.5312, -8833.38, 628.628, 94.0066)
        find_path_time = time.time() - start

        print('Path from Human start to Stormwind (length = %.2f, took %.2f seconds):' % (path_length(test_path), find_path_time))
        for point in test_path:
            print('    (%.2f, %.2f, %.2f)' % (point[0], point[1], point[2]))
    except:
        raise
        sys.exit(1)
    finally:
        try_print_mem_usage()

    sys.exit(0)