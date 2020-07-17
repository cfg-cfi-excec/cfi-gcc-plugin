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
#include <rtl.h>
#include <tree-pass.h>
#include <tree.h>
#include <print-tree.h>



		const struct pass_data my_first_pass_data = {
				.type = RTL_PASS,
				.name = "my_first_pass",
				.optinfo_flags = OPTGROUP_NONE,
				.tv_id = TV_TREE_CLEANUP_CFG,
				.properties_required = 0,//(PROP_rtl | PROP_cfglayout),
				.properties_provided = 0,
				.properties_destroyed = 0,
				.todo_flags_start = 0,
				.todo_flags_finish = 0,
		};
		
class GCC_PLUGIN : public rtl_opt_pass{
	public:
		GCC_PLUGIN(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter);

		unsigned int execute(function *fun);
		virtual GCC_PLUGIN *clone() = 0;

	protected:
		int argc;
		struct plugin_argument *argv;
		virtual void instrumentFunctionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) = 0;
		virtual void instrumentFunctionReturn(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) = 0;
		virtual void instrumentFunctionExit(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) = 0;
		virtual void instrumentDirectFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) = 0;
		virtual void instrumentIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn)  = 0;
		virtual void instrumentSetJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) = 0;
		virtual void instrumentLongJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) = 0;

		rtx_insn* emitInsn(rtx rtxInsn,rtx_insn* attachRtx, basic_block bb, bool after);
		rtx_insn* emitAsmInput(const char* asmInstr, rtx_insn* attachRtx, basic_block bb, bool after);

	private:
		rtx_insn* firstRealINSN(basic_block bb);
		rtx_insn* lastRealINSN(basic_block bb);
		rtx createConstInt(int number);
	 	bool findCode(rtx expr, rtx_code code);
		bool isCall(rtx_insn* expr);
		bool isReturn(rtx_insn* expr);
		void printRtxClass(rtx_code code);

};

#endif /* GCC_PLUGIN_H_ */
