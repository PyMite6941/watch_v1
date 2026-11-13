"""Minimal S-expression reader/writer for KiCad files."""


class Sym(str):
    """An unquoted token (as opposed to a quoted string)."""


def parse(text):
    pos = 0
    n = len(text)
    stack = [[]]
    while pos < n:
        c = text[pos]
        if c in " \t\r\n":
            pos += 1
        elif c == "(":
            stack.append([])
            pos += 1
        elif c == ")":
            done = stack.pop()
            stack[-1].append(done)
            pos += 1
        elif c == '"':
            pos += 1
            out = []
            while text[pos] != '"':
                if text[pos] == "\\":
                    pos += 1
                    out.append({"n": "\n", "t": "\t"}.get(text[pos], text[pos]))
                else:
                    out.append(text[pos])
                pos += 1
            pos += 1
            stack[-1].append("".join(out))
        else:
            start = pos
            while pos < n and text[pos] not in ' \t\r\n()"':
                pos += 1
            stack[-1].append(Sym(text[start:pos]))
    return stack[0][0]


def dump(node, indent=0):
    if isinstance(node, list):
        simple = all(not isinstance(x, list) for x in node)
        if simple:
            return "(" + " ".join(dump(x) for x in node) + ")"
        pad = "\n" + "\t" * (indent + 1)
        parts = []
        for i, x in enumerate(node):
            s = dump(x, indent + 1)
            parts.append(s if i == 0 or not isinstance(x, list) and i < 2 else pad + s)
        return "(" + " ".join(parts[:1]) + "".join(
            (" " + p) if not p.startswith("\n") else p for p in parts[1:]
        ) + "\n" + "\t" * indent + ")"
    if isinstance(node, Sym):
        return str(node)
    if isinstance(node, str):
        return '"' + node.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\n") + '"'
    if isinstance(node, float):
        s = f"{node:.4f}".rstrip("0").rstrip(".")
        return s if s not in ("-0", "") else "0"
    return str(node)


def find(node, name):
    return [x for x in node if isinstance(x, list) and x and x[0] == name]


def first(node, name):
    r = find(node, name)
    return r[0] if r else None
