#include "gcc_plugin_excec.h"

  GCC_PLUGIN_EXCEC::GCC_PLUGIN_EXCEC(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_EXCEC::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    // Don't instrument function entry of __main with a CFICHK
    if (strcmp(function_name.c_str(), "__main") == 0) {
      // reset CFI state (e.g., exit(1) might have left CFI module in a dirty state)
      generateAndEmitAsm(CFI_RESET, firstInsn, firstBlock, false);
      // enable CFI from here on
      generateAndEmitAsm(CFI_ENABLE, firstInsn, firstBlock, false);
    } else if (strcmp(function_name.c_str(), "exit") == 0) {
      // reset CFI state because exit() breaks out of CFG
      generateAndEmitAsm(CFI_RESET, firstInsn, firstBlock, false);
    } else {
      int label = getLabelForIndirectlyCalledFunction(function_name, file_name);

      // only instrument functions, which are known to be called indirectly
      if (label >= 0) {
        generateAndEmitAsm("CFICHECK " + std::to_string(label), firstInsn, firstBlock, false);
      }
    }
  }

  void GCC_PLUGIN_EXCEC::onFunctionRecursionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    onFunctionEntry(file_name, function_name, firstBlock, firstInsn);
  }

  void GCC_PLUGIN_EXCEC::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    if (function_name.compare("__main") == 0) {
      // disable CFI from here on
      generateAndEmitAsm(CFI_DISABLE, lastInsn, lastBlock, false);
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
          insn = generateAndEmitAsm("CFICHECK " + std::to_string(function_call.label), insn, lastBlock, true);
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
          // CFICALL_I 0x0 enables the following CFICHECK, which triggers an exception because of the label mismatch
          // generateAndEmitAsm("CFICHECK 0x1", insn, lastBlock, true);
          // generateAndEmitAsm("CFICALL_I 0x0", insn, lastBlock, true);

          generateAndEmitAsm("CFICHECK 0x0", insn, lastBlock, true);

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
        rtx outer = XVECEXP(PATTERN(insn), 0, 0);
        
        if (GET_CODE (outer) == SET) {
          outer = XEXP(outer, 1);
        }
        
        std::string regName = getRegisterNameForNumber(REGNO(XEXP(XEXP(outer, 0), 0)));
        std::cerr << "    Using register " << regName << std::endl;

        // increase stack pointer
        generateAndEmitAsm("addi	sp,sp,-4", insn, block, false);
        // push old register content to stack
        generateAndEmitAsm("SW	" + regName + ",0(sp)", insn, block, false);
        // re-route jump: write address of trampoline to register
        generateAndEmitAsm("LA " + regName +  ", _trampolines_" + std::string(function_name) + "_"  + std::to_string(line_number), insn, block, false);
        // add CFI instruction to announce an indirect call
        generateAndEmitAsm("CFICALL_I " + std::to_string(label), insn, block, false);

        basic_block lastBlock = lastRealBlockInFunction();
        rtx_insn *lastInsn = UpdatePoint::lastRealINSN(lastBlock);
        emitTrampolines(file_name, function_name, line_number, regName, lastBlock, lastInsn);
      } else {
        // add CFI instruction to announce an indirect call
        generateAndEmitAsm("CFICALL_I " + std::to_string(label), insn, block, false);
      }
    } else {
      handleIndirectFunctionCallWithoutConfigEntry(file_name, function_name, line_number);
    }
  }

  void GCC_PLUGIN_EXCEC::onNamedLabel(std::string file_name, std::string function_name, std::string label_name, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectJumpSymbol(file_name, function_name, label_name);

    if (label >= 0) {
      generateAndEmitAsm("CFICHECK " + std::to_string(label), insn, block, false);
    } else {
      std::cerr << "No label found..." << std::endl;
      exit(1);
    }
  }
  
  void GCC_PLUGIN_EXCEC::onSwitchCase(int label, basic_block block, rtx_insn *insn) {
    generateAndEmitAsm("CFICHECK " + std::to_string(label), insn, block, true);
  }

  void GCC_PLUGIN_EXCEC::onIndirectJump(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectJump(file_name, function_name);

    if (label >= 0) {
      generateAndEmitAsm("CFIJUMP_I " + std::to_string(label), insn, block, false);
    } else {
      handleIndirectJumpWithoutConfigEntry(file_name, function_name);
    }
  }

  int GCC_PLUGIN_EXCEC::onIndirectJumpWithJumpTable(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    writeLabelToTmpFile(readLabelFromTmpFile()+1);
    generateAndEmitAsm("CFIJUMP_I " + std::to_string(readLabelFromTmpFile()), insn, block, false);

    return readLabelFromTmpFile();
  }

  void GCC_PLUGIN_EXCEC::onSetJumpFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn, int index) {
    generateAndEmitAsm("cfisetjmp " + std::to_string(index), insn, block, false);

    rtx_insn *tmpInsn = NEXT_INSN(insn);
    while (NOTE_P(tmpInsn)) {
      tmpInsn = NEXT_INSN(tmpInsn);
    }

    // TODO: generate label dynamically
    generateAndEmitAsm("CFICHECK 0x1", insn, block, true);
  }

  void GCC_PLUGIN_EXCEC::onLongJumpFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn, int index) {
    generateAndEmitAsm("cfilongjmp " + std::to_string(index), insn, block, false);
  }

  void GCC_PLUGIN_EXCEC::init()
  {
    for (int i = 0; i < argc; i++) {
      if (std::strcmp(argv[i].key, "cfg_file") == 0) {
        std::cerr << "CFG file for instrumentation: " << argv[i].value << "\n";

        readConfigFile(argv[i].value);
        //prinExistingFunctions();
        //printFunctionCalls();
        //printLabelJumps();

        break;
      }
    }
  }

  GCC_PLUGIN_EXCEC *GCC_PLUGIN_EXCEC::clone()
  {
    // We do not clone ourselves
    return this;
  }