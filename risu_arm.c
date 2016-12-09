/*******************************************************************************
 * Copyright (c) 2010 Linaro Limited
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Peter Maydell (Linaro) - initial implementation
 *******************************************************************************/

#include <stdio.h>
#include <ucontext.h>
#include <string.h>

#include "risu.h"
#include "risu_reginfo_arm.h"

struct reginfo master_ri, apprentice_ri;
uint8_t apprentice_memblock[MEMBLOCKLEN];

static int mem_used = 0;
static int packet_mismatch = 0;

int insnsize(ucontext_t *uc)
{
   /* Return instruction size in bytes of the
    * instruction at PC
    */
   if (uc->uc_mcontext.arm_cpsr & 0x20) 
   {
      uint16_t faulting_insn = *((uint16_t*)uc->uc_mcontext.arm_pc);
      switch (faulting_insn & 0xF800)
      {
         case 0xE800:
         case 0xF000:
         case 0xF800:
            /* 32 bit Thumb2 instruction */
            return 4;
         default:
            /* 16 bit Thumb instruction */
            return 2;
      }
   }
   /* ARM instruction */
   return 4;
}

void advance_pc(void *vuc)
{
   ucontext_t *uc = vuc;
   uc->uc_mcontext.arm_pc += insnsize(uc);
   if (ismaster) {
      report_test_status((void *) uc->uc_mcontext.arm_pc);
   }
}

static void set_r0(void *vuc, uint32_t r0)
{
   ucontext_t *uc = vuc;
   uc->uc_mcontext.arm_r0 = r0;
}

static int get_risuop(uint32_t insn, int isz)
{
   /* Return the risuop we have been asked to do
    * (or -1 if this was a SIGILL for a non-risuop insn)
    */
   uint32_t op = insn & 0xf;
   uint32_t key = insn & ~0xf;
   uint32_t risukey = (isz == 2) ? 0xdee0 : 0xe7fe5af0;
   return (key != risukey) ? -1 : op;
}


int send_register_info(write_fn write_fn, void *uc)
{
   struct reginfo ri;
   trace_header_t header;
   int op, r = 0;

   reginfo_init(&ri, uc);
   op = get_risuop(ri.faulting_insn, ri.faulting_insn_size);

   /* Write a header with PC/op to keep in sync */
   header.pc = ri.gpreg[15];
   header.risu_op = op;
   if (write_fn(&header, sizeof(header)) != 0) {
      fprintf(stderr,"%s: failed header write\n", __func__);
      return -1;
   }

   switch (op)
   {
      case OP_TESTEND:
         if (write_fn(&ri, sizeof(ri)) != 0) {
            fprintf(stderr,"%s: failed last write\n", __func__);
         }
         r = 1;
         break;
      case OP_SETMEMBLOCK:
         memblock = (void *)ri.gpreg[0];
         break;
      case OP_GETMEMBLOCK:
         set_r0(uc, ri.gpreg[0] + (uintptr_t)memblock);
         break;
      case OP_COMPAREMEM:
         r = write_fn(memblock, MEMBLOCKLEN);
         break;
      case OP_COMPARE:
      default:
         /* Do a simple register compare on (a) explicit request
          * (b) end of test (c) a non-risuop UNDEF
          */
         r = write_fn(&ri, sizeof(ri));
         break;
   }

   return r;
}

/* Read register info from the socket and compare it with that from the
 * ucontext. Return 0 for match, 1 for end-of-test, 2 for mismatch.
 * NB: called from a signal handler.
 *
 * We don't have any kind of identifying info in the incoming data
 * that says whether it's register or memory data, so if the two
 * sides get out of sync then we will fail obscurely.
 */
int recv_and_compare_register_info(read_fn read_fn, respond_fn resp_fn, void *uc)
{
   int resp = 0, op;
   trace_header_t header;

   reginfo_init(&master_ri, uc);
   op = get_risuop(master_ri.faulting_insn, master_ri.faulting_insn_size);

   if (read_fn(&header, sizeof(header)) != 0) {
      fprintf(stderr,"%s: failed header read\n", __func__);
      return -1;
   }

   if ( header.pc == master_ri.gpreg[15] &&
        header.risu_op == op ) {

      /* send OK for the header */
      resp_fn(0);

      switch (op)
      {
         case OP_COMPARE:
         case OP_TESTEND:
         default:
            /* Do a simple register compare on (a) explicit request
             * (b) end of test (c) a non-risuop UNDEF
             */
            if (read_fn(&apprentice_ri, sizeof(apprentice_ri)))
            {
               packet_mismatch = 1;
               resp = 2;
            }
            else if (memcmp(&master_ri, &apprentice_ri, sizeof(master_ri)) != 0)
            {
               /* register mismatch */
               resp = 2;
            }
            else if (op == OP_TESTEND)
            {
               resp = 1;
            }
            resp_fn(resp);
            break;
         case OP_SETMEMBLOCK:
            memblock = (void *)master_ri.gpreg[0];
            break;
         case OP_GETMEMBLOCK:
            set_r0(uc, master_ri.gpreg[0] + (uintptr_t)memblock);
            break;
         case OP_COMPAREMEM:
            mem_used = 1;
            if (read_fn(apprentice_memblock, MEMBLOCKLEN))
            {
               packet_mismatch = 1;
               resp = 2;
            }
            else if (memcmp(memblock, apprentice_memblock, MEMBLOCKLEN) != 0)
            {
               /* memory mismatch */
               resp = 2;
            }
            resp_fn(resp);
            break;
      }
   } else {
      fprintf(stderr, "out of sync %x/%x %d/%d\n",
              master_ri.gpreg[15], header.pc,
              op, header.risu_op);
      resp = 2;
      resp_fn(resp);
   }

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
   int resp = 0;
   fprintf(stderr, "match status...\n");
   if (packet_mismatch)
   {
      fprintf(stderr, "packet mismatch (probably disagreement "
              "about UNDEF on load/store)\n");
      /* We don't have valid reginfo from the apprentice side
       * so stop now rather than printing anything about it.
       */
      fprintf(stderr, "master reginfo:\n");
      reginfo_dump(&master_ri, stderr);
      return 1;
   }
   if (!reginfo_is_eq(&master_ri, &apprentice_ri))
   {
      fprintf(stderr, "mismatch on regs!\n");
      resp = 1;
   }
   if (mem_used && memcmp(memblock, &apprentice_memblock, MEMBLOCKLEN) != 0)
   {
      fprintf(stderr, "mismatch on memory!\n");
      resp = 1;
   }
   if (!resp)
   {
      fprintf(stderr, "match!\n");
      return 0;
   }

   fprintf(stderr, "master reginfo:\n");
   reginfo_dump(&master_ri, stderr);
   fprintf(stderr, "apprentice reginfo:\n");
   reginfo_dump(&apprentice_ri, stderr);

   reginfo_dump_mismatch(&master_ri, &apprentice_ri, stderr);
   return resp;
}
