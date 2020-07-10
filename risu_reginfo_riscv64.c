/******************************************************************************
 * Copyright (c) 2020 T-Head Semiconductor Co., Ltd.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     LIU Zhiwei (T-Head) - initial implementation
 *     based on Peter Maydell's risu_arm.c
 *****************************************************************************/

#include <stdio.h>
#include <ucontext.h>
#include <string.h>
#include <signal.h> /* for FPSIMD_MAGIC */
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <assert.h>
#include <sys/prctl.h>

#include "risu.h"
#include "risu_reginfo_riscv64.h"

const struct option * const arch_long_opts;
const char * const arch_extra_help;

void process_arch_opt(int opt, const char *arg)
{
    abort();
}

const int reginfo_size(void)
{
    return sizeof(struct reginfo);
}

/* reginfo_init: initialize with a ucontext */
void reginfo_init(struct reginfo *ri, ucontext_t *uc)
{
    int i;
    union __riscv_mc_fp_state *fp;
    /* necessary to be able to compare with memcmp later */
    memset(ri, 0, sizeof(*ri));

    for (i = 0; i < 32; i++) {
        ri->regs[i] = uc->uc_mcontext.__gregs[i];
    }

    ri->regs[2] = 0xdeadbeefdeadbeef;
    ri->regs[3] = 0xdeadbeefdeadbeef;
    ri->regs[4] = 0xdeadbeefdeadbeef;
    ri->pc = uc->uc_mcontext.__gregs[0] - image_start_address;
    ri->regs[0] = ri->pc;
    ri->faulting_insn = *((uint32_t *) uc->uc_mcontext.__gregs[0]);
    fp = &uc->uc_mcontext.__fpregs;
#if __riscv_flen == 64
    ri->fcsr = fp->__d.__fcsr;

    for (i = 0; i < 32; i++) {
        ri->fregs[i] = fp->__d.__f[i];
    }
#else
# error "Unsupported fp length"
#endif
}

/* reginfo_is_eq: compare the reginfo structs, returns nonzero if equal */
int reginfo_is_eq(struct reginfo *r1, struct reginfo *r2)
{
    return memcmp(r1, r2, reginfo_size()) == 0;
}

/* reginfo_dump: print state to a stream, returns nonzero on success */
int reginfo_dump(struct reginfo *ri, FILE * f)
{
    int i;
    fprintf(f, "  faulting insn %08x\n", ri->faulting_insn);

    for (i = 1; i < 32; i++) {
        fprintf(f, "  X%-2d    : %016" PRIx64 "\n", i, ri->regs[i]);
    }

    fprintf(f, "  pc     : %016" PRIx64 "\n", ri->pc);
    fprintf(f, "  fcsr   : %08x\n", ri->fcsr);

    for (i = 0; i < 32; i++) {
        fprintf(f, "  F%-2d    : %016" PRIx64 "\n", i, ri->fregs[i]);
    }

    return !ferror(f);
}

/* reginfo_dump_mismatch: print mismatch details to a stream, ret nonzero=ok */
int reginfo_dump_mismatch(struct reginfo *m, struct reginfo *a, FILE * f)
{
    int i;
    fprintf(f, "mismatch detail (master : apprentice):\n");
    if (m->faulting_insn != a->faulting_insn) {
        fprintf(f, "  faulting insn mismatch %08x vs %08x\n",
                m->faulting_insn, a->faulting_insn);
    }
    for (i = 1; i < 32; i++) {
        if (m->regs[i] != a->regs[i]) {
            fprintf(f, "  X%-2d    : %016" PRIx64 " vs %016" PRIx64 "\n",
                    i, m->regs[i], a->regs[i]);
        }
    }

    if (m->pc != a->pc) {
        fprintf(f, "  pc     : %016" PRIx64 " vs %016" PRIx64 "\n",
                m->pc, a->pc);
    }

    if (m->fcsr != a->fcsr) {
        fprintf(f, "  fcsr   : %08x vs %08x\n", m->fcsr, a->fcsr);
    }

    for (i = 0; i < 32; i++) {
        if (m->fregs[i] != a->fregs[i]) {
            fprintf(f, "  F%-2d    : "
                    "%016" PRIx64 " vs "
                    "%016" PRIx64 "\n", i,
                    (uint64_t) m->fregs[i],
                    (uint64_t) a->fregs[i]);
        }
    }

    return !ferror(f);
}
