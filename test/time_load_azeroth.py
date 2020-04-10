#!/usr/bin/python3

import time
import sys
import os

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

if __name__ == '__main__':
    try:
        map = time_load()

        test_path = map.find_path(-8949.95, -132.493, 83.5312, -8833.38, 628.628, 94.0066)

        print('Path from Human start to Stormwind:\n%s' % test_path)
    except:
        raise
        sys.exit(1)
    finally:
        try_print_mem_usage()

    sys.exit(0)