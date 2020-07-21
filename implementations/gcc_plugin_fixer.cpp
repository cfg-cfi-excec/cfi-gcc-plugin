#include "gcc_plugin_fixer.h"

struct CFG_FUNCTION;
struct CFG_EXISTING_FUNCTION;
struct CFG_FUNCTION_CALL;

  GCC_PLUGIN_FIXER::GCC_PLUGIN_FIXER(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_FIXER::onFunctionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {

  }
  
  void GCC_PLUGIN_FIXER::onFunctionRecursionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {
    onFunctionEntry(file_name, function_name, line_number, firstBlock, firstInsn);
  }

  void GCC_PLUGIN_FIXER::onFunctionReturn(const tree_node *tree, char *function_name, basic_block lastBlock, rtx_insn *lastInsn) {
  
  }

  void GCC_PLUGIN_FIXER::onFunctionExit(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) {
    
  }

  void GCC_PLUGIN_FIXER::onDirectFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
 
  }

  void GCC_PLUGIN_FIXER::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {

  }

  void GCC_PLUGIN_FIXER::onNamedLabel(const tree_node *tree, char *fName, const char *label_name, basic_block block, rtx_insn *insn) {

  }
  
  void GCC_PLUGIN_FIXER::onIndirectJump(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {

  }

  void GCC_PLUGIN_FIXER::onRecursiveFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
   
  }

  void GCC_PLUGIN_FIXER::onSetJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {

  }

  void GCC_PLUGIN_FIXER::onLongJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
 
  }

  void GCC_PLUGIN_FIXER::init()
  {
 
  }

  GCC_PLUGIN_FIXER *GCC_PLUGIN_FIXER::clone()
  {
    // We do not clone ourselves
    return this;
  }

	void GCC_PLUGIN_FIXER::onPluginFinished() {

  }