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

#include "risu.h"
#include "risu_reginfo_ppc64le.h"

struct reginfo master_ri, apprentice_ri;

void advance_pc(void *vuc)
{
    ucontext_t *uc = (ucontext_t*)vuc;
    uc->uc_mcontext.regs->nip += 4;
}

int send_register_info(int sock, void *uc)
{
    struct reginfo ri;
    reginfo_init(&ri, uc);
    return send_data_pkt(sock, &ri, sizeof(ri));
}

/* Read register info from the socket and compare it with that from the
 * ucontext. Return 0 for match, 1 for end-of-test, 2 for mismatch.
 * NB: called from a signal handler.
 */
int recv_and_compare_register_info(int sock, void *uc)
{
    int resp = 0;
    reginfo_init(&master_ri, uc);
    if (recv_data_pkt(sock, &apprentice_ri, sizeof(apprentice_ri))) {
        printf("Packed mismatch\n");
        resp = 2;

    } else if (!reginfo_is_eq(&master_ri, &apprentice_ri, uc))
    {
        /* mismatch */
        resp = 2;
    }
    else if (master_ri.faulting_insn == 0x5af1)
    {
        /* end of test */
        resp = 1;
    }
    else
    {
        /* either successful match or expected undef */
        resp = 0;
    }
    send_response_byte(sock, resp);

    return resp;
}

/* Print a useful report on the status of the last comparison
 * done in recv_and_compare_register_info(). This is called on
 * exit, so need not restrict itself to signal-safe functions.
 * Should return 0 if it was a good match (ie end of test)
 * and 1 for a mismatch.
 */
int report_match_status(void)
{
    fprintf(stderr, "match status...\n");
    int resp = reginfo_is_eq(&master_ri, &apprentice_ri, NULL);
    if (resp) {
        fprintf(stderr, "\e[1;34mTest passed successfuly\e[0m\n");
        return 0;
    }

    fprintf(stderr, "\n******************** [master reginfo]\n");
    reginfo_dump(&master_ri, 1);

    fprintf(stderr, "\n******************* [apprentice reginfo]\n");
    reginfo_dump(&apprentice_ri, 0);

    fprintf(stderr, "\e[1;31mmismatch!\e[0m\n");

    return resp;
}
