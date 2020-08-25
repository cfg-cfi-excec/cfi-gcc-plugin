#include "gcc_plugin_trampolines.h"

  GCC_PLUGIN_TRAMPOLINES::GCC_PLUGIN_TRAMPOLINES(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_TRAMPOLINES::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    // Don't instrument function entry of MAIN
    if (strcmp(function_name.c_str(), "main") != 0) {
      int label = getLabelForIndirectlyCalledFunction(function_name, file_name);

      if (label >= 0) {
        generateAndEmitAsm("CFICHK " + std::to_string(label), firstInsn, firstBlock, false);
      }
    }
  }

  void GCC_PLUGIN_TRAMPOLINES::emitTrampolines(std::string file_name, std::string function_name, int line_number, std::string register_name, basic_block lastBlock, rtx_insn *lastInsn) {
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

  void GCC_PLUGIN_TRAMPOLINES::onDirectFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    writeLabelToTmpFile(readLabelFromTmpFile()+1);
    unsigned label = readLabelFromTmpFile();

    generateAndEmitAsm("CFIBR " + std::to_string(label), insn, block, false);

    rtx_insn *tmpInsn = NEXT_INSN(insn);
    while (NOTE_P(tmpInsn)) {
      tmpInsn = NEXT_INSN(tmpInsn);
    }
    generateAndEmitAsm("CFIRET " + std::to_string(label), tmpInsn, block, false);
  }

  void GCC_PLUGIN_TRAMPOLINES::onRecursiveFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    onDirectFunctionCall(file_name, function_name, block, insn);
  }

  void GCC_PLUGIN_TRAMPOLINES::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {   
    writeLabelToTmpFile(readLabelFromTmpFile()+1);
    unsigned label = readLabelFromTmpFile(); 
    int labelPRC = getLabelForIndirectFunctionCall(function_name, file_name, line_number);
    std::string regName = getRegisterNameForNumber(REGNO(XEXP(XEXP(XEXP(XVECEXP(PATTERN(insn), 0, 0), 1), 0), 0)));

    rtx_insn *tmpInsn = NEXT_INSN(insn);
    while (NOTE_P(tmpInsn)) {
      tmpInsn = NEXT_INSN(tmpInsn);
    }

    // add CFIRET instruction after the indirect JALR
    generateAndEmitAsm("CFIRET " + std::to_string(label), tmpInsn, block, false);
    // increase stack pointer
    insn = generateAndEmitAsm("addi	sp,sp,-4", insn, block, false);
    // add CFIBR instruction (for backward edge protection)
    insn = generateAndEmitAsm("CFIBR " + std::to_string(label), insn, block, true);
    
    if (labelPRC >= 0) {
      // push old register content to stack
      generateAndEmitAsm("SW	" + regName + ",0(sp)", insn, block, false);
      // re-route jump: write address of trampoline to register
      generateAndEmitAsm("LA " + regName +  ", _trampolines_" + std::string(function_name) + "_"  + std::to_string(line_number), insn, block, false);
      // add CFIPRC instruction
      generateAndEmitAsm("CFIPRC " + std::to_string(labelPRC), insn, block, true);

      basic_block lastBlock = lastRealBlockInFunction();
      rtx_insn *lastInsn = lastRealINSN(lastBlock);
      emitTrampolines(file_name, function_name, line_number, regName, lastBlock, lastInsn);
    } else {
       printf("\033[31m Warning: NO CFI RULES FOR INDIRECT CALL IN %s:%s:%d \x1b[0m\n",file_name.c_str(), function_name.c_str(), line_number);
    }
  }

  void GCC_PLUGIN_TRAMPOLINES::onNamedLabel(std::string file_name, std::string function_name, std::string label_name, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectJumpSymbol(file_name, function_name, label_name);

    if (label >= 0) {
      generateAndEmitAsm("CFICHK " + std::to_string(label), insn, block, false);
    }
  }
  
  void GCC_PLUGIN_TRAMPOLINES::onIndirectJump(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectJump(file_name, function_name);

    if (label >= 0) {
      generateAndEmitAsm("CFIPRJ " + std::to_string(label), insn, block, false);
    } else {
       printf("\033[31m Warning: NO CFI RULES FOR INDIRECT JUMP IN %s:%s \x1b[0m\n",file_name.c_str(), function_name.c_str());
    }
  }

  void GCC_PLUGIN_TRAMPOLINES::init() {
    for (int i = 0; i < argc; i++) {
      if (std::strcmp(argv[i].key, "cfg_file") == 0) {
        std::cout << "CFG file for instrumentation: " << argv[i].value << "\n";

        readConfigFile(argv[i].value);
        //printFunctionCalls();
        //printLabelJumps();
        //printIndirectlyCalledFunctions();
      }
    }
  }

  GCC_PLUGIN_TRAMPOLINES *GCC_PLUGIN_TRAMPOLINES::clone() {
    // We do not clone ourselves
    return this;
  }
  
	void GCC_PLUGIN_TRAMPOLINES::onPluginFinished() {
    remove("tmp.txt");
  }
