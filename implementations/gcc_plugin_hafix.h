#ifndef GCC_PLUGIN_HAFIX_H_
#define GCC_PLUGIN_HAFIX_H_

#include "gcc_plugin.h"

class GCC_PLUGIN_HAFIX : public GCC_PLUGIN {
	public:
		GCC_PLUGIN_HAFIX(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter);

		GCC_PLUGIN_HAFIX *clone();

	protected:
		void onFunctionEntry			(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn);
		void onFunctionReturn			(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn);
		void onFunctionRecursionEntry	(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn);
		void onDirectFunctionCall		(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn);
		void onIndirectFunctionCall		(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn);
		void onRecursiveFunctionCall	(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn);
		
};

#endif /* GCC_PLUGIN_HAFIX_H_ */
