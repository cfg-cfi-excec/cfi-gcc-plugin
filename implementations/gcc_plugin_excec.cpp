#include "gcc_plugin_excec.h"

  GCC_PLUGIN_EXCEC::GCC_PLUGIN_EXCEC(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_EXCEC::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    // Don't instrument function entry of _main with a CFICHK
    if (strcmp(function_name.c_str(), "_main") == 0) {
      // reset CFI state (e.g., exit(1) might have left CFI module in a dirty state)
      //TODO: replace CFI_DBG5 with some sort of CFI_RESET instruction
      generateAndEmitAsm("CFI_DBG5 t0", firstInsn, firstBlock, false);
      // enable CFI from here on
      //TODO: replace CFI_DBG6 with some sort of CFI_ENABLE instruction
      generateAndEmitAsm("CFI_DBG6 t0", firstInsn, firstBlock, false);
    } else if (strcmp(function_name.c_str(), "exit") == 0) {
      // reset CFI state because exit() breaks out of CFG
      //TODO: replace CFI_DBG5 with some sort of CFI_RESET instruction
      generateAndEmitAsm("CFI_DBG5 t0", firstInsn, firstBlock, false);
    } else {
      int label = getLabelForIndirectlyCalledFunction(function_name, file_name);

      // only instrument functions, which are known to be called indirectly
      if (label >= 0) {
        generateAndEmitAsm("CFICHK " + std::to_string(label), firstInsn, firstBlock, false);
      }
    }
  }

  void GCC_PLUGIN_EXCEC::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    if (function_name.compare("_main") == 0) {
      // disable CFI from here on
      //TODO: replace CFI_DBG7 with some sort of CFI_DISABLE instruction
      generateAndEmitAsm("CFI_DBG7 t0", lastInsn, lastBlock, false);
    }
  }

  void GCC_PLUGIN_EXCEC::emitTrampolines(std::string file_name, std::string function_name, int line_number, std::string register_name, basic_block lastBlock, rtx_insn *lastInsn) {
    //Generate Trampolines for an indirect call in this function
    std::vector<CFG_FUNCTION_CALL> function_calls = getIndirectFunctionCalls();

    for(CFG_FUNCTION_CALL function_call : function_calls) {
      if (function_call.function_name.compare(function_name) == 0) {
        if (function_call.line_number == line_number) {
          // generate symbol for trampoline
          rtx_insn *insn = generateAndEmitAsm("_trampolines_" + std::string(function_name) + "_"  
            + std::to_string(function_call.line_number) + ":", lastInsn, lastBlock, true);
          // add CFICHK at the very beginning
          insn = generateAndEmitAsm("CFICHK " + std::to_string(function_call.label), insn, lastBlock, true);
          // restore original register content
          insn = generateAndEmitAsm("lw	" + register_name + ",0(sp)", insn, lastBlock, true);
          insn = generateAndEmitAsm("addi	sp,sp,4", insn, lastBlock, true);

          for(CFG_SYMBOL call : function_call.calls) {
            // load symbol address of one possible call target to t0
            insn = generateAndEmitAsm("LA t0, " + call.symbol_name, insn, lastBlock, true);
            // compare actual call target with t0
            insn = generateAndEmitAsm("BEQ t0, " + register_name + ", " + call.symbol_name + "+4", insn, lastBlock, true);        
          }

          // This is the "else-branch": if we arrive here, there is a CFI violation
          generateAndEmitAsm("CFIRET 0xFFFF", insn, lastBlock, true);

          break;
        }
      }
    } 
  }

  void GCC_PLUGIN_EXCEC::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectFunctionCall(function_name, file_name, line_number);
    if (label >= 0) {
      bool trampolinesNeeded = areTrampolinesNeeded(file_name, function_name, line_number);
      //std::cerr << "#### TRAMPOLINES NEEDED: " << (trampolinesNeeded ? "YES" : "NO") << std::endl;

      if (trampolinesNeeded) {
        std::string regName = getRegisterNameForNumber(REGNO(XEXP(XEXP(XEXP(XVECEXP(PATTERN(insn), 0, 0), 1), 0), 0)));

        // increase stack pointer
        generateAndEmitAsm("addi	sp,sp,-4", insn, block, false);
        // push old register content to stack
        generateAndEmitAsm("SW	" + regName + ",0(sp)", insn, block, false);
        // re-route jump: write address of trampoline to register
        generateAndEmitAsm("LA " + regName +  ", _trampolines_" + std::string(function_name) + "_"  + std::to_string(line_number), insn, block, false);
        // add CFIPRC instruction
        generateAndEmitAsm("CFIPRC " + std::to_string(label), insn, block, false);

        basic_block lastBlock = lastRealBlockInFunction();
        rtx_insn *lastInsn = UpdatePoint::lastRealINSN(lastBlock);
        emitTrampolines(file_name, function_name, line_number, regName, lastBlock, lastInsn);
      } else {
        // add CFIPRC instruction without trampolines
        generateAndEmitAsm("CFIPRC " + std::to_string(label), insn, block, false);
      }
    } else {
      std::cerr << "Warning: NO CFI RULES FOR INDIRECT CALL IN " << file_name.c_str() << ":" 
        << function_name.c_str() << ":" << std::to_string( line_number) << "\n";
    }
  }

  void GCC_PLUGIN_EXCEC::onNamedLabel(std::string file_name, std::string function_name, std::string label_name, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectJumpSymbol(file_name, function_name, label_name);

    if (label >= 0) {
      generateAndEmitAsm("CFICHK " + std::to_string(label), insn, block, false);
    }
  }
  
  void GCC_PLUGIN_EXCEC::onIndirectJump(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectJump(file_name, function_name);

    if (label >= 0) {
      //TODO: change to custom instruction
      generateAndEmitAsm("CFIPRJ " + std::to_string(label), insn, block, false);
    } else {
       printf("\033[31m Warning: NO CFI RULES FOR INDIRECT JUMP IN %s:%s \x1b[0m\n",file_name.c_str(), function_name.c_str());
    }
  }

  void GCC_PLUGIN_EXCEC::init()
  {
    for (int i = 0; i < argc; i++) {
      if (std::strcmp(argv[i].key, "cfg_file") == 0) {
        std::cerr << "CFG file for instrumentation: " << argv[i].value << "\n";

        readConfigFile(argv[i].value);
        //prinExistingFunctions();
        //printFunctionCalls();
      }
    }
  }

  GCC_PLUGIN_EXCEC *GCC_PLUGIN_EXCEC::clone()
  {
    // We do not clone ourselves
    return this;
  }