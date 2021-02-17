#ifndef GCC_PLUGIN_ICET_H_
#define GCC_PLUGIN_ICET_H_

#include "gcc_plugin.h"

class GCC_PLUGIN_ICET : public GCC_PLUGIN {
	public:
		GCC_PLUGIN_ICET(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter);

		GCC_PLUGIN_ICET *clone();

	protected:
		void onFunctionEntry			(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn);
		void onFunctionRecursionEntry	(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn);
		void onFunctionReturn			(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn);
		void onNamedLabel				(std::string file_name, std::string function_name, std::string label_name, basic_block block, rtx_insn *insn);
		void onSwitchCase				(int label, basic_block block, rtx_insn *insn);

	private:
		const char *ENDBRANCH = "CFIENDBRANCH";
		void init();
};

#endif /* GCC_PLUGIN_ICET_H_ */
