#ifndef GCC_PLUGIN_HCFI_H_
#define GCC_PLUGIN_HCFI_H_

#include "../gcc_plugin.h"

class GCC_PLUGIN_HCFI : public GCC_PLUGIN {
	public:
		GCC_PLUGIN_HCFI(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter);

		GCC_PLUGIN_HCFI *clone();
		void onPluginFinished();

	protected:

		void onFunctionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn);
		void onFunctionRecursionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn);
		void onFunctionReturn(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn);
		void onFunctionExit(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn);
		void onDirectFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn);
		void onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn);
		void onSetJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn);
		void onLongJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn);
		void onRecursiveFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn);
		void onNamedLabel(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn);
		void onIndirectJump(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn);
		
	private:
		std::vector<CFG_EXISTING_FUNCTION> existing_functions;
		std::vector<CFG_FUNCTION_CALL> function_calls;

		void init();
		void read_cfg_file(char * file_name);
		void print_existing_functions();
		void print_function_call();
		int get_label_for_existing_function(std::string function_name, std::string file_name);
		int get_label_for_function_call(std::string function_name, std::string file_name, int line_number);
};

#endif /* GCC_PLUGIN_HCFI_H_ */
