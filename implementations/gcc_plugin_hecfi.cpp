#include "gcc_plugin_hecfi.h"

  GCC_PLUGIN_HECFI::GCC_PLUGIN_HECFI(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_HECFI::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
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

      if (label >= 0) {
        generateAndEmitAsm("CFICHK " + std::to_string(label), firstInsn, firstBlock, false);
      }
    }
  }

  void GCC_PLUGIN_HECFI::onFunctionRecursionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    onFunctionEntry(file_name, function_name, firstBlock, firstInsn);
  }

  void GCC_PLUGIN_HECFI::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    if (function_name.compare("__main") == 0) {
      // disable CFI from here on
      generateAndEmitAsm(CFI_DISABLE, lastInsn, lastBlock, false);
    }
  }

  void GCC_PLUGIN_HECFI::emitTrampolines(std::string file_name, std::string function_name, int line_number, std::string register_name, basic_block lastBlock, rtx_insn *lastInsn) {
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

  void GCC_PLUGIN_HECFI::onDirectFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    // Don't instrument functions in libgcc
    if (isLibGccFunction(function_name)) {
      // disable CFI from here on
      generateAndEmitAsm(CFI_DISABLE, insn, block, false);

      rtx_insn *tmpInsn = NEXT_INSN(insn);
      while (NOTE_P(tmpInsn)) {
        tmpInsn = NEXT_INSN(tmpInsn);
      }

      // Enable CFI again after excluded function
      generateAndEmitAsm(CFI_ENABLE, tmpInsn, block, false);
    } else {
      writeLabelToTmpFile(readLabelFromTmpFile()+1);
      unsigned label = readLabelFromTmpFile();

      generateAndEmitAsm("CFIBR " + std::to_string(label), insn, block, false);

      rtx_insn *tmpInsn = NEXT_INSN(insn);
      while (NOTE_P(tmpInsn)) {
        tmpInsn = NEXT_INSN(tmpInsn);
      }
      generateAndEmitAsm("CFIRET " + std::to_string(label), tmpInsn, block, false);
    }
  }

  void GCC_PLUGIN_HECFI::onSetJumpFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn, int indexn) {
    // setjmp calls require normal instrumentation just as any other call
    onDirectFunctionCall(file_name, function_name, block, insn);
  }
	
  void GCC_PLUGIN_HECFI::onLongJumpFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn, int index) {
    // lonjmp calls require normal instrumentation just as any other call
    onDirectFunctionCall(file_name, function_name, block, insn);
  }

  void GCC_PLUGIN_HECFI::onRecursiveFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    onDirectFunctionCall(file_name, function_name, block, insn);
  }

  void GCC_PLUGIN_HECFI::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {   
    writeLabelToTmpFile(readLabelFromTmpFile()+1);
    unsigned label = readLabelFromTmpFile(); 
    int labelPRC = getLabelForIndirectFunctionCall(function_name, file_name, line_number);
    rtx_insn *indirectCall = insn;
    rtx_insn *tmpInsn = NEXT_INSN(insn);
    while (NOTE_P(tmpInsn)) {
      tmpInsn = NEXT_INSN(tmpInsn);
    }

    // add CFIRET instruction after the indirect JALR
    generateAndEmitAsm("CFIRET " + std::to_string(label), tmpInsn, block, false);
    // add CFIBR instruction (for backward edge protection)
    insn = generateAndEmitAsm("CFIBR " + std::to_string(label), insn, block, false);
    
    if (labelPRC >= 0) {
      bool trampolinesNeeded = areTrampolinesNeeded(file_name, function_name, line_number);
      //std::cerr << "#### TRAMPOLINES NEEDED: " << (trampolinesNeeded ? "YES" : "NO") << std::endl;

      if (trampolinesNeeded) {
        rtx outer = XVECEXP(PATTERN(indirectCall), 0, 0);
        
        if (GET_CODE (outer) == SET) {
          outer = XEXP(outer, 1);
        }
        
        std::string regName = getRegisterNameForNumber(REGNO(XEXP(XEXP(outer, 0), 0)));

        // increase stack pointer
        generateAndEmitAsm("addi	sp,sp,-4", insn, block, false);
        // push old register content to stack
        generateAndEmitAsm("SW	" + regName + ",0(sp)", insn, block, false);
        // re-route jump: write address of trampoline to register
        generateAndEmitAsm("LA " + regName +  ", _trampolines_" + std::string(function_name) + "_"  + std::to_string(line_number), insn, block, false);
        // add CFIPRC instruction
        generateAndEmitAsm("CFIPRC " + std::to_string(labelPRC), insn, block, true);

        basic_block lastBlock = lastRealBlockInFunction();
        rtx_insn *lastInsn = UpdatePoint::lastRealINSN(lastBlock);
        emitTrampolines(file_name, function_name, line_number, regName, lastBlock, lastInsn);
      } else {
        // add CFIPRC instruction without trampolines
        generateAndEmitAsm("CFIPRC " + std::to_string(labelPRC), insn, block, true);
      }
    } else {
      handleIndirectFunctionCallWithoutConfigEntry(file_name, function_name, line_number);
    }
  }

  void GCC_PLUGIN_HECFI::onNamedLabel(std::string file_name, std::string function_name, std::string label_name, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectJumpSymbol(file_name, function_name, label_name);

    if (label >= 0) {
      generateAndEmitAsm("CFICHK " + std::to_string(label), insn, block, false);
    }
  }
  
  void GCC_PLUGIN_HECFI::onSwitchCase(int label, basic_block block, rtx_insn *insn) {
    generateAndEmitAsm("CFICHK " + std::to_string(label), insn, block, true);
  }
  
  void GCC_PLUGIN_HECFI::onIndirectJump(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectJump(file_name, function_name);

    if (label >= 0) {
      generateAndEmitAsm("CFIPRJ " + std::to_string(label), insn, block, false);
    } else {
      handleIndirectJumpWithoutConfigEntry(file_name, function_name);
    }
  }

  void GCC_PLUGIN_HECFI::init() {
    for (int i = 0; i < argc; i++) {
      if (std::strcmp(argv[i].key, "cfg_file") == 0) {
        std::cerr << "CFG file for instrumentation: " << argv[i].value << "\n";

        readConfigFile(argv[i].value);
        //printFunctionCalls();
        //printLabelJumps();
        //printIndirectlyCalledFunctions();

        break;
      }
    }
  }

  GCC_PLUGIN_HECFI *GCC_PLUGIN_HECFI::clone() {
    // We do not clone ourselves
    return this;
  }
  
	void GCC_PLUGIN_HECFI::onPluginFinished() {
    remove("tmp.txt");
  }
