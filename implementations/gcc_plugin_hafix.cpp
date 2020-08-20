#include "gcc_plugin_hafix.h"

  GCC_PLUGIN_HAFIX::GCC_PLUGIN_HAFIX(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {}

  void GCC_PLUGIN_HAFIX::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    writeLabelToTmpFile(readLabelFromTmpFile()+1);
    unsigned label = readLabelFromTmpFile();

    std::string tmp = "CFIBR " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, firstInsn, firstBlock, false);
  }

  void GCC_PLUGIN_HAFIX::onFunctionRecursionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    unsigned label = readLabelFromTmpFile();

    std::string tmp = "CFIREC " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, firstInsn, firstBlock, false);
  }

  void GCC_PLUGIN_HAFIX::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    unsigned label = readLabelFromTmpFile();
    std::string tmp = "CFIDEL " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, lastInsn, lastBlock, false);
  }

  void GCC_PLUGIN_HAFIX::onDirectFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    unsigned label = readLabelFromTmpFile();
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

  void GCC_PLUGIN_HAFIX::onRecursiveFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    onDirectFunctionCall(file_name, function_name, block, insn);
  }

  void GCC_PLUGIN_HAFIX::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    unsigned label = readLabelFromTmpFile();
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

  GCC_PLUGIN_HAFIX *GCC_PLUGIN_HAFIX::clone() {
    // We do not clone ourselves
    return this;
  }
  
	void GCC_PLUGIN_HAFIX::onPluginFinished() {
    remove("tmp.txt");
  }
