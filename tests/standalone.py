"""Compile isolated headers and pasted contents, with no sibling dependencies."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
COMPILER = os.environ.get("CXX", "g++")
CASES = {
    "sorted_set": "sorted_set::SortedSet<int> s; assert(s.add(2)); assert(!s.add(2)); assert(s.pop() == 2);",
    "sorted_multiset": "sorted_set::SortedMultiset<int> m; m.add(2); m.add(2); assert(m.count(2) == 2); assert(m.discard(2));",
    "bucket_list": "sorted_set::BucketList<int> b; b.append(2); b.insert(0, 3); assert(b.pop() == 2);",
}
for names in [(name,) for name in CASES] + [tuple(CASES)]:
    for pasted in (False, True):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            contents = []
            for name in names:
                header = (ROOT / "include" / f"{name}.hpp").read_text()
                if pasted:
                    contents.append(header)
                else:
                    (folder / f"{name}.hpp").write_text(header)
                    contents.append(f'#include "{name}.hpp"')
            source = "\n".join(contents) + "\n#include <cassert>\nint main() {\n"
            source += "\n".join(CASES[name] for name in names) + "\n}\n"
            (folder / "main.cpp").write_text(source)
            subprocess.run([COMPILER, "-std=c++17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "main.cpp", "-o", "check"], cwd=folder, check=True)
            subprocess.run([str(folder / "check")], check=True)
            print("Standalone", "paste" if pasted else "header", "OK:", ", ".join(names))
