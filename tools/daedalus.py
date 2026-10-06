"""Read, patch and write compiled Daedalus scripts (Gothic.dat).

Layout: uint8 version, uint32 count, uint32 sort_table[count], count symbols, uint32
code_size, bytecode. A symbol is: uint32 named, [name '\\n'], uint32 offset, uint32 props
(count:12 type:4 flags:6 ...), 5 x uint32 debug info, content (unless it is a class
member), int32 parent. Content: floats/ints/'\\n'-terminated strings for `count`
elements, or one uint32 for classes, functions, prototypes and instances.
"""
import struct

T_FLOAT, T_INT, T_STRING, T_CLASS, T_FUNC, T_PROTOTYPE, T_INSTANCE = 1, 2, 3, 4, 5, 6, 7
F_CLASSVAR, F_EXTERNAL = 4, 8

# opcodes that carry a 4-byte operand (push_vv carries 4 + 1)
OP_RSR, OP_BL, OP_BE, OP_PUSH_I, OP_PUSH_V, OP_PUSH_VI, OP_B, OP_BZ, OP_GMOVI, OP_PUSH_VV = 60, 61, 62, 64, 65, 67, 75, 76, 80, 245
WIDE = {OP_BL, OP_BE, OP_PUSH_I, OP_PUSH_V, OP_PUSH_VI, OP_B, OP_BZ, OP_GMOVI}


class Symbol:
    __slots__ = ("named", "name", "head", "props", "content", "parent")

    @property
    def type(self):
        return (self.props >> 12) & 0xF

    @property
    def count(self):
        return self.props & 0xFFF

    @property
    def flags(self):
        return (self.props >> 16) & 0x3F


class Script:
    def __init__(self, data):
        self.version = data[0]
        count = struct.unpack_from("<I", data, 1)[0]
        pos = 5
        self.sort_table = data[pos:pos + count * 4]
        pos += count * 4
        self.symbols = []
        for _ in range(count):
            s = Symbol()
            s.named = struct.unpack_from("<I", data, pos)[0]
            pos += 4
            s.name = None
            if s.named:
                end = data.index(b"\n", pos)
                s.name = data[pos:end]
                pos = end + 1
            s.head = data[pos:pos + 28]  # offset, props, 5 x debug info
            s.props = struct.unpack_from("<I", s.head, 4)[0]
            pos += 28
            if s.flags & F_CLASSVAR:
                s.content = None
            elif s.type == T_STRING:
                s.content = []
                for _ in range(s.count):
                    end = data.index(b"\n", pos)
                    s.content.append(data[pos:end])
                    pos = end + 1
            elif s.type in (T_FLOAT, T_INT):
                s.content = data[pos:pos + 4 * s.count]
                pos += 4 * s.count
            elif s.type in (T_CLASS, T_FUNC, T_PROTOTYPE, T_INSTANCE):
                s.content = data[pos:pos + 4]
                pos += 4
            else:
                s.content = b""
            s.parent = data[pos:pos + 4]
            pos += 4
            self.symbols.append(s)
        size = struct.unpack_from("<I", data, pos)[0]
        self.code = data[pos + 4:pos + 4 + size]
        self.by_name = {s.name.decode("latin-1").upper(): i for i, s in enumerate(self.symbols) if s.name}

    def to_bytes(self):
        out = [bytes([self.version]), struct.pack("<I", len(self.symbols)), self.sort_table]
        for s in self.symbols:
            out.append(struct.pack("<I", s.named))
            if s.named:
                out.append(s.name + b"\n")
            out.append(s.head)
            if isinstance(s.content, list):
                out += [v + b"\n" for v in s.content]
            elif s.content is not None:
                out.append(s.content)
            out.append(s.parent)
        out += [struct.pack("<I", len(self.code)), self.code]
        return b"".join(out)

    def instructions(self, start):
        """Yield (address, opcode, operand) from `start` up to and including the next rsr."""
        pc = start
        while True:
            op = self.code[pc]
            if op in WIDE:
                arg = struct.unpack_from("<i", self.code, pc + 1)[0]
                yield pc, op, arg
                pc += 5
            elif op == OP_PUSH_VV:
                yield pc, op, struct.unpack_from("<I", self.code, pc + 1)[0]
                pc += 6
            else:
                yield pc, op, None
                pc += 1
                if op == OP_RSR:
                    return

    def string_args_of_call(self, instance, function):
        """Symbol indices of the string literals pushed for each call to `function` inside `instance`."""
        start = struct.unpack_from("<I", self.symbols[self.by_name[instance.upper()]].content)[0]
        target = self.by_name[function.upper()]
        calls, pushed = [], []
        for _, op, arg in self.instructions(start):
            if op == OP_PUSH_V and self.symbols[arg].type == T_STRING:
                pushed.append(arg)
            elif op in (OP_BE, OP_BL):
                if op == OP_BE and arg == target:
                    calls.append(pushed)
                pushed = []
        return calls

    def int_value(self, name):
        """The value of an integer constant."""
        return struct.unpack_from("<i", self.symbols[self.by_name[name.upper()]].content)[0]

    def references(self, symbol_index):
        """Number of push_v instructions in the whole script that push `symbol_index`."""
        count, pc = 0, 0
        while pc < len(self.code):
            op = self.code[pc]
            if op in WIDE:
                if op == OP_PUSH_V and struct.unpack_from("<i", self.code, pc + 1)[0] == symbol_index:
                    count += 1
                pc += 5
            else:
                pc += 6 if op == OP_PUSH_VV else 1
        return count
