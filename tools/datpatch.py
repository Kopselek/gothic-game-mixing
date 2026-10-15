"""Patch compiled Daedalus scripts (Gothic.dat) without the script sources.

Gothic.dat lives in _work\\DATA\\scripts\\_compiled\\. Always keep the original next to it
(Gothic.dat.orig) and patch from that copy, so every run starts from the same input and patches
can be re-applied. Changes take effect for a new game; instances already in a savegame (items,
NPCs) keep the values they were created with.

How an instance's field assignments look in bytecode (e.g. `name = "Sword";`):
    push_v  <value symbol>     (string literal / constant / variable)   or push_i <int>
    push_v  C_ITEM.NAME        (class member)                           or push_vv C_ITEM.TEXT [index]
    assigns / assign / assignf / assignfunc
String literals are symbols named "\\xff<number>"; change their content to change the text. A
literal can be shared by several pushes: check `script.references(index)` before editing it, or
it changes everywhere. Gothic 1 Polish/German texts are cp1250/latin-1.
"""
import os
import shutil
import struct

from daedalus import OP_PUSH_I, OP_PUSH_V, OP_PUSH_VV, Script


def load(gothic_dir):
    """The original Gothic.dat (backed up as Gothic.dat.orig on first use) and the path to write."""
    path = os.path.join(gothic_dir, "_work", "DATA", "scripts", "_compiled", "Gothic.dat")
    if not os.path.exists(path + ".orig"):
        shutil.copy2(path, path + ".orig")
    return Script(open(path + ".orig", "rb").read()), path


def save(script, path):
    with open(path, "wb") as f:
        f.write(script.to_bytes())


def assignments(script, instance, prefix=b"C_ITEM."):
    """[(member name, array index, value pc, value opcode, value operand)] of an instance's
    field assignments (prefix: the class, C_ITEM., C_NPC., ...)."""
    start = struct.unpack_from("<I", script.symbols[script.by_name[instance.upper()]].content)[0]
    out, previous = [], None
    for pc, op, arg in script.instructions(start):
        if op in (OP_PUSH_V, OP_PUSH_VV) and previous and (script.symbols[arg].name or b"").startswith(prefix):
            out.append((script.symbols[arg].name.decode(), script.code[pc + 5] if op == OP_PUSH_VV else 0) + previous)
            previous = None
        else:
            previous = (pc, op, arg)
    return out


class Patcher:
    """Edits on one script; call apply() at the end."""

    def __init__(self, script):
        self.script, self.code = script, bytearray(script.code)

    def operand(self, pc, value):
        """Replace the operand of the instruction at pc (push_i value, or push_v symbol index)."""
        self.code[pc + 1:pc + 5] = struct.pack("<i", value)

    def literal(self, symbol_index, text, uses=1, encoding="cp1250"):
        """Set a string literal's text (refuses if more pushes than `uses` share it)."""
        assert self.script.references(symbol_index) <= uses, "string literal is shared with other code"
        self.script.symbols[symbol_index].content = [text.encode(encoding)]

    def set_field(self, instance, member, value, index=0, prefix=b"C_ITEM."):
        """Set an existing assignment: int -> push_i operand, str -> the pushed literal, a symbol
        name like "DAM_BLUNT" or "MAT_METAL" -> push_v of that constant. Returns how many were set."""
        sym = self.script.by_name
        done = 0
        for name, idx, pc, op, arg in assignments(self.script, instance, prefix):
            if name.upper() != member.upper() or idx != index:
                continue
            if isinstance(value, int) and op == OP_PUSH_I:
                self.operand(pc, value)
            elif isinstance(value, str) and value.upper() in sym and op == OP_PUSH_V:
                self.operand(pc, sym[value.upper()])
            elif isinstance(value, str) and op == OP_PUSH_V:
                self.literal(arg, value)
            else:
                continue
            done += 1
        return done

    def apply(self):
        self.script.code = bytes(self.code)


def set_hero_visual(script, model_mds, body_mesh, head_mesh=""):
    """Point the hero's Mdl_SetVisual / Mdl_SetVisualBody calls (instance PC_HERO) at another
    model, e.g. ("MYHERO.MDS", "MYHERO_BODY", "")."""
    model = script.string_args_of_call("PC_HERO", "MDL_SETVISUAL")[0][-1]
    (body, head), = script.string_args_of_call("PC_HERO", "MDL_SETVISUALBODY")
    for index, value in ((model, model_mds), (body, body_mesh), (head, head_mesh)):
        assert script.references(index) == 1, "string literal is shared with other code"
        script.symbols[index].content = [value.encode()]


if __name__ == "__main__":
    # example: rename an item and make it blunt
    import sys
    script, path = load(sys.argv[1] if len(sys.argv) > 1 else r"C:\Program Files (x86)\Steam\steamapps\common\Gothic")
    for row in assignments(script, "ITMW_1H_SWORD_01"):
        print(row)
