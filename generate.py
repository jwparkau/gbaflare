#!/usr/bin/env python3


generate_groups = (
    ("BRANCH_INSTR.gencpp", (("link", 2),), "arm_branch"),
    ("ALU_INSTR.gencpp", (("is_imm", 2), ("aluop", 16), ("set_cond", 2), ("shift_type", 4), ("shift_by_reg", 2)), "arm_alu")
)

dirname = "src/"
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

    x, n = l[0]
    for i in range(n):
        recurse(l[1:], p + [(x, i)])

for group in generate_groups:
    filename = dirname + group[0]
    functionname = group[2]
    f = open(filename, "w")
    print("if (0) {}", file=f)

    recurse(group[1], [])
