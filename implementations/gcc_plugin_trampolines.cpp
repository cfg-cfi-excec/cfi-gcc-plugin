#include "gcc_plugin_trampolines.h"

  struct CFG_FUNCTION;
  struct CFG_EXISTING_FUNCTION;
  struct CFG_FUNCTION_CALL;

  GCC_PLUGIN_TRAMPOLINES::GCC_PLUGIN_TRAMPOLINES(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_TRAMPOLINES::onFunctionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {
    // Don't instrument function entry of MAIN
    if (strcmp(function_name.c_str(), "main") != 0) {
      int label = getLabelForExistingFunction(function_name, file_name);

      //printf("LABEL: %d \n\n\n", label);
      std::string tmp = "CFICHK " + std::to_string(label);  

      char *buff = new char[tmp.size()+1];
      std::copy(tmp.begin(), tmp.end(), buff);
      buff[tmp.size()] = '\0';

      emitAsmInput(buff, firstInsn, firstBlock, false);
    }
    
    /*unsigned label = 123;

    std::string tmp = "CFIBR " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, firstInsn, firstBlock, false);*/

/*
    rtx_code_label *new_label = gen_label_rtx ();
    rtx_insn* label = emit_label (new_label);

    debug_rtx(label);
    rtx_insn* jump =  emit_jump_insn (gen_jump  (label));
    debug_rtx(jump);

    basic_block new_bb = create_basic_block (label, jump, firstBlock);
    new_bb->aux = firstBlock->aux;
    new_bb->count = firstBlock->count;
    firstBlock->aux = new_bb;
    emit_barrier_after_bb (new_bb);
    make_single_succ_edge (new_bb, firstBlock, 0);*/



    //emitInsn(jump, firstInsn, firstBlock, false);
  }

  void GCC_PLUGIN_TRAMPOLINES::onFunctionRecursionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {
    /*unsigned label = 123;

    std::string tmp = "CFIREC " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, firstInsn, firstBlock, false);*/
  }

  void GCC_PLUGIN_TRAMPOLINES::onFunctionReturn(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) {
    
    //This Jump-Approach actually works:
    /*
    rtx_insn* label = emitCodeLabel(1234, lastInsn, lastBlock, false);
	  rtx_insn* jump =  emit_jump_insn (gen_jump  (label));
   
    JUMP_LABEL(jump) = label;
	  LABEL_NUSES (label)++;
 	  emit_barrier ();
    */


    //This code works for generating trampolines:
    /*
    std::string tmp = "J functionWithReturnValueWithArgument+4";  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    rtx_insn *insn = emitAsmInput(buff, lastInsn, lastBlock, true);
    
    tmp = "CFICHK " + std::to_string(42);  

    buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, insn, lastBlock, false);
    */
  }

  void GCC_PLUGIN_TRAMPOLINES::onFunctionExit(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) {

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
    
    unsigned labelPRC = getLabelForFunctionCall(function_name, file_name, line_number);
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
    
    //printf("\nasdf###########################################\n");
    //debug_rtx(insn);
    //printf("\n###########################################\n");

   // rtx_insn* label = emitCodeLabel(1234, insn, block, false);
	  //rtx_insn* jump =  emit_jump_insn (gen_jump  (label));
   
   // JUMP_LABEL(insn) = label;
	 // LABEL_NUSES (label)++;

  }

  void GCC_PLUGIN_TRAMPOLINES::onNamedLabel(const tree_node *tree, char *fName, const char *label_name, basic_block block, rtx_insn *insn) {
    //TODO: Set Label
    unsigned label = 123;

    std::string tmp = "CFICHK " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, insn, block, false);
  }
  
  void GCC_PLUGIN_TRAMPOLINES::onIndirectJump(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    //TODO: Set Label
    unsigned label = 123;

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
        //prinExistingFunctions();
        //printFunctionCalls();
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
