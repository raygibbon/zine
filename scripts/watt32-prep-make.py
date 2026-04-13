#!/usr/bin/env python3
import sys


def any_defined(tokens, symbols):
    return any(token in symbols for token in tokens)


def active_now(stack):
    return all(frame["active"] for frame in stack)


def outer_active(stack):
    if len(stack) <= 1:
        return True
    return all(frame["active"] for frame in stack[:-1])


def push_frame(stack, condition, line_number):
    parent = active_now(stack)
    active = parent and condition
    stack.append({"line": line_number, "parent": parent, "active": active, "matched": condition, "taken": active})


def main(argv):
    if len(argv) < 4:
        print("usage: watt32-prep-make.py INPUT OUTPUT SYMBOL...", file=sys.stderr)
        return 2

    src_path = argv[1]
    dst_path = argv[2]
    symbols = set(arg.upper() for arg in argv[3:])
    stack = []

    with open(src_path, "r", encoding="latin-1") as src, open(dst_path, "w", encoding="latin-1") as dst:
        for line_number, raw_line in enumerate(src, 1):
            line = raw_line.rstrip("\n")
            stripped = line.lstrip()

            if stripped.startswith("@"):
                cleaned = stripped[1:].split("#", 1)[0]
                parts = cleaned.split()
                directive = parts[0].lower() if parts else ""
                tokens = [part.upper() for part in parts[1:]]

                if directive == "ifdef":
                    push_frame(stack, any_defined(tokens, symbols), line_number)
                    continue
                if directive == "ifndef":
                    push_frame(stack, not any_defined(tokens, symbols), line_number)
                    continue
                if directive == "elifdef":
                    if not stack:
                        print("elifdef without if", file=sys.stderr)
                        return 1
                    frame = stack[-1]
                    condition = any_defined(tokens, symbols)
                    frame["active"] = frame["parent"] and not frame["matched"] and condition
                    frame["matched"] = frame["matched"] or condition
                    frame["taken"] = frame["taken"] or frame["active"]
                    continue
                if directive == "else":
                    if not stack:
                        print("else without if", file=sys.stderr)
                        return 1
                    frame = stack[-1]
                    frame["active"] = frame["parent"] and not frame["matched"]
                    frame["matched"] = True
                    frame["taken"] = frame["taken"] or frame["active"]
                    continue
                if directive == "endif":
                    if not stack:
                        print("endif without if", file=sys.stderr)
                        return 1
                    stack.pop()
                    continue

            if active_now(stack):
                if "WATCOM" in symbols:
                    tail = line.rstrip()
                    if tail.endswith("\\"):
                        line = tail[:-1] + "&"
                    line = line.replace("\\", "/")
                dst.write(line + "\n")

    if len(stack) == 1 and stack[0]["line"] == 1:
        stack.pop()

    if stack:
        print("unterminated conditional block", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
