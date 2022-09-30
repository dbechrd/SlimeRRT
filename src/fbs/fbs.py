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
        # --gen-mutable       allow user to scalar fields in-place
        # --gen-name-strings  generate string converters for e.g. enums
        # --gen-object-api    allow user to Pack/Unpack to native C++ objects
        # --reflect-names     generate RTTI for field types, names, etc.
        run(f"flatc --cpp --gen-mutable --gen-name-strings --gen-object-api -o ..\..\include\ {schema}")
    else:
        print(f"Unknown action {action}")
