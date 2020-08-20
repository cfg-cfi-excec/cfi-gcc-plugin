#include "gcc_plugin_fixer.h"

  GCC_PLUGIN_FIXER::GCC_PLUGIN_FIXER(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_FIXER::onFunctionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {
    // Load the policy matrix on entry of MAIN
    if (function_name.compare("main") == 0) {
      std::vector<CFG_FUNCTION_CALL> function_calls = getIndirectFunctionCalls();
      for(CFG_FUNCTION_CALL function_call : function_calls) {
        for(CFG_SYMBOL call : function_call.calls) {
          //printf("Function %s calls %s at %d\n", function_call.function_name.c_str(), call.function_name.c_str(), function_call.offset);
          std::string tmp = "CFI_MATLD_CALLER " + function_call.function_name + "+" + std::to_string(function_call.offset);  

          char *buff = new char[tmp.size()+1];
          std::copy(tmp.begin(), tmp.end(), buff);
          buff[tmp.size()] = '\0';

          emitAsmInput(buff, firstInsn, firstBlock, false);
          
          tmp = "CFI_MATLD_CALLEE " + call.symbol_name;  

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

  void GCC_PLUGIN_FIXER::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    // Don't instrument function entry of MAIN
    if (function_name.compare("main") != 0) {
      emitAsmInput("CFI_RET", lastInsn, lastBlock, false);
    }
  }

  void GCC_PLUGIN_FIXER::onFunctionExit(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    
  }

  void GCC_PLUGIN_FIXER::onDirectFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    emitAsmInput("CFI_CALL", insn, block, false);
  }

  void GCC_PLUGIN_FIXER::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    //CFI_CALL is explicit here, done automatically by CFI module
    //emitAsmInput("CFI_CALL", insn, block, false);

    rtx body = PATTERN(insn);
    rtx parallel = XVECEXP(body, 0, 0);
    rtx call;
    if (GET_CODE (parallel) == SET) {
      call = XEXP(parallel, 1);
    } else if (GET_CODE (parallel) == CALL) {
      call = parallel;
    }
    
    rtx mem = XEXP(call, 0);
    rtx subExpr = XEXP(mem, 0);

    if (((rtx_code)subExpr->code) == REG) {
      bool validReg = true;
      std::string regName = "";

      switch (REGNO(subExpr)) {
        case 18: regName = "s2"; break;
        case 19: regName = "s3"; break;
        case 20: regName = "s4"; break;
        case 21: regName = "s5"; break;
        case 22: regName = "s6"; break;
        case 23: regName = "s7"; break;
        case 24: regName = "s8"; break;
        case 25: regName = "s9"; break;
        case 26: regName = "s10"; break;
        case 27: regName = "s11"; break;
        default: validReg = false;
      };

      if (validReg) {        
        std::string tmp = "cfi_fwd " + regName;  

        char *buff = new char[tmp.size()+1];
        std::copy(tmp.begin(), tmp.end(), buff);
        buff[tmp.size()] = '\0';

        emitAsmInput(buff, insn, block, false);
        //printf("REGNO %d %d\n\n", REGNO (subExpr), ORIGINAL_REGNO (subExpr));
      } else {
        printf("ERROR reading register...\n");
        exit(1);
      }
    }
  }

  void GCC_PLUGIN_FIXER::onNamedLabel(std::string file_name, std::string function_name, std::string label_name, basic_block block, rtx_insn *insn) {

  }
  
  void GCC_PLUGIN_FIXER::onIndirectJump(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {

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