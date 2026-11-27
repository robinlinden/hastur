# SPDX-FileCopyrightText: 2026 Robin Lindén <dev@robinlinden.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

import subprocess
import sys
import tempfile
from pathlib import Path

from python.runfiles import runfiles


# TODO(robinlinden): Support something like a --timeout_s argument to only fuzz
# for a specified amount of time. Alternatively, --iterations?
def main():
    runfiles_env = runfiles.Create()
    if runfiles_env is None:
        raise RuntimeError("Unable to initialize Bazel runfiles")

    tested_target = runfiles_env.Rlocation(sys.argv[1])
    domato = runfiles_env.Rlocation(sys.argv[2])
    if tested_target is None:
        raise RuntimeError(f"Unable to locate tested target: {sys.argv[1]}")

    if domato is None:
        raise RuntimeError(f"Unable to locate Domato executable: {sys.argv[2]}")

    with tempfile.TemporaryDirectory(prefix="domato-fuzz-") as temp_dir:
        sample = Path(temp_dir) / "sample.html"
        tested_target_args = sys.argv[3:]
        while True:
            # TODO(robinlinden): Generate multiple file at the same time and
            # pass them one-by-one. Domato's generation is slow.
            result = subprocess.run([domato, "--file", str(sample)], check=False)
            if result.returncode:
                return result.returncode

            result = subprocess.run(
                [tested_target, *tested_target_args, sample.as_uri()], check=False
            )

            if result.returncode:
                return result.returncode


if __name__ == "__main__":
    sys.exit(main())
