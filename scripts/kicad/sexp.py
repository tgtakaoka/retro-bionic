"""KiCad s-expression helpers: parse, extract a block, read symbol pins, re-indent."""
import re


def tokenize(s):
    return re.findall(r'\(|\)|"(?:[^"\\]|\\.)*"|[^\s()"]+', s)


def parse(s):
    """parse text into nested lists of tokens; strings keep their quotes"""
    toks = tokenize(s)
    pos = 0

    def rd():
        nonlocal pos
        t = toks[pos]
        pos += 1
        if t == '(':
            lst = []
            while toks[pos] != ')':
                lst.append(rd())
            pos += 1
            return lst
        return t

    out = []
    while pos < len(toks):
        out.append(rd())
    return out


def uq(t):
    return t[1:-1] if isinstance(t, str) and t.startswith('"') else t


def find(node, key):
    return [c for c in node if isinstance(c, list) and c and c[0] == key]


def extract_block(text, header):
    """raw text of the s-expression starting at the first occurrence of header, e.g. '(symbol "X"'"""
    i = text.index(header)
    d = 0
    j = i
    while True:
        c = text[j]
        if c == '"':
            j += 1
            while text[j] != '"':
                if text[j] == '\\':
                    j += 1
                j += 1
        elif c == '(':
            d += 1
        elif c == ')':
            d -= 1
            if d == 0:
                return text[i:j + 1]
        j += 1


def pins_of(symtext):
    """pin number -> (x, y, angle, length, name, type) in symbol library coordinates (unit 1)"""
    tree = parse(symtext)[0]
    pins = {}

    def walk(n):
        for c in n:
            if not isinstance(c, list) or not c:
                continue
            if c[0] == 'pin':
                at = find(c, 'at')[0]
                num = uq(find(c, 'number')[0][1])
                name = uq(find(c, 'name')[0][1])
                ln = float(find(c, 'length')[0][1])
                pins[num] = (float(at[1]), float(at[2]), float(at[3]) if len(at) > 3 else 0.0, ln, name, c[1])
            elif c[0] == 'symbol' and re.search(r'_\d+_([2-9])"$', c[1]):
                continue
            else:
                walk(c)

    walk(tree)
    return pins


def format_tree(n, d=0):
    """indent a parsed tree the way KiCad 9 writes its files"""
    if not isinstance(n, list):
        return n
    if all(not isinstance(c, list) for c in n):
        return '(' + ' '.join(n) + ')'
    if n[0] == 'pts':
        return '(pts\n' + '\t' * (d + 1) + ' '.join(format_tree(c, d + 1) for c in n[1:]) + '\n' + '\t' * d + ')'
    s = '(' + ' '.join(c for c in n if not isinstance(c, list))
    for c in n:
        if isinstance(c, list):
            s += '\n' + '\t' * (d + 1) + format_tree(c, d + 1)
    return s + '\n' + '\t' * d + ')'


def format_text(text):
    return format_tree(parse(text)[0]) + '\n'


if __name__ == '__main__':
    import sys
    if len(sys.argv) != 2:
        sys.exit('usage: sexp.py <file>   re-indent a KiCad s-expression file in place')
    with open(sys.argv[1]) as f:
        t = f.read()
    with open(sys.argv[1], 'w') as f:
        f.write(format_text(t))
