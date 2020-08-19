#include "gcc_plugin_hcfi.h"

  GCC_PLUGIN_HCFI::GCC_PLUGIN_HCFI(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_HCFI::onFunctionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {
    // Don't instrument function entry of MAIN
    if (strcmp(function_name.c_str(), "main") != 0) {
      int label = getLabelForIndirectlyCalledFunction(function_name, file_name);

      //printf("LABEL: %d \n\n\n", label);
      std::string tmp = "CHECKLABEL " + std::to_string(label);  

      char *buff = new char[tmp.size()+1];
      std::copy(tmp.begin(), tmp.end(), buff);
      buff[tmp.size()] = '\0';

      emitAsmInput(buff, firstInsn, firstBlock, false);
    }
  }
  
  void GCC_PLUGIN_HCFI::onFunctionRecursionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {
    onFunctionEntry(file_name, function_name, line_number, firstBlock, firstInsn);
  }

  void GCC_PLUGIN_HCFI::onFunctionReturn(const tree_node *tree, char *function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    // Don't instrument function entry of MAIN
    if (strcmp(function_name, "main") != 0) {
      emitAsmInput("CHECKPC", lastInsn, lastBlock, false);
      //printf ("    Generating CHECKPC \n");
    }
  }

  void GCC_PLUGIN_HCFI::onFunctionExit(std::string file_name, char *fName, basic_block lastBlock, rtx_insn *lastInsn) {
    
  }

  void GCC_PLUGIN_HCFI::onDirectFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    emitAsmInput("SETPC", insn, block, false);
    //printf ("    Generating SETPC \n");
  }

  void GCC_PLUGIN_HCFI::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectFunctionCall(function_name, file_name, line_number);
    std::string tmp = "SETPCLABEL " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, insn, block, false);
  }

  void GCC_PLUGIN_HCFI::onNamedLabel(std::string file_name, std::string function_name, std::string label_name, basic_block block, rtx_insn *insn) {

  }
  
  void GCC_PLUGIN_HCFI::onIndirectJump(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {

  }

  void GCC_PLUGIN_HCFI::onRecursiveFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    onDirectFunctionCall(tree, fName, block, insn);
  }

  void GCC_PLUGIN_HCFI::onSetJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    //TODO: Set Label propperly
    emitAsmInput("SJCFI 0x42", insn, block, false);
    //printf ("    Generating SJCFI \n");
  }

  void GCC_PLUGIN_HCFI::onLongJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    emitAsmInput("LJCFI", insn, block, false);
    //printf ("    Generating LJCFI \n");
  }


  /*
  // Attribute handler callback
  static tree handle_user_attribute (tree *node, tree name, tree args, int flags, bool *no_add_attrs) {
    std::cout << "CALL ATTRI ##############################################################"  << "\n";
  
    debug_tree(*node);
    debug_tree(args);

    return NULL_TREE;
  }

  // Attribute definition
  static struct attribute_spec user_attr = { "user", 1, 1, false,  false, false, handle_user_attribute, false };

  static void register_attributes (void *event_data, void *data) {
    register_attribute (&user_attr);
  }
  */

  void GCC_PLUGIN_HCFI::init()
  {
    for (int i = 0; i < argc; i++) {
      if (std::strcmp(argv[i].key, "cfg_file") == 0) {
        std::cout << "CFG file for instrumentation: " << argv[i].value << "\n";

        readConfigFile(argv[i].value);
        //prinExistingFunctions();
        //printFunctionCalls();
      }
    }
  }

  GCC_PLUGIN_HCFI *GCC_PLUGIN_HCFI::clone()
  {
    // We do not clone ourselves
    return this;
  }

	void GCC_PLUGIN_HCFI::onPluginFinished() {

  }