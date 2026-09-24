"""Compile and execute the Doxygen examples directly from the public headers."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
COMPILER = os.environ.get("CXX", "g++")
total = 0
for header in sorted((ROOT / "include").glob("*.hpp")):
    comments = "\n".join(
        line.lstrip()[4:] if line.lstrip().startswith("/// ") else ""
        for line in header.read_text().splitlines()
    )
    examples = re.findall(r"@code\{\.cpp\}\n(.*?)\n@endcode", comments, re.DOTALL)
    if not examples:
        raise RuntimeError(f"No executable examples in {header}")
    # Each example is a complete program. Give its main a unique name so all
    # examples can be compiled together while keeping local scopes independent.
    programs = []
    for number, example in enumerate(examples):
        if example.count("int main()") != 1:
            raise RuntimeError(f"Expected one main in {header.name} example {number}")
        programs.append(example.replace("int main()", f"void example_{number}()"))
    source = "\n".join(programs) + "\nint main() {\n"
    source += "\n".join(f"    example_{i}();" for i in range(len(examples))) + "\n}\n"
    with tempfile.TemporaryDirectory() as directory:
        folder = Path(directory)
        (folder / "examples.cpp").write_text(source)
        subprocess.run(
            [COMPILER, "-std=c++17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-I", str(ROOT / "include"), "examples.cpp", "-o", "examples"],
            cwd=folder, check=True,
        )
        subprocess.run([str(folder / "examples")], check=True)
    total += len(examples)
    print(f"{header.name}: {len(examples)} documentation examples passed")
print(f"Total: {total} documentation examples passed")
