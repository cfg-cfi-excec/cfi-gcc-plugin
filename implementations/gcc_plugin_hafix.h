#ifndef GCC_PLUGIN_HAFIX_H_
#define GCC_PLUGIN_HAFIX_H_

#include "../gcc_plugin.h"

class GCC_PLUGIN_HAFIX : public GCC_PLUGIN {
	public:
		GCC_PLUGIN_HAFIX(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter);

		GCC_PLUGIN_HAFIX *clone();
		void onPluginFinished();
		
	protected:

		void instrumentFunctionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn);
		void instrumentFunctionReturn(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn);
		void instrumentFunctionExit(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn);
		void instrumentDirectFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn);
		void instrumentIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn);
		void instrumentSetJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn);
		void instrumentLongJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn);

	private:
		void init();
		void clearTmpFile();
		void writeLabelToTmpFile(unsigned label);
		unsigned getLabelFromTmpFile();
};

#endif /* GCC_PLUGIN_HAFIX_H_ */
