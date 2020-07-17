#include "gcc_plugin_hafix.h"

  GCC_PLUGIN_HAFIX::GCC_PLUGIN_HAFIX(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_HAFIX::instrumentFunctionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {
    label++;

    std::string tmp = "CFIBR " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, firstInsn, firstBlock, false);
  }

  void GCC_PLUGIN_HAFIX::instrumentFunctionReturn(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) {
    std::string tmp = "CFIDEL " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, lastInsn, lastBlock, false);
  }

  void GCC_PLUGIN_HAFIX::instrumentFunctionExit(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) {

  }

  void GCC_PLUGIN_HAFIX::instrumentDirectFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {

  }

  void GCC_PLUGIN_HAFIX::instrumentIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    
  }

  void GCC_PLUGIN_HAFIX::instrumentSetJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    // Do nothing...
  }

  void GCC_PLUGIN_HAFIX::instrumentLongJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    // Do nothing...
  }

  void GCC_PLUGIN_HAFIX::init() {

  }

  GCC_PLUGIN_HAFIX *GCC_PLUGIN_HAFIX::clone() {
    // We do not clone ourselves
    return this;
  }
