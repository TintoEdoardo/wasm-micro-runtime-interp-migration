/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_INTERP_H
#define _WASM_INTERP_H

#include "wasm.h"

#ifdef __cplusplus
extern "C" {
#endif

struct WASMModuleInstance;
struct WASMFunctionInstance;
struct WASMExecEnv;

#if WASM_ENABLE_MIGRATING_INTERP != 0
struct WASMExecEnvCheckpoint ;
#endif

typedef struct WASMInterpFrame {
    /* The frame of the caller that are calling the current function. */
    struct WASMInterpFrame *prev_frame;

    /* The current WASM function. */
    struct WASMFunctionInstance *function;

    /* Instruction pointer of the bytecode array.  */
    uint8 *ip;

#if WASM_ENABLE_FAST_JIT != 0
    uint8 *jitted_return_addr;
#endif

#if WASM_ENABLE_PERF_PROFILING != 0
    uint64 time_started;
#endif

#if WASM_ENABLE_EXCE_HANDLING != 0
    /* set to true if the callee returns an exception rather than
     * result values on the stack
     */
    bool exception_raised;
    uint32 tag_index;
#endif

#if WASM_ENABLE_FAST_INTERP != 0
    /* Return offset of the first return value of current frame,
       the callee will put return values here continuously */
    uint32 ret_offset;
    uint32 *lp;
#if WASM_ENABLE_GC != 0
    uint8 *frame_ref;
#endif
    uint32 operand[1];
#else  /* else of WASM_ENABLE_FAST_INTERP != 0 */
    /* Operand stack top pointer of the current frame. The bottom of
       the stack is the next cell after the last local variable. */
    uint32 *sp_bottom;
    uint32 *sp_boundary;
    uint32 *sp;

    WASMBranchBlock *csp_bottom;
    WASMBranchBlock *csp_boundary;
    WASMBranchBlock *csp;

    /**
     * Frame data, the layout is:
     *  lp: parameters and local variables
     *  sp_bottom to sp_boundary: wasm operand stack
     *  csp_bottom to csp_boundary: wasm label stack
     *  frame ref flags: only available for GC
     *    whether each cell in local and stack area is a GC obj
     *  jit spill cache: only available for fast jit
     */
    uint32 lp[1];
#endif /* end of WASM_ENABLE_FAST_INTERP != 0 */
} WASMInterpFrame;

#if WASM_ENABLE_MIGRATING_INTERP != 0
typedef struct WASMInterpFrameCheckpoint {
    struct WASMInterpFrameCheckpoint *next_frame;

    /* TODO: REMOVE
    char *module_name;
     */
    uint32 func_index;
    uint32  size;

    /**
     * Frame data, the layout is:
     *  lp: parameters and local variables
     *  ip_offset: instruction pointer (as offset)
     *  sp_offset: operand stack top pointer (as offset)
     *  csp_bottom: base of the checkpointed csp stack
     *  csp_boundary: top of the checkpointed csp stack
     *  csp_offset: wasm label stack pointer (as offset)
     */

    uint32 *lp;
    uint32 ip_offset;
    uint32 sp_offset;
    WASMBranchBlockCheckpoint *csp_bottom;
    WASMBranchBlockCheckpoint *csp_boundary;
    uint32 csp_offset;
} WASMInterpFrameCheckpoint;
#endif

/**
 * Calculate the size of interpreter area of frame of a function.
 *
 * @param all_cell_num number of all cells including local variables
 * and the working stack slots
 *
 * @return the size of interpreter area of the frame
 */
static inline unsigned
wasm_interp_interp_frame_size(unsigned all_cell_num)
{
    unsigned frame_size;

#if WASM_ENABLE_FAST_INTERP == 0
#if WASM_ENABLE_GC == 0
    frame_size = (uint32)offsetof(WASMInterpFrame, lp) + all_cell_num * 4;
#else
    frame_size =
        (uint32)offsetof(WASMInterpFrame, lp) + align_uint(all_cell_num * 5, 4);
#endif
#else
    frame_size = (uint32)offsetof(WASMInterpFrame, operand) + all_cell_num * 4;
#endif
    return align_uint(frame_size, 4);
}

void
wasm_interp_call_wasm(struct WASMModuleInstance *module_inst,
                      struct WASMExecEnv *exec_env,
                      struct WASMFunctionInstance *function, uint32 argc,
                      uint32 argv[]);

/* TODO: REMOVE
struct WASMExecEnvCheckpoint *
wasm_interp_produce_checkpoint(struct WASMExecEnv *exec_env);

void
wasm_interp_restore_exec_env(struct WASMModuleInstance *module_inst,
                             struct WASMExecEnv *exec_env,
                             struct WASMExecEnvCheckpoint *exec_env_checkpoint);

void
wasm_interp_resume_wasm(struct WASMModuleInstance *module_inst,
                        uint32 argv[]);
*/

#if WASM_ENABLE_MIGRATING_INTERP != 0
void
wasm_copy_checkpoint(struct WASMExecEnvCheckpoint *dst, struct WASMExecEnvCheckpoint *src, uint32 stack_size);
#endif

#if WASM_ENABLE_GC != 0
bool
wasm_interp_traverse_gc_rootset(struct WASMExecEnv *exec_env, void *heap);

uint8 *
wasm_interp_get_frame_ref(WASMInterpFrame *frame);
#endif

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_INTERP_H */
