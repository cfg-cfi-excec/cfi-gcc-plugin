#include "gcc_plugin_trampolines.h"

  GCC_PLUGIN_TRAMPOLINES::GCC_PLUGIN_TRAMPOLINES(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_TRAMPOLINES::onFunctionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {
    // Don't instrument function entry of MAIN
    if (strcmp(function_name.c_str(), "main") != 0) {
      int label = getLabelForIndirectlyCalledFunction(function_name, file_name);

      if (label >= 0) {
        std::string tmp = "CFICHK " + std::to_string(label);  

        char *buff = new char[tmp.size()+1];
        std::copy(tmp.begin(), tmp.end(), buff);
        buff[tmp.size()] = '\0';

        emitAsmInput(buff, firstInsn, firstBlock, false);
      }
    }
  }

  void GCC_PLUGIN_TRAMPOLINES::onFunctionRecursionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {
  }

  void GCC_PLUGIN_TRAMPOLINES::onFunctionReturn(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) {
    //This Jump-Approach actually works:
    /*
    rtx_insn* label = emitCodeLabel(lastInsn, lastBlock, false);
	  rtx_insn* jump =  emit_jump_insn (gen_jump  (label));
   
    JUMP_LABEL(jump) = label;
	  LABEL_NUSES (label)++;
 	  emit_barrier ();
    */
  }

  void GCC_PLUGIN_TRAMPOLINES::onFunctionExit(std::string file_name, char *function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    //Generate Trampolines for each indirect call in this function
    std::vector<CFG_FUNCTION_CALL> function_calls = getIndirectFunctionCalls();

    for(CFG_FUNCTION_CALL function_call : function_calls) {
      if (function_call.function_name.compare(function_name) == 0) {
        std::string tmp = "_trampolines_" + std::string(function_name) + "_"  + std::to_string(function_call.line_number) + ":"; 
        char *buff = new char[tmp.size()+1];
        std::copy(tmp.begin(), tmp.end(), buff);
        buff[tmp.size()] = '\0';
        rtx_insn *insn = emitAsmInput(buff, lastInsn, lastBlock, true);

        tmp = "CFICHK " + std::to_string(function_call.label);  
        buff = new char[tmp.size()+1];
        std::copy(tmp.begin(), tmp.end(), buff);
        buff[tmp.size()] = '\0';
        insn = emitAsmInput(buff, insn, lastBlock, true);

        for(CFG_SYMBOL call : function_call.calls) {
          tmp = "LA t0, " + call.symbol_name;  
          buff = new char[tmp.size()+1];
          std::copy(tmp.begin(), tmp.end(), buff);
          buff[tmp.size()] = '\0';
          insn = emitAsmInput(buff, insn, lastBlock, true);
          
          tmp = "BEQ t0, t1, " + call.symbol_name + "+4";  
          buff = new char[tmp.size()+1];
          std::copy(tmp.begin(), tmp.end(), buff);
          buff[tmp.size()] = '\0';
          insn = emitAsmInput(buff, insn, lastBlock, true);          
        }

        //TODO: add else-branch for final exception (if none of the BEQs hit)
      }
    } 
  }

  void GCC_PLUGIN_TRAMPOLINES::onDirectFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    writeLabelToTmpFile(readLabelFromTmpFile()+1);
    unsigned label = readLabelFromTmpFile();

    std::string tmp = "CFIBR " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, insn, block, false);

    tmp = "CFIRET " + std::to_string(label);  

    buff = new char[tmp.size()+1];
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
    writeLabelToTmpFile(readLabelFromTmpFile()+1);
    unsigned label = readLabelFromTmpFile();  
    std::string tmp = "CFIBR " + std::to_string(label);  
    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';
    emitAsmInput(buff, insn, block, false);
    
    unsigned labelPRC = getLabelForIndirectFunctionCall(function_name, file_name, line_number);
    tmp = "CFIPRC " + std::to_string(labelPRC);  
    buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';
    emitAsmInput(buff, insn, block, false);

    tmp = "CFIRET " + std::to_string(label);  
    buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';
    rtx_insn *tmpInsn = NEXT_INSN(insn);
    while (NOTE_P(tmpInsn)) {
      tmpInsn = NEXT_INSN(tmpInsn);
    }
    emitAsmInput(buff, tmpInsn, block, false);

    // copy target of indirect jump to register t1
    std::string regName = "";

    switch (REGNO(XEXP(XEXP(XEXP(XVECEXP(PATTERN(insn), 0, 0), 1), 0), 0))) {
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
      default: exit(1);
    };

    tmp = "add t1, " + regName + ", zero";  
    buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';
    emitAsmInput(buff, insn, block, false);

    // re-route jump: write address of trampoline to register
    tmp = "la " + regName + ", _trampolines_" + std::string(function_name) + "_"  + std::to_string(line_number);  
    buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';
    emitAsmInput(buff, insn, block, false);
  }

  void GCC_PLUGIN_TRAMPOLINES::onNamedLabel(std::string file_name, std::string function_name, std::string label_name, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectJumpSymbol(file_name, function_name, label_name);

    if (label >= 0) {
      std::string tmp = "CFICHK " + std::to_string(label);  
      char *buff = new char[tmp.size()+1];
      std::copy(tmp.begin(), tmp.end(), buff);
      buff[tmp.size()] = '\0';
      emitAsmInput(buff, insn, block, false);
    }
  }
  
  void GCC_PLUGIN_TRAMPOLINES::onIndirectJump(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    unsigned label = getLabelForIndirectJump(file_name, function_name);

    std::string tmp = "CFIPRJ " + std::to_string(label);  
    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';
    emitAsmInput(buff, insn, block, false);
  }

  void GCC_PLUGIN_TRAMPOLINES::onSetJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    // Do nothing...
  }

  void GCC_PLUGIN_TRAMPOLINES::onLongJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    // Do nothing...
  }

  void GCC_PLUGIN_TRAMPOLINES::init() {
    for (int i = 0; i < argc; i++) {
      if (std::strcmp(argv[i].key, "cfg_file") == 0) {
        std::cout << "CFG file for instrumentation: " << argv[i].value << "\n";

        readConfigFile(argv[i].value);
        printFunctionCalls();
        printLabelJumps();
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
