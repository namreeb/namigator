#!/usr/bin/python3

import os
import sys
import tempfile
import shutil
import argparse
import time
import math

sys.path.append(os.path.realpath(os.path.join(os.path.dirname(__file__), '..', 'lib')))

import mapbuild
import pathfind

def approximate(a, b, epsilon=0.002):
    return abs(a-b) <= epsilon

def compute_path_length(path):
    result = 0
    for i in range(1, len(path)):
        delta_x = path[i-1][0] - path[i][0]
        delta_y = path[i-1][1] - path[i][1]
        delta_z = path[i-1][2] - path[i][2]

        result += delta_x * delta_x + delta_y * delta_y + delta_z * delta_z

    return math.sqrt(result)

def time_build_map(wow_data, nav_data, go_csv, jobs, name):
    if go_csv is None:
        go_csv = ''

    start = time.time()
    mapbuild.build_map(wow_data, nav_data, name, jobs, go_csv)
    return time.time() - start

def test_build_data(wow_data, nav_data, jobs):
    # Step 1: Build BVH data
    start = time.time()
    bvh_file_count = mapbuild.build_bvh(wow_data, nav_data, jobs)
    build_time = time.time() - start

    min_bvh_count = 230
    if bvh_file_count < min_bvh_count:
        raise RuntimeError('Expected %d BVH files, found only %d' % (
            min_bvh_count, bvh_file_count))

    print('BVH generation completed in %.2f seconds' % build_time)

    # TODO:
    # Step 2: Query game object info from database
    go_csv = None

    # Step 2: Build necessary maps
    # What maps to build
    maps = ['Azeroth']
    for map in maps:
        build_time = time_build_map(wow_data, nav_data, go_csv, jobs, map)
        print('Map %s built in %.2f seconds' % (map, build_time))

def test_use_data(nav_data):
    azeroth = pathfind.Map(nav_data, 'Azeroth')

    # Test precise Z values
    adt_x, adt_y = azeroth.load_adt_at(1748.29, -661.98)

    # There is a hole at this location, and ensuring a single value is returned
    # is evidence that the hole is being identified correctly
    z_values = azeroth.query_heights(1748.29, -661.98)
    assert len(z_values) == 1 and approximate(z_values[0], 45.058178)

    z_values = azeroth.query_heights(1753.842285, -662.430908)

    # No hole here, should return value from ADTs
    assert len(z_values) == 1 and approximate(z_values[0], 44.429478)

    print('Z query tests succeeded')

    # This doorway in Deathknell is known to be problematic
    adt_x, adt_y = azeroth.load_adt_at(1926, 1548.88)
    path = azeroth.find_path(1942.09863, 1541.59216, 90.514, 1940.185, 1522.914, 88.229)
    path_length = compute_path_length(path)

    assert len(path) > 3 and path_length < 30

    print('Deathknell doorway test succeeded')

    azeroth.load_adt_at(-9068, 413)
    zone, area = azeroth.get_zone_and_area(-9068.827148, 413.834045, 92.931786)
    zone2, area2 = azeroth.get_zone_and_area(-9069.045898, 413.626892, 92.868759)
    zone3, area3 = azeroth.get_zone_and_area(-9086.793945, 443.588013, 92.940720)

    assert zone == 1519 and area == 1519

    print('Stormwind area check succeeded')

    assert zone2 == 12 and area2 == 12

    print('Elwynn Forest area check succeeded')

    assert zone3 == 1519 and area3 == 1617

    print('Stormwind border area check succeeded')

    azeroth.load_adt_at(1573, 262)

    zone, area = azeroth.get_zone_and_area(1573.982666, 262.304504, -59.160473)

    assert zone == 1497 and area == 1497

    print('Undercity area check succeeded')
    
    # This WMO is rotated around all axis and serves as a check that the translation is working well
    azeroth.load_adt_at(-1173.688, -2046.505)
    z_values = azeroth.query_heights(-1173.688, -2046.505)
    assert len(z_values) == 2 and approximate(z_values[0], 37.323)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--wowdata', help='Path to wow data', required=True)
    parser.add_argument('-n', '--navdata', help='Use existing navigation data')
    parser.add_argument('-j', '--jobs', help='How many jobs to run', type=int, default=4)

    args = parser.parse_args()

    assert os.path.isdir(args.wowdata)

    if args.navdata is not None:
        assert os.path.isdir(args.navdata)
        args.build_nav_data = False
    else:
        args.navdata = tempfile.mkdtemp(prefix='navtest')
        args.build_nav_data = True
        print('Using temporary directory %s for nav data' % args.navdata)

    try:
        if args.build_nav_data:
            test_build_data(args.wowdata, args.navdata, args.jobs)

        test_use_data(args.navdata)
    finally:
        if args.build_nav_data:
            print('Removing temporary directory')
            shutil.rmtree(args.navdata)

    return 0

if __name__ == '__main__':
    sys.exit(main())

