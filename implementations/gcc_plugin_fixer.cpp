#include "gcc_plugin_fixer.h"

struct CFG_FUNCTION;
struct CFG_EXISTING_FUNCTION;
struct CFG_FUNCTION_CALL;

  GCC_PLUGIN_FIXER::GCC_PLUGIN_FIXER(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_FIXER::onFunctionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {
    // Load the policy matrix on entry of MAIN
    if (function_name.compare("main") == 0) {
      std::vector<CFG_EXISTING_FUNCTION> existing_functions = getExistingFunctions();
      for(CFG_EXISTING_FUNCTION existing_function : existing_functions) {
        for(CFG_FUNCTION called_by : existing_function.called_by) {
          //printf("Function %s is called by %s \n", existing_function.function_name.c_str(), called_by.function_name.c_str());
          std::string tmp = "CFI_MATLD_CALLER " + called_by.function_name;  

          char *buff = new char[tmp.size()+1];
          std::copy(tmp.begin(), tmp.end(), buff);
          buff[tmp.size()] = '\0';

          emitAsmInput(buff, firstInsn, firstBlock, false);
          
          tmp = "CFI_MATLD_CALLEE " + existing_function.function_name;  

          buff = new char[tmp.size()+1];
          std::copy(tmp.begin(), tmp.end(), buff);
          buff[tmp.size()] = '\0';

          emitAsmInput(buff, firstInsn, firstBlock, false);
        }
      }

    }
  }
  
  void GCC_PLUGIN_FIXER::onFunctionRecursionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {
    onFunctionEntry(file_name, function_name, line_number, firstBlock, firstInsn);
  }

  void GCC_PLUGIN_FIXER::onFunctionReturn(const tree_node *tree, char *function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    // Don't instrument function entry of MAIN
    if (strcmp(function_name, "main") != 0) {
      emitAsmInput("CFI_RET", lastInsn, lastBlock, false);
    }
  }

  void GCC_PLUGIN_FIXER::onFunctionExit(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) {
    
  }

  void GCC_PLUGIN_FIXER::onDirectFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    emitAsmInput("CFI_CALL", insn, block, false);
  }

  void GCC_PLUGIN_FIXER::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    emitAsmInput("CFI_CALL", insn, block, false);

    std::string tmp = "cfi_fwd_caller " + function_name;  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, insn, block, false);
    
    //TODO: set fp of called function here
    tmp = "cfi_fwd_callee " + function_name;  

    buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, insn, block, false);
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

  void GCC_PLUGIN_FIXER::init() {
    for (int i = 0; i < argc; i++) {
      if (std::strcmp(argv[i].key, "cfg_file") == 0) {
        std::cout << "CFG file for instrumentation: " << argv[i].value << "\n";

        readConfigFile(argv[i].value);
        //prinExistingFunctions();
        //printFunctionCalls();
      }
    }
  }

  GCC_PLUGIN_FIXER *GCC_PLUGIN_FIXER::clone()
  {
    // We do not clone ourselves
    return this;
  }

	void GCC_PLUGIN_FIXER::onPluginFinished() {

  }