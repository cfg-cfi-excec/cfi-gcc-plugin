#ifndef GCC_PLUGIN_EXCEC_H_
#define GCC_PLUGIN_EXCEC_H_

#include "gcc_plugin.h"

class GCC_PLUGIN_EXCEC : public GCC_PLUGIN {
	public:
		GCC_PLUGIN_EXCEC(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter);

		GCC_PLUGIN_EXCEC *clone();

	protected:
		void onFunctionEntry			(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn);
		void onFunctionRecursionEntry	(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn);
		void onFunctionReturn			(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn);
		void onIndirectFunctionCall		(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn);
		void onNamedLabel				(std::string file_name, std::string function_name, std::string label_name, basic_block block, rtx_insn *insn);
		void onIndirectJump				(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn);
		void onSwitchCase				(int label, basic_block block, rtx_insn *insn);
		int  onIndirectJumpWithJumpTable(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn);
		void onSetJumpFunctionCall		(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn, int index);
		void onLongJumpFunctionCall		(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn, int index);
		
	private:
		void emitTrampolines(std::string file_name, std::string function_name, int line_number, std::string register_name, basic_block lastBlock, rtx_insn *lastInsn);
		void init();
};

#endif /* GCC_PLUGIN_EXCEC_H_ */
