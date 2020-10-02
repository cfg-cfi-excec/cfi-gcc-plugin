#include "gcc_plugin_fixer.h"

  GCC_PLUGIN_FIXER::GCC_PLUGIN_FIXER(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_FIXER::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    if (strcmp(function_name.c_str(), "_main") == 0) {
      // enable CFI from here on
      //TODO: replace CFI_DBG6 with some sort of CFI_ENABLE instruction
      generateAndEmitAsm("CFI_DBG6 t0", firstInsn, firstBlock, false);

      // Load the policy matrix on entry of _main
      std::vector<CFG_FUNCTION_CALL> function_calls = getIndirectFunctionCalls();
      for(CFG_FUNCTION_CALL function_call : function_calls) {
        for(CFG_SYMBOL call : function_call.calls) {
          generateAndEmitAsm("CFIMATLDCALLER " + function_name + " +" + std::to_string(function_call.offset), firstInsn, firstBlock, false);
          generateAndEmitAsm("CFIMATLDCALLEE " + call.symbol_name, firstInsn, firstBlock, false);
        }
      }
    } else if (function_name.compare("printf") == 0) {
      // Disable CFI temporarily during printf
      // TODO: replace CFI_DBG7 with some sort of CFI_DISABLE instruction
      // TODO: Fix CFI in printf
      generateAndEmitAsm("CFI_DBG7 t0", firstInsn, firstBlock, false);
    }
  }
  
  void GCC_PLUGIN_FIXER::onFunctionRecursionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    onFunctionEntry(file_name, function_name, firstBlock, firstInsn);
  }

  void GCC_PLUGIN_FIXER::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    // Don't instrument function return of _main
    if (function_name.compare("_main") != 0) {
      // Enable CFI again after printf
      // TODO: replace CFI_DBG6 with some sort of CFI_ENABLE instruction
      // TODO: Fix CFI in printf
      if (function_name.compare("printf") == 0) {
        generateAndEmitAsm("CFI_DBG6 t0", lastInsn, lastBlock, false);
      }

      generateAndEmitAsm("CFIRET", lastInsn, lastBlock, false);
    } else {
      // disable CFI from here on
      //TODO: replace CFI_DBG7 with some sort of CFI_DISABLE instruction
      generateAndEmitAsm("CFI_DBG7 t0", lastInsn, lastBlock, false);
    }
  }

  void GCC_PLUGIN_FIXER::onDirectFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    // Don't instrument function call of MAIN
    if (function_name.compare("_main") != 0) {
      generateAndEmitAsm("CFICALL", insn, block, false);
    }
  }

  void GCC_PLUGIN_FIXER::onRecursiveFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    onDirectFunctionCall(file_name, function_name, block, insn);
  }

  void GCC_PLUGIN_FIXER::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectFunctionCall(function_name, file_name, line_number);
    if (label >= 0) {
      // Only emit CFIFWD in case there is an entry in the config file
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
        insn = generateAndEmitAsm("CFIFWD " + regName, insn, block, false);
      }
    } else {
      onDirectFunctionCall(file_name, function_name, block, insn);
    }
  }

  void GCC_PLUGIN_FIXER::init() {
    for (int i = 0; i < argc; i++) {
      if (std::strcmp(argv[i].key, "cfg_file") == 0) {
        std::cerr << "CFG file for instrumentation: " << argv[i].value << "\n";

        readConfigFile(argv[i].value);
        //printFunctionCalls();
        //printLabelJumps();
        //printIndirectlyCalledFunctions();
      }
    }
  }

  GCC_PLUGIN_FIXER *GCC_PLUGIN_FIXER::clone()
  {
    // We do not clone ourselves
    return this;
  }