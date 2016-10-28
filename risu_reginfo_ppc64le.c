/******************************************************************************
 * Copyright (c) 2013 Linaro Limited
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Jose Ricardo Ziviani - initial implementation
 *     based on Claudio Fontana's risu_aarch64.c
 *     based on Peter Maydell's risu_arm.c
 *****************************************************************************/

#include <stdio.h>
#include <ucontext.h>
#include <string.h>
#include <math.h>

#include "risu.h"
#include "risu_reginfo_ppc64le.h"

#define XER 37
#define CCR 38

/* reginfo_init: initialize with a ucontext */
void reginfo_init(struct reginfo *ri, ucontext_t *uc)
{
    ri->faulting_insn = *((uint32_t *)uc->uc_mcontext.regs->nip);
    ri->prev_insn = *((uint32_t *)(uc->uc_mcontext.regs->nip - 4));
    ri->prev_addr = uc->uc_mcontext.regs->nip - 4;

    int i;
    for (i = 0; i < NGREG; i++)
    {
        ri->gregs[i] = uc->uc_mcontext.gp_regs[i];
    }

    for (i = 0; i < NFPREG; i++)
    {
        ri->fpregs[i] = uc->uc_mcontext.fp_regs[i];
    }

    for (i = 0; i < 32; i++)
    {
        ri->vrregs.vrregs[i][0] = uc->uc_mcontext.v_regs->vrregs[i][0];
        ri->vrregs.vrregs[i][1] = uc->uc_mcontext.v_regs->vrregs[i][1];
        ri->vrregs.vrregs[i][2] = uc->uc_mcontext.v_regs->vrregs[i][2];
        ri->vrregs.vrregs[i][3] = uc->uc_mcontext.v_regs->vrregs[i][3];
    }
    ri->vrregs.vscr = uc->uc_mcontext.v_regs->vscr;
    ri->vrregs.vrsave = uc->uc_mcontext.v_regs->vrsave;
}

/* reginfo_is_eq: compare the reginfo structs, returns nonzero if equal */
int reginfo_is_eq(struct reginfo *master, struct reginfo *apprentice, ucontext_t *uc)
{
    int i;
    for (i = 0; i < 32; i++)
    {
        if (i == 1 || i == 13)
            continue;

        if (master->gregs[i] != apprentice->gregs[i])
        {
            fprintf(stderr, "\e[1;32mMismatch: \e[0mRegister r%d\n", i);
            fprintf(stderr, "master: [%lx] - apprentice: [%lx]\n",
                    master->gregs[i], apprentice->gregs[i]);
            return 0;
        }
    }

    if (master->gregs[XER] != apprentice->gregs[XER])
    {
        fprintf(stderr, "\e[1;32mMismatch: \e[0mXER\n");
        fprintf(stderr, "master: [%lx] != apprentice: [%lx]\n",
                    master->gregs[XER], apprentice->gregs[XER]);
        return 0;
    }

    if ((master->gregs[CCR] & 0x10) != (apprentice->gregs[CCR] & 0x10))
    {
        fprintf(stderr, "\e[1;32mMismatch: \e[0mCond. Register\n");
        fprintf(stderr, "master: [%lx] != apprentice: [%lx]\n",
                master->gregs[CCR], apprentice->gregs[CCR]);
        return 0;
    }

    for (i = 0; i < 32; i++)
    {
        if (isnan(master->fpregs[i]) && isnan(apprentice->fpregs[i]))
            continue;

        if  (master->fpregs[i] != apprentice->fpregs[i])
        {
            fprintf(stderr, "\e[1;32mMismatch: \e[0mRegister r%d\n", i);
            fprintf(stderr, "master: [%f] != apprentice: [%f]\n",
                    master->fpregs[i], apprentice->fpregs[i]);
            return 0;
        }
    }

    for (i = 0; i < 32; i++)
    {
        if (master->vrregs.vrregs[i][0] != apprentice->vrregs.vrregs[i][0] ||
                master->vrregs.vrregs[i][1] != apprentice->vrregs.vrregs[i][1] ||
                master->vrregs.vrregs[i][2] != apprentice->vrregs.vrregs[i][2] ||
                master->vrregs.vrregs[i][3] != apprentice->vrregs.vrregs[i][3])
        {
            if (uc != NULL && (master->gregs[CCR] & 0x10)) {
                uc->uc_mcontext.v_regs->vrregs[i][0] = apprentice->vrregs.vrregs[i][0];
                uc->uc_mcontext.v_regs->vrregs[i][1] = apprentice->vrregs.vrregs[i][1];
                uc->uc_mcontext.v_regs->vrregs[i][2] = apprentice->vrregs.vrregs[i][2];
                uc->uc_mcontext.v_regs->vrregs[i][3] = apprentice->vrregs.vrregs[i][3];
                return 1;
            }

            fprintf(stderr, "\e[1;32mMismatch: \e[0mRegister vr%d\n", i);
            fprintf(stderr, "master: [%x, %x, %x, %x] != apprentice: [%x, %x, %x, %x]\n",
                    master->vrregs.vrregs[i][0], master->vrregs.vrregs[i][1],
                    master->vrregs.vrregs[i][2], master->vrregs.vrregs[i][3],
                    apprentice->vrregs.vrregs[i][0], apprentice->vrregs.vrregs[i][1],
                    apprentice->vrregs.vrregs[i][2], apprentice->vrregs.vrregs[i][3]);

            return 0;
        }
    }

    return 1;
}

/* reginfo_dump: print state to a stream, returns nonzero on success */
void reginfo_dump(struct reginfo *ri, int is_master)
{
    int i;
    if (is_master) {
        fprintf(stderr, "  faulting insn \e[1;101;37m0x%x\e[0m\n", ri->faulting_insn);
        fprintf(stderr, "  prev insn     \e[1;101;37m0x%x\e[0m\n", ri->prev_insn);
        fprintf(stderr, "  prev addr     \e[1;101;37m0x%" PRIx64 "\e[0m\n\n", ri->prev_addr);
    }

    for (i = 0; i < 16; i++)
    {
        fprintf(stderr, "\tr%2d: %16lx\tr%2d: %16lx\n", i, ri->gregs[i],
                i + 16, ri->gregs[i + 16]);
    }

    fprintf(stderr, "\n");
    fprintf(stderr, "\tnip    : %16lx\n", ri->gregs[32]);
    fprintf(stderr, "\tmsr    : %16lx\n", ri->gregs[33]);
    fprintf(stderr, "\torig r3: %16lx\n", ri->gregs[34]);
    fprintf(stderr, "\tctr    : %16lx\n", ri->gregs[35]);
    fprintf(stderr, "\tlnk    : %16lx\n", ri->gregs[36]);
    fprintf(stderr, "\txer    : %16lx\n", ri->gregs[37]);
    fprintf(stderr, "\tccr    : %16lx\n", ri->gregs[38]);
    fprintf(stderr, "\tmq     : %16lx\n", ri->gregs[39]);
    fprintf(stderr, "\ttrap   : %16lx\n", ri->gregs[40]);
    fprintf(stderr, "\tdar    : %16lx\n", ri->gregs[41]);
    fprintf(stderr, "\tdsisr  : %16lx\n", ri->gregs[42]);
    fprintf(stderr, "\tresult : %16lx\n", ri->gregs[43]);
    fprintf(stderr, "\tdscr   : %16lx\n\n", ri->gregs[44]);

    for (i = 0; i < 16; i++)
    {
        fprintf(stderr, "\tf%2d: %.4f\tr%2d: %.4f\n", i, ri->fpregs[i],
                i + 16, ri->fpregs[i + 16]);
    }
    fprintf(stderr, "\tfpscr: %f\n\n", ri->fpregs[32]);

    for (i = 0; i < 32; i++)
    {
        fprintf(stderr, "vr%02d: %8x, %8x, %8x, %8x\n", i,
                ri->vrregs.vrregs[i][0], ri->vrregs.vrregs[i][1],
                ri->vrregs.vrregs[i][2], ri->vrregs.vrregs[i][3]);
    }
}
