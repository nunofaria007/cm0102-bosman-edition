"""Native versions of hot loops of the original, put in place of the loop by the recompiler.

Each entry of INSNS (see widescreen.py) replaces the translation of the first instruction of a loop with a
call of a C function of src/recomp_rt/fastloops.h that does the same work with the same results. The run
counts of the blocks (recomp.py --count-blocks) show which loops are worth it; tests/ checks each one
against the instructions of the original."""

INSNS = {
    # Player initialisation 0x535a80: the distance of an attribute vector to a template, 0x537777..0x537812.
    0x537777: ("xor eax, eax",
               "rt_player_init_distance(esp, ebp, &eax, &ecx, &edx, &esi, &edi); goto L_537812;"),
}
