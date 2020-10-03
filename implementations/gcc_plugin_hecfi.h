#ifndef GCC_PLUGIN_HECFI_H_
#define GCC_PLUGIN_HECFI_H_

#include "../gcc_plugin.h"

class GCC_PLUGIN_HECFI : public GCC_PLUGIN {
	public:
		GCC_PLUGIN_HECFI(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter);

		GCC_PLUGIN_HECFI *clone();
		void onPluginFinished();
  		void emitTrampolines		(std::string file_name, std::string function_name, int line_number, std::string register_name, basic_block lastBlock, rtx_insn *lastInsn);
  
	protected:
		void onFunctionEntry		(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn);
		void onFunctionReturn		(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn);
		void onDirectFunctionCall	(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn);
		void onIndirectFunctionCall	(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn);
		void onRecursiveFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn);
		void onNamedLabel			(std::string file_name, std::string function_name, std::string label_name, basic_block block, rtx_insn *insn);
		void onIndirectJump			(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn);
		
	private:
		void init();
};

#endif /* GCC_PLUGIN_HECFI_H_ */