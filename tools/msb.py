"""Read a compiled model script (.MSB) back into model script text (.MDS).

Gothic 2 ships its model scripts compiled only (_COMPILED\\HUMANS.MSB in Anims.vdf and
Anims_Addon.vdf), where Gothic 1 ships the HUMANS.MDS source. The tools that give the hero's
model Gothic's animations read the MDS text, so this turns an MSB into the same syntax.

An MSB is a flat run of chunks, each a uint16 type and a uint32 length (layout as ZenKit's
ModelScript.cc reads it, checked against Gothic 2 NotR's HUMANS.MSB): strings end with "\\n",
numbers are little-endian int32 / float32. An animation's event chunks follow it between
0xF5A0 (event count) and 0xF5AF. A few fields ZenKit reads as integers are floats in the game's
files: the particle effect index and the sound and ATTACH flags; the sound flag is the opposite
of EMPTY_SLOT.

  python msb.py <file.MSB> [out.MDS]
"""
import argparse
import struct

ROOT, SOURCE, MODEL, MODEL_END = 0xF000, 0xF100, 0xF200, 0xF2FF
MESH_AND_TREE, REGISTER_MESH, ANI_ENUM, ANI_ENUM_END = 0xF300, 0xF400, 0xF500, 0xF5FF
ANI, ALIAS, BLEND, COMB, DISABLE, MODEL_TAG = 0xF520, 0xF530, 0xF540, 0xF570, 0xF580, 0xF590
EVENTS, EVENTS_END = 0xF5A0, 0xF5AF
SFX, SFX_GRND, TAG, PFX, PFX_STOP, MM_ANI, CAM_TREMOR = 0xF5A1, 0xF5A2, 0xF5A3, 0xF5A4, 0xF5A5, 0xF5A9, 0xF5AA
END = 0xFFFF


class _Chunk:
    def __init__(self, data):
        self.data, self.pos = data, 0

    def string(self):
        end = self.data.index(b"\n", self.pos)
        text = self.data[self.pos:end].decode("latin-1")
        self.pos = end + 1
        return text

    def int(self):
        self.pos += 4
        return struct.unpack_from("<i", self.data, self.pos - 4)[0]

    def float(self):
        self.pos += 4
        return struct.unpack_from("<f", self.data, self.pos - 4)[0]

    def rest(self):
        """The strings left in the chunk."""
        out = []
        while self.pos < len(self.data):
            out.append(self.string())
        return out


def chunks(data):
    pos = 0
    while pos + 6 <= len(data):
        kind, size = struct.unpack_from("<HI", data, pos)
        yield kind, _Chunk(data[pos + 6:pos + 6 + size])
        pos += 6 + size


def _num(x):
    return f"{round(x, 4):g}"


def _q(s):
    return f'"{s}"'


