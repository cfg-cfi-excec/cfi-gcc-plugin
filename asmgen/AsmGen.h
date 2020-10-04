/*
 * This GCC Plugin has been developed during a research grant from the Baekeland program of the Flemish Agency for Innovation and Entrepreneurship (VLAIO) in cooperation with Televic Healthcare NV, under grant agreement IWT 150696.
 * Copyright (c) 2019 Jens Vankeirsbilck & KU Leuven LRD & Televic Healthcare NV.
 * Distributed under the MIT "Expat" License. (See accompanying file LICENSE.txt)
 */

/*
 * This file contains the prototypes of the methods used to insert extra
 * code into the code of the program that is to be protected.
 *
 * The functions are named after the assembly instruction they insert.
 *
 * Code for POP and PUSH instructions is located in the according <targetISA>_Functions.cpp
 * file in the Targets folder.
 */

#ifndef ASM_ASMGEN_H_
#define ASM_ASMGEN_H_

#include <gcc-plugin.h>
#include <basic-block.h>
#include <rtl.h>
#include <context.h>
#include <df.h>
#include <tree-pass.h>
#include <tree.h>
#include <print-tree.h>

class AsmGen{
	public:
		static rtx_insn* emitCodeLabel(unsigned int insnID, rtx_insn* attachRtx, basic_block bb, bool after);
		static rtx_insn* emitAsmInput(const char* asmInstr, rtx_insn* attachRtx, basic_block bb, bool after);

	private:
		static rtx_insn* emitInsn(rtx rtxInsn,rtx_insn* attachRtx, basic_block bb, bool after);
		static rtx_insn* emitLabel(rtx label, rtx_insn* attachRtx, bool after);
};



#endif /* ASM_ASMGEN_H_ */
