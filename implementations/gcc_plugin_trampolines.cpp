#include "gcc_plugin_trampolines.h"

  GCC_PLUGIN_TRAMPOLINES::GCC_PLUGIN_TRAMPOLINES(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_TRAMPOLINES::onFunctionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {
    unsigned label = 123;

    std::string tmp = "CFIBR " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, firstInsn, firstBlock, false);
  }

  void GCC_PLUGIN_TRAMPOLINES::onFunctionRecursionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {
    unsigned label = 123;

    std::string tmp = "CFIREC " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, firstInsn, firstBlock, false);
  }

  void GCC_PLUGIN_TRAMPOLINES::onFunctionReturn(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) {
    unsigned label = 123;
    std::string tmp = "CFIDEL " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, lastInsn, lastBlock, false);
  }

  void GCC_PLUGIN_TRAMPOLINES::onFunctionExit(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) {

  }

  void GCC_PLUGIN_TRAMPOLINES::onDirectFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    unsigned label = 123;
    std::string tmp = "CFIRET " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    rtx_insn *tmpInsn = NEXT_INSN(insn);
    while (NOTE_P(tmpInsn)) {
      tmpInsn = NEXT_INSN(tmpInsn);
    }

    emitAsmInput(buff, tmpInsn, block, false);
  }

  void GCC_PLUGIN_TRAMPOLINES::onRecursiveFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    onDirectFunctionCall(tree, fName, block, insn);
  }

  void GCC_PLUGIN_TRAMPOLINES::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    unsigned label = 123;
    std::string tmp = "CFIRET " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    rtx_insn *tmpInsn = NEXT_INSN(insn);
    while (NOTE_P(tmpInsn)) {
      tmpInsn = NEXT_INSN(tmpInsn);
    }

    emitAsmInput(buff, tmpInsn, block, false);
  }

  void GCC_PLUGIN_TRAMPOLINES::onSetJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    // Do nothing...
  }

  void GCC_PLUGIN_TRAMPOLINES::onLongJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    // Do nothing...
  }

  void GCC_PLUGIN_TRAMPOLINES::init() {

  }

  GCC_PLUGIN_TRAMPOLINES *GCC_PLUGIN_TRAMPOLINES::clone() {
    // We do not clone ourselves
    return this;
  }
