#!/usr/bin/python3

import os
import sys
import tempfile
import shutil
import time
import math

sys.path.append(os.path.realpath(os.path.join(os.path.dirname(__file__), '..', 'lib')))

import mapbuild
import pathfind

def approximate(a, b, epsilon=0.002):
    return abs(a-b) <= epsilon

def test_build(temp_dir):
	data_dir = os.path.dirname(__file__)

	start = time.time()
	mapbuild.build_map(data_dir, temp_dir, "development", 8, "")
	stop = time.time()

	print("Map built in {} seconds".format(int(stop-start)))

def test_pathfind(temp_dir):
	map_data = pathfind.Map(temp_dir, "development")

	x = 16271.025391
	y = 16845.421875

	adt_x, adt_y = map_data.load_adt_at(x, y)

	z_values = map_data.query_z(x, y)

	expected_z_values = [35.610786, 46.300201]

	if len(z_values) != len(expected_z_values):
		raise Exception("Expected {} Z values, found {}".format(
			len(expected_z_values), len(z_values)))

	expected_z_values.sort()
	z_values.sort()

	for i in range(0, len(z_values)):
		print("Z value #{}: Expected = {}, Found = {}".format(i,
			expected_z_values[i], z_values[i]))
		if not approximate(expected_z_values[i], z_values[i]):
			raise Exception("Expected Z {} Found {}".format(
				expected_z_values[i], z_values[i]))

	print("Z value check succeeded")

	def compute_path_length(path):
		result = 0
		for i in range(1, len(path)):
			delta_x = path[i-1][0] - path[i][0]
			delta_y = path[i-1][1] - path[i][1]
			delta_z = path[i-1][2] - path[i][2]

			result += delta_x * delta_x + delta_y * delta_y + delta_z * delta_z

		return math.sqrt(result)

	path = map_data.find_path(16303.294922, 16789.242188, 45.219631,
		16200.139648, 16834.345703, 37.028622)
	path_length = compute_path_length(path)

	if len(path) < 5 or path_length > 100:
		raise Exception("Path invalid.  Length: {} Distance: {}".format(
			len(path), path_length))

	print("Pathfind check succeeded")

	zone, area = map_data.get_zone_and_area(x, y, expected_z_values[-1])

	if zone != 22 or area != 22:
		raise Exception("Zone check failed.  Zone: {} Area: {}".format(zone, area))

	print("Zone check succeeded")

def main():
	temp_dir = tempfile.mkdtemp()
	print("Temporary directory: {}".format(temp_dir))

	try:
		test_build(temp_dir)
		test_pathfind(temp_dir)
	finally:
		print("Removing temporary directory...")
		shutil.rmtree(temp_dir)

if __name__ == "__main__":
	main()
	sys.exit(0)