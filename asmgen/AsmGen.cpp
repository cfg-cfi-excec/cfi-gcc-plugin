/*
 * This GCC Plugin has been developed during a research grant from the Baekeland program of the Flemish Agency for Innovation and Entrepreneurship (VLAIO) in cooperation with Televic Healthcare NV, under grant agreement IWT 150696.
 * Copyright (c) 2019 Jens Vankeirsbilck & KU Leuven LRD & Televic Healthcare NV.
 * Distributed under the MIT "Expat" License. (See accompanying file LICENSE.txt)
 */

#include "AsmGen.h"

/**
 * Emits: .codeLabel
 */
rtx_insn* AsmGen::emitCodeLabel(unsigned int insnID, rtx_insn* attachRtx, basic_block bb, bool after){
	rtx codeLab = gen_label_rtx();
	rtx_insn* insn = emitLabel(codeLab, attachRtx, after);
	return insn;
}

/**
 * Emits the provided assembly instruction
 */
rtx_insn* AsmGen::emitAsmInput(const char* asmInstr, rtx_insn* attachRtx, basic_block bb, bool after){
	rtx asmBxLR = gen_rtx_ASM_INPUT_loc(VOIDmode, asmInstr, 1);
	asmBxLR->volatil=1;
	rtx memRTX = gen_rtx_MEM(BLKmode, gen_rtx_SCRATCH(VOIDmode));
	rtx clobber = gen_rtx_CLOBBER(VOIDmode, memRTX);
	rtvec vec = rtvec_alloc(2);
	vec->elem[0] = asmBxLR;
	vec->elem[1] = clobber;
	rtx par = gen_rtx_PARALLEL(VOIDmode, vec);
	rtx_insn* insn = emitInsn(par, attachRtx, bb, after);
	return insn;
}

/*
 * Actually emits the insn at the desired place.
 */
rtx_insn* AsmGen::emitInsn(rtx rtxInsn,rtx_insn* attachRtx, basic_block bb, bool after){
	if (after){
		return emit_insn_after_noloc(rtxInsn, attachRtx, bb);
	}
	else{
		return emit_insn_before_noloc(rtxInsn, attachRtx, bb);
	}
}

/*
 * Actually emits the codelabel at the desired place
 */
rtx_insn* AsmGen::emitLabel(rtx label, rtx_insn* attachRtx, bool after){
	if(after){
		return emit_label_after(label, attachRtx);
	}
	else{
		return emit_label_before(label, attachRtx);
	}
}
