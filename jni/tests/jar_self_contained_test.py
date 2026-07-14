#!/usr/bin/env python3
"""Assert the published jar is self-contained.

The embedded smoke test loads via the Bazel runtime classpath, where the
dds_ffm classes are present through java_export's `exports` edge regardless of
whether they were bundled into the Maven artifact. This test instead inspects
the built jar file directly, so a regression where java_export stopped merging
the exported classes (leaving a jar with only the native resource) is caught —
a real Maven consumer depending on the coordinate would otherwise hit
NoClassDefFoundError for org.dds.ffm.Dds.

Invoked as: jar_self_contained_test.py <dds_ffm_dist jar>
"""

import re
import sys
import zipfile

_NATIVE_LIB = re.compile(r"^native/[^/]+/(lib)?dds\.(so|dylib|dll)$")


def main(argv):
    jar_path = argv[1]
    with zipfile.ZipFile(jar_path) as jar:
        names = jar.namelist()

    if "org/dds/ffm/Dds.class" not in names:
        print("FAIL: org/dds/ffm/Dds.class not bundled in", jar_path)
        return 1

    native = [n for n in names if _NATIVE_LIB.match(n)]
    if not native:
        print("FAIL: no embedded native library (native/<triplet>/lib) in", jar_path)
        return 1

    print(f"OK: jar is self-contained (Dds.class + {native[0]}).")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