def to_mds(data, name="MODEL"):
    """The model script of MSB `data` as MDS text: meshAndTree, registerMesh, modelTag and the
    aniEnum (ani, aniAlias, aniBlend, aniComb, aniDisable with their event blocks)."""
    head, anis = [], []
    events = None  # the event lines of the animation they follow

    def close_events():
        nonlocal events
        if events is not None:
            anis.append("\t\t{\n" + "".join(f"\t\t\t{e}\n" for e in events) + "\t\t}\n")
            events = None

    for kind, c in chunks(data):
        if kind == MESH_AND_TREE:
            dont_use = c.int()
            head.append(f'\tmeshAndTree ({_q(c.string() + ".ASC")}{" DONT_USE_MESH" if dont_use else ""})\n')
        elif kind == REGISTER_MESH:
            head.append(f"\tregisterMesh ({_q(c.string())})\n")
        elif kind == MODEL_TAG:
            tag, *args = [s for s in c.rest() if s]
            head.append(f"\tmodelTag ({' '.join(_q(s) for s in (tag, *args))})\n")
        elif kind == ANI:
            close_events()
            ani, layer, nxt = c.string(), c.int(), c.string()
            blend_in, blend_out, flags, asc, direction = c.float(), c.float(), c.string(), c.string(), c.string()
            first, last, fps, _speed, cvs = c.int(), c.int(), c.float(), c.float(), c.float()
            options = (f" FPS:{_num(fps)}" if abs(fps - 25) > 1e-3 else "") + (f" CVS:{_num(cvs)}" if abs(cvs - 1) > 1e-3 else "")
            anis.append(f"\t\tani\t\t({_q(ani)}\t{layer}\t{_q(nxt)}\t{_num(blend_in)}\t{_num(blend_out)}\t{flags}"
                        f"\t{_q(asc)}\t{direction}\t{first}\t{last}{options})\n")
        elif kind == ALIAS:
            close_events()
            ani, layer, nxt = c.string(), c.int(), c.string()
            blend_in, blend_out, flags, alias, direction = c.float(), c.float(), c.string(), c.string(), c.string()
            anis.append(f"\t\taniAlias\t({_q(ani)}\t{layer}\t{_q(nxt)}\t{_num(blend_in)}\t{_num(blend_out)}\t{flags}"
                        f"\t{_q(alias)}\t{direction})\n")
        elif kind == BLEND:
            close_events()
            ani, nxt, blend_in, blend_out = c.string(), c.string(), c.float(), c.float()
            anis.append(f"\t\taniBlend\t({_q(ani)}\t{_q(nxt)}\t{_num(blend_in)}\t{_num(blend_out)})\n")
        elif kind == COMB:
            close_events()
            ani, layer, nxt = c.string(), c.int(), c.string()
            blend_in, blend_out, flags, prefix, count = c.float(), c.float(), c.string(), c.string(), c.int()
            anis.append(f"\t\taniComb\t\t({_q(ani)}\t{layer}\t{_q(nxt)}\t{_num(blend_in)}\t{_num(blend_out)}\t{flags}"
                        f"\t{_q(prefix)}\t{count})\n")
        elif kind == DISABLE:
            close_events()
            anis.append(f"\t\taniDisable\t({_q(c.string())})\n")
        elif kind == EVENTS:
            events = []
        elif kind == EVENTS_END:
            close_events()
        elif kind == TAG and events is not None:
            frame, tag = c.int(), c.string()
            args = c.rest()
            while args and not args[-1]:
                args.pop()
            if tag == "DEF_FIGHTMODE" and not args:
                args = [""]  # Gothic's own scripts write the empty mode out
            events.append(f"*eventTag\t({frame}\t{' '.join(_q(s) for s in (tag, *args))})")
        elif kind in (SFX, SFX_GRND) and events is not None:
            # the last float is 1 for a sound in the model's slot and 0 for EMPTY_SLOT (Gothic 1's
            # HUMANS.MDS has M_FALL_SMALL with EMPTY_SLOT, Walk_Water without; the MSB 0 and 1)
            frame, sound, radius, in_slot = c.int(), c.string(), c.float(), c.float()
            word, default_radius = ("*eventSFX", 2500) if kind == SFX else ("*eventSFXGrnd", 1000)
            events.append(f"{word}\t({frame}\t{_q(sound)}{'' if radius == default_radius else ' R:' + _num(radius)}"
                          f"{'' if in_slot else ' EMPTY_SLOT'})")
        elif kind == PFX and events is not None:
            frame, index, effect, node, attach = c.int(), c.float(), c.string(), c.string(), c.float()
            events.append(f"*eventPFX\t({frame}\t{int(index)}\t{_q(effect)}\t{_q(node)}{' ATTACH' if attach else ''})")
        elif kind == PFX_STOP and events is not None:
            frame, index = c.int(), c.float()
            events.append(f"*eventPFXStop\t({frame}\t{int(index)})")
        elif kind == MM_ANI and events is not None:
            frame, ani, node = c.int(), c.string(), c.string()
            events.append(f"*eventMMStartAni\t({frame}\t{_q(ani)}{' ' + _q(node) if node else ''})")
        elif kind == CAM_TREMOR and events is not None:
            frame = c.int()
            events.append(f"*eventCamTremor\t({frame}\t{c.int()}\t{c.int()}\t{c.int()}\t{c.int()})")
    close_events()
    return f'Model ({_q(name)})\n{{\n{"".join(head)}\n\taniEnum\n\t{{\n{"".join(anis)}\t}}\n}}\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("msb")
    parser.add_argument("out", nargs="?", help="write the MDS here (default: print it)")
    args = parser.parse_args()
    text = to_mds(open(args.msb, "rb").read())
    if args.out:
        open(args.out, "w", encoding="latin-1", newline="\r\n").write(text)
    else:
        print(text)


if __name__ == "__main__":
    main()
