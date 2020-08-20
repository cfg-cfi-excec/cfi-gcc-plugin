#ifndef GCC_PLUGIN_H_
#define GCC_PLUGIN_H_

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <vector>

// This is the first gcc header to be included
#include "gcc-plugin.h"
#include <context.h>
#include <basic-block.h>
#include <df.h>
#include <rtl.h>
#include <tree-pass.h>
#include <tree.h>
#include <print-tree.h>
//#include <stringpool.h>

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
};

struct CFG_FUNCTION_CALL {
    std::string file_name;
    std::string function_name;
    int line_number;
    int offset;
    int label;
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
		virtual void onPluginFinished() = 0;

	protected:
		int argc;
		struct plugin_argument *argv;
		virtual void onFunctionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) = 0;
		virtual void onFunctionRecursionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) = 0;
		virtual void onFunctionReturn(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) = 0;
		virtual void onFunctionExit(std::string file_name, char *fName, basic_block lastBlock, rtx_insn *lastInsn) = 0;
		virtual void onDirectFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) = 0;
		virtual void onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn)  = 0;
		virtual void onSetJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) = 0;
		virtual void onLongJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) = 0;
		virtual void onRecursiveFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) = 0;
		virtual void onNamedLabel(std::string file_name, std::string function_name, std::string label_name, basic_block block, rtx_insn *insn) = 0;
		virtual void onIndirectJump(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) = 0;

		rtx_insn* emitInsn(rtx rtxInsn,rtx_insn* attachRtx, basic_block bb, bool after);
		rtx_insn* emitAsmInput(const char* asmInstr, rtx_insn* attachRtx, basic_block bb, bool after);
		rtx_insn* emitCodeLabel(unsigned int insnID, rtx_insn* attachRtx, basic_block bb, bool after);
		rtx_insn* emitLabel(rtx label, rtx_insn* attachRtx, bool after);
		rtx_insn* generateAndEmitAsm(std::string insn, rtx_insn* attachRtx, basic_block bb, bool after);
		rtx_insn* firstRealINSN(basic_block bb);
		rtx_insn* lastRealINSN(basic_block bb);
		basic_block lastRealBlockInFunction();
		rtx createConstInt(int number);
	 	bool findCode(rtx expr, rtx_code code);
		bool isCall(rtx_insn* expr);
		bool isReturn(rtx_insn* expr);
		void printRtxClass(rtx_code code);
		std::string getRegisterNameForNumber(unsigned regno);
		
		void clearTmpFile();
		void writeLabelToTmpFile(unsigned label);
		unsigned readLabelFromTmpFile();
		void readConfigFile(char * file_name);
		void printFunctionCalls();
		void printLabelJumps();
		std::vector<CFG_FUNCTION_CALL> getIndirectFunctionCalls();
		std::vector<CFG_LABEL_JUMP> getIndirectLabelJumps();
		int getLabelForIndirectlyCalledFunction(std::string function_name, std::string file_name);
 		int getLabelForIndirectFunctionCall(std::string function_name, std::string file_name, int line_number);
		int getLabelForIndirectJumpSymbol(std::string file_name, std::string function_name, std::string symbol_name);
		int getLabelForIndirectJump(std::string file_name, std::string function_name);

	private:
		std::vector<CFG_FUNCTION_CALL> function_calls;
		std::vector<CFG_LABEL_JUMP> label_jumps;

};

#endif /* GCC_PLUGIN_H_ */
