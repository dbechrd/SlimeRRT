import glob
import os
import sys

def usage(args):
    print("Unexpected args:")
    for arg in args:
        print(arg)
    print("Usage: fbs [json|gen] <schema>")

def run(cmd):
    print(cmd)
    os.system(cmd)

if (len(sys.argv) != 3):
    usage(sys.argv)
    sys.exit(1)

action = sys.argv[1]
schema_pattern = sys.argv[2]

schemas = glob.glob(schema_pattern)
for schema in schemas:
    name = os.path.splitext(schema)[0]
    if action == "json":
        #run(f"flatc -t --size-prefixed {name}.fbs -- {name}.dat")
        run(f"flatc -t {name}.fbs -- {name}.dat")
    elif action == "gen":
        #run(f"flatc --cpp --gen-mutable --gen-name-strings --reflect-names -o ..\..\include\ {schema}")
        run(f"flatc --cpp --gen-mutable --gen-name-strings -o ..\..\include\ {schema}")
    else:
        print(f"Unknown action {action}")
