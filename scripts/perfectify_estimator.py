#!/usr/bin/python3

import sys

if len(sys.argv) != 5:
    print("Usage: %s table_start table_end total_tables time_per_table" % sys.argv[0])
    print()
    print("     table_start: the number of the table to start with")
    print("       table_end: the number of the table to end on")
    print("    total_tables: the total number of tables")
    print("  time_per_table: the number of seconds spent processing each table")
    print()
    exit(-1)

start = int(sys.argv[1])
end = int(sys.argv[2])
total = int(sys.argv[3])
time_per_table = float(sys.argv[4])

num_steps = 0
for i in range(start, end):
    for j in range(i + 1, total):
        num_steps = num_steps + 1

num_seconds = time_per_table * num_steps
num_minutes = num_seconds / 60
num_hours = num_minutes / 60
num_days = num_hours / 24

print("Number of steps: %u" % num_steps)
print()
print("Estimated time to complete:")
print("  Seconds: %.1f" % num_seconds)
print("  Minutes: %.1f" % num_minutes)
print("  Hours:   %.1f" % num_hours)
print("  Days:    %.1f" % num_days)
