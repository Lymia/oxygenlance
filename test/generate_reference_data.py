#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3 -p nodejs

import subprocess
import sys
import os

dir_path = os.path.dirname(os.path.realpath(__file__))
os.chdir(dir_path)

source_order = list(filter(lambda x: len(x) != 0, open("warriors.idx").read().split("\n")))

def main():
    with open("reference.data", "w") as f:
        for left in source_order:
            for right in source_order:
                result = subprocess.check_output(['node', 'egojsout.js', left, right]).decode("latin1")
                print(f"{left} vs {right}: {result.strip()}")
                f.write(result)

if __name__ == '__main__':
    main()
