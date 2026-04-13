#!/usr/bin/env python3
import re
import sys


DEFINE_RE = re.compile(r"^#define\s+([A-Z][A-Z0-9_]+)\s+([0-9]+)\b")


def main(argv):
    if len(argv) != 3:
        print("usage: watt32-gen-syserr.py WATCOM.ERR SYSERR.C", file=sys.stderr)
        return 2

    names_by_value = {}
    max_value = 0

    with open(argv[1], "r", encoding="latin-1") as fp:
        for line in fp:
            match = DEFINE_RE.match(line)
            if not match:
                continue

            name = match.group(1)
            value = int(match.group(2))
            if name.startswith("ERRNO_"):
                continue

            if value not in names_by_value:
                names_by_value[value] = name
            if value > max_value:
                max_value = value

    with open(argv[2], "w", encoding="latin-1") as fp:
        fp.write("/* Generated for Local Zine's Watt-32 OpenWatcom host build. */\n")
        fp.write("char __syserr000[] = \"Unknown error\";\n")
        for value in range(1, max_value + 1):
            name = names_by_value.get(value)
            if name:
                fp.write("char __syserr%03d[] = \"Error %d (%s)\";\n" % (value, value, name))

        fp.write("\nchar * _WCDATA SYS_ERRLIST[] = {\n")
        for value in range(0, max_value + 1):
            if value in names_by_value:
                fp.write("  __syserr%03d" % value)
            else:
                fp.write("  __syserr000")

            if value != max_value:
                fp.write(",")
            fp.write("\n")
        fp.write("};\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
