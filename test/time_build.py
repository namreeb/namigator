#!/usr/bin/python3

import time
import sys
import os
import math
import argparse

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'lib')))

import mapbuild

# Depends on the psutil python package:
# pip install psutil
def try_print_mem_usage():
    try:
        import psutil
        mem_gb = psutil.Process().memory_info().rss / (1024.0**3)
        print('Memory usage: %.2f Gb' % mem_gb)
    except ModuleNotFoundError:
        return

def time_build_bvh(wow_data, nav_data, jobs):
    start = time.time()
    mapbuild.build_bvh(wow_data, nav_data, jobs)
    build_time = time.time() - start

    print('BVH built in %.2f seconds' % build_time)

def time_build_map(wow_data, nav_data, jobs, name):
    start = time.time()
    # TODO: Add gameobject CSV
    mapbuild.build_map(wow_data, nav_data, name, jobs, '')
    build_time = time.time() - start

    print('Map %s built in %.2f seconds' % (name, build_time))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-w', '--wowdata', help='Path to wow data', default=r'f:\wow 1.12.1\Data')
    parser.add_argument('-n', '--navdata', help='Path to output navigation data', default=r'f:\nav_vanilla')
    parser.add_argument('-j', '--jobs', help='How many jobs to run', type=int, default=4)

    args = parser.parse_args()

    try:
        if not os.path.isdir(args.wowdata):
            raise RuntimeError('Wow data directory %s does not exist' % args.wowdata)

        if os.path.isdir(args.navdata):
            raise RuntimeError('Navigation directory %s already exists' % args.navdata)

        # First, build BVH data
        time_build_bvh(args.wowdata, args.navdata, args.jobs)

        # Second, build Azeroth
        time_build_map(args.wowdata, args.navdata, args.jobs, 'Azeroth')

        # Third, build Kalimdor
        time_build_map(args.wowdata, args.navdata, args.jobs, 'Kalimdor')
    except:
        raise
        sys.exit(1)
    finally:
        try_print_mem_usage()

    sys.exit(0)