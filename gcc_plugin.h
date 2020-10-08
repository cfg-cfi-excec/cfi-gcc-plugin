#ifndef GCC_PLUGIN_H_
#define GCC_PLUGIN_H_

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <vector>
#include <algorithm>

// This is the first gcc header to be included
#include "gcc-plugin.h"
#include <context.h>
#include <basic-block.h>
#include <df.h>
#include <rtl.h>
#include <tree-pass.h>
#include <tree.h>
#include <print-tree.h>
#include "asmgen/AsmGen.h"
#include "asmgen/InstrType.h"
#include "asmgen/UpdatePoint.h"

const struct pass_data cfi_plugin_pass_data = {
		.type = RTL_PASS,
		.name = "cfi_pass",
		.optinfo_flags = OPTGROUP_NONE,
		.tv_id = TV_TREE_CLEANUP_CFG,
		.properties_required = 0,//(PROP_rtl | PROP_cfglayout),
		.properties_provided = 0,
		.properties_destroyed = 0,
		.todo_flags_start = 0,
		.todo_flags_finish = 0,
};

struct CFG_SYMBOL {
    std::string file_name;
    std::string symbol_name;
	int label;
};

struct CFG_FUNCTION_CALL {
    std::string file_name;
    std::string function_name;
    int line_number;
    int label;
    int offset;
    std::vector<CFG_SYMBOL> calls;
};

struct CFG_LABEL_JUMP {
    std::string file_name;
    std::string function_name;
    int label;
    std::vector<CFG_SYMBOL> jumps_to;
};

class GCC_PLUGIN : public rtl_opt_pass{
	public:
		GCC_PLUGIN(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter);

		unsigned int execute(function *fun);
		virtual GCC_PLUGIN *clone() = 0;
		virtual void onPluginFinished() {}

	protected:
		int argc;
		struct plugin_argument *argv;

		// These functions are optionally overwritten in derived classes
		virtual void onFunctionEntry			(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {}
		virtual void onFunctionRecursionEntry	(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {}
		virtual void onFunctionReturn			(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {}
		virtual void onFunctionExit				(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {}
		virtual void onDirectFunctionCall		(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {}
		virtual void onIndirectFunctionCall		(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {}
		virtual void onSetJumpFunctionCall		(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {}
		virtual void onLongJumpFunctionCall		(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {}
		virtual void onRecursiveFunctionCall	(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {}
		virtual void onNamedLabel				(std::string file_name, std::string function_name, std::string label_name, basic_block block, rtx_insn *insn) {}
		virtual void onIndirectJump				(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {}
 
		rtx_insn* generateAndEmitAsm(std::string insn, rtx_insn* attachRtx, basic_block bb, bool after);
		basic_block lastRealBlockInFunction();
		std::string getRegisterNameForNumber(unsigned regno);
		
		void clearTmpFile();
		void writeLabelToTmpFile(unsigned label);
		unsigned readLabelFromTmpFile();
		void readConfigFile(char * file_name);
		void printFunctionCalls();
		void printLabelJumps();
		void printIndirectlyCalledFunctions();
		std::vector<CFG_FUNCTION_CALL> getIndirectFunctionCalls();
		std::vector<CFG_LABEL_JUMP> getIndirectLabelJumps();
		int getLabelForIndirectlyCalledFunction(std::string function_name, std::string file_name);
 		int getLabelForIndirectFunctionCall(std::string function_name, std::string file_name, int line_number);
		int getLabelForIndirectJumpSymbol(std::string file_name, std::string function_name, std::string symbol_name);
		int getLabelForIndirectJump(std::string file_name, std::string function_name);
		bool isFunctionUsedInMultipleIndirectCalls(std::string file_name, std::string function_name);
		bool areTrampolinesNeeded(std::string file_name, std::string function_name, int line_number);
		bool isFunctionExcludedFromCFI(std::string function_name);

	private:
		std::vector<CFG_FUNCTION_CALL> function_calls;
		std::vector<CFG_LABEL_JUMP> label_jumps;
		std::vector<CFG_SYMBOL> indirectly_called_functions;

};

#endif /* GCC_PLUGIN_H_ */
