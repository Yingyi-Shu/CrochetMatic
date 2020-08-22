"""
Loop instruction duplication script for CrochetMatic

Generates GCode for multiple loops on one needle given
the GCode for a singular loop on one needle.
Script runs on command line for Python 2.7. 

Argument 1: original GCode filename
Argument 2: number of loops

Prerequisite: GCode file given is valid
"""
import sys

# WARNING: output file will overwrite files with same filename
try:
    inp = sys.argv[1]
    assert(inp.split('.')[1] == 'gcode'), '1st arg must be a gcode file'
    num_repeats = int(sys.argv[2])
    assert(num_repeats > 0), '2nd arg must a positive int'
except IndexError:
    print('error: need 2 arguments: [gcode file] [number]')
    sys.exit(1)
except ValueError:
    print('error: 2nd arg must be int')
    sys.exit(1)

output_name = '%s_dup_%d.gcode' % (inp.split('.')[0], num_repeats)

with open(inp) as src:
    data = src.read()

with open(output_name, "w") as dst:
    for i in range(num_repeats-1):
        dst.write(data)
        dst.write("\n")
    dst.write(data)

print('%s successfully created' % output_name)