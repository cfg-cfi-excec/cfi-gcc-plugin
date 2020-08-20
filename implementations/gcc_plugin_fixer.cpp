#include "gcc_plugin_fixer.h"

  GCC_PLUGIN_FIXER::GCC_PLUGIN_FIXER(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_FIXER::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
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
  
  void GCC_PLUGIN_FIXER::onFunctionRecursionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    onFunctionEntry(file_name, function_name, firstBlock, firstInsn);
  }

  void GCC_PLUGIN_FIXER::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    // Don't instrument function entry of MAIN
    if (function_name.compare("main") != 0) {
      emitAsmInput("CFI_RET", lastInsn, lastBlock, false);
    }
  }

  void GCC_PLUGIN_FIXER::onDirectFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
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
      std::string regName = getRegisterNameForNumber(REGNO(subExpr));

      std::string tmp = "cfi_fwd " + regName;  
      char *buff = new char[tmp.size()+1];
      std::copy(tmp.begin(), tmp.end(), buff);
      buff[tmp.size()] = '\0';
      emitAsmInput(buff, insn, block, false);
    }
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