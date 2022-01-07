#!/usr/bin/env python3
import os

generate_groups = (
    ("BRANCH_INSTR.gencpp", (("link", 2),), "arm_branch"),
    ("ALU_INSTR.gencpp", (("is_imm", 2), ("aluop", 16), ("set_cond", 2), ("shift_type", 4), ("shift_by_reg", 2)), "arm_alu"),
    ("MUL_INSTR.gencpp", (("mulop", 0, (0, 1, 4, 5, 6, 7)), ("set_cond", 2)), "arm_mul"),
    ("PSR_INSTR.gencpp", (("psr", 2), ("dir", 2)), "arm_psr"),
    ("SDT_INSTR.gencpp", (("reg_offset", 2), ("prepost", 2), ("updown", 2), ("byteword", 2), ("writeback", 2), ("load", 2), ("shift_type", 4)), "arm_sdt"),
    ("SWP_INSTR.gencpp", (("byteword", 2),), "arm_swp"),
    ("MISC_DT_INSTR.gencpp", (("prepost", 2), ("updown", 2), ("imm_offset", 2), ("writeback", 2), ("load", 2), ("sign", 2), ("half", 2)), "arm_misc_dt"),
    ("BLOCK_DT_INSTR.gencpp", (("prepost", 2), ("updown", 2), ("psr", 2), ("writeback", 2), ("load", 2)), "arm_block_dt"),
    ("THUMB_BRANCH.gencpp", (("h", 0, (0, 2, 3)),), "thumb_branch"),
    ("THUMB_SHIFT_REG.gencpp", (("shift_type", 0, (0, 1, 2)),), "thumb_shift_reg"),
    ("THUMB_ADDSUB.gencpp", (("aluop", 4),), "thumb_addsub"),
    ("THUMB_ADDSUBCMPMOV.gencpp", (("aluop", 4),), "thumb_addsubcmpmov"),
    ("THUMB_ALU.gencpp", (("aluop", 16),), "thumb_alu"),
    ("THUMB_SPECIAL.gencpp", (("aluop", 0, (0, 1, 2)),), "thumb_special_data"),
    ("THUMB_LOADSTORE_REG.gencpp", (("code", 8),), "thumb_loadstore_reg"),
    ("THUMB_LOADSTORE_IMM.gencpp", (("code", 4),), "thumb_loadstore_imm"),
    ("THUMB_LOADSTORE_HALF.gencpp", (("code", 2),), "thumb_loadstore_half"),
    ("THUMB_LOADSTORE_SP.gencpp", (("code", 2),), "thumb_loadstore_sp"),
    ("THUMB_PUSHPOP.gencpp", (("code", 2), ("pclr", 2)), "thumb_pushpop"),
    ("THUMB_ADDPCSP.gencpp", (("code", 2),), "thumb_addpcsp"),
    ("THUMB_SPADD.gencpp", (("code", 2),), "thumb_sp_add"),
    ("THUMB_MULTIPLE.gencpp", (("code", 2),), "thumb_multiple"),
    ("ARM_MSR_IMM.gencpp", (("psr", 2),), "arm_msr_imm")
)

dirname = "src/gencpp/"
os.makedirs(os.path.dirname(dirname), exist_ok=True)
filename = ""
functionname = ""
f = ""

def process(p):
    print("else if (", end="", file=f)
    for i in range(len(p)):
        x, n = p[i]
        print("%s == %d" % (x, n), end="", file=f)
        if i != len(p) - 1:
            print(" && ", end="", file=f)
    print(") {return %s<" % functionname, end="", file=f)
    print(",".join(str(x[1]) for x in p), end="", file=f)
    print(">;}", file=f)

def recurse(l, p):
    if len(l) == 0:

        process(p)

        return

    x, n, *q = l[0]
    if (n > 0):
        for i in range(n):
            recurse(l[1:], p + [(x, i)])
    else:
        for i in q[0]:
            recurse(l[1:], p + [(x, i)])

for group in generate_groups:
    filename = dirname + group[0]
    functionname = group[2]
    f = open(filename, "w")
    print("if (0) {}", file=f)

    recurse(group[1], [])
