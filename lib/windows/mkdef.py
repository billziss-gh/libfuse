#!/usr/bin/python3

import re, sys

class Parser:
    def __init__(self, file):
        self._iter = self._tokenize(file)
    @staticmethod
    def _tokenize(file):
        for line in file:
            line = line.rstrip()
            if not line or line.startswith("#"):
                continue
            for part in re.split("(\s+|[:;}{])", line):
                if not part.strip():
                    continue
                yield part
    def consume(self, expect):
        try:
            token = next(self._iter)
        except StopIteration:
            return None
        for e in expect:
            if "V" == e:
                if re.match("[A-Za-z_0-9.]+", token):
                    return token
            elif "S" == e:
                if re.match("[A-Za-z_*][A-Za-z_0-9*]*", token):
                    return token
            elif token == e:
                return token
        raise SyntaxError("expected token " + e + ", got \"" + token + "\"")

submap = {}
for arg in sys.argv[1:]:
    parts = arg.split("=", 2)
    submap[parts[0]] = parts[1] if 2 <= len(parts) else None

vers = {}
parser = Parser(sys.stdin)
while True:
    ver = parser.consume(["V"])
    if ver is None:
        break
    parser.consume(["{"])
    syms = []
    glob = False
    while True:
        sym = parser.consume(["S", "global", "local", "}"])
        if "}" == sym:
            break
        if sym in ["global", "local"]:
            parser.consume([":"])
            glob = "global" == sym
        else:
            parser.consume([";"])
            if glob:
                syms.append(sym)
    anc = parser.consume(["V", ";"])
    if ";" != anc:
        syms.extend(vers[anc])
        parser.consume([";"])
    vers[ver] = syms

if False:
    for ver in sorted(vers.keys()):
        print(ver)
        print("    " + str(vers[ver]))

ver = max(vers.keys(), key=lambda v: [int(p) if p.isdigit() else p for p in re.split(r"(\d+)", v)])
syms = set(vers[ver])

print("; automatically generated; rebuild with:")
print(";     python3 mkdef.py " + " ".join(sys.argv[1:]) + " <../fuse_versionscript >fuse.def")
print("EXPORTS")
for sym in sorted(syms):
    if sym in submap:
        if submap[sym]:
            print("    " + sym + "=" + submap[sym])
    else:
        print("    " + sym)
