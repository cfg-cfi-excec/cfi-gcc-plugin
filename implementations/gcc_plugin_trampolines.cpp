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
      int label = get_label_for_existing_function(function_name, file_name);

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
    writeLabelToTmpFile(getLabelFromTmpFile()+1);
    unsigned label = getLabelFromTmpFile();

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
    writeLabelToTmpFile(getLabelFromTmpFile()+1);
    unsigned label = getLabelFromTmpFile();  

    std::string tmp = "CFIBR " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, insn, block, false);
    
    unsigned labelPRC = get_label_for_function_call(function_name, file_name, line_number);
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

  void GCC_PLUGIN_TRAMPOLINES::onNamedLabel(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
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

  void GCC_PLUGIN_TRAMPOLINES::writeLabelToTmpFile(unsigned label) {
    clearTmpFile();

    std::ofstream tmp ("tmp.txt");
    tmp << std::to_string(label) << "\n";
    tmp.close();
  }

  unsigned GCC_PLUGIN_TRAMPOLINES::getLabelFromTmpFile() {
    std::ifstream tmp ("tmp.txt");
    std::string label;
    std::getline(tmp, label);
    tmp.close();

    return atoi(label.c_str());
  }

  void GCC_PLUGIN_TRAMPOLINES::clearTmpFile() {
    std::ofstream tmp ("tmp.txt", std::ofstream::out | std::ofstream::trunc);
    tmp.close();
  }

  void GCC_PLUGIN_TRAMPOLINES::init() {
    for (int i = 0; i < argc; i++) {
      if (std::strcmp(argv[i].key, "cfg_file") == 0) {
        std::cout << "CFG file for instrumentation: " << argv[i].value << "\n";

        read_cfg_file(argv[i].value);
        //print_existing_functions();
        //print_function_call();
      }
    }
  }


  void GCC_PLUGIN_TRAMPOLINES::read_cfg_file(char * filename) {
    std::ifstream input( filename );

    std::string allowed_calls_title = "# allowed calls";
    std::string calls_title = "# indirect calls";

    bool section_allowed_calls = false;
    bool section_calls = false;

    size_t pos = 0;
    std::string token, token_name, token_file, file_name, function_name, line_number, label;
    std::string delimiter = " ";
    std::string delimiter_entry = ":";

    for(std::string line; getline( input, line ); ) {
      pos = 0;
      //std::cout << "LINE READ: " << line << std::endl;

      if (line.find(allowed_calls_title) != std::string::npos) {
        section_allowed_calls = true;
        section_calls = false;
      } else if (line.find(calls_title) != std::string::npos) {
        section_allowed_calls = false;
        section_calls = true;
      } else if (line.length() > 0) {
        if (section_allowed_calls) {
          // extract file name
          pos = line.find(delimiter);
          file_name = line.substr(0, pos);
          line.erase(0, pos + delimiter.length());

          // extract function name
          pos = line.find(delimiter);
          function_name = line.substr(0, pos);
          line.erase(0, pos + delimiter.length());

          // extract label
          pos = line.find(delimiter);
          label = line.substr(0, pos-1);
          line.erase(0, pos + delimiter.length());

          CFG_EXISTING_FUNCTION cfg_function;
          cfg_function.file_name = file_name;
          cfg_function.function_name = function_name;
          cfg_function.label = std::stoi(label);

          // extract allowed callers of this function
          while ((pos = line.find(delimiter)) != std::string::npos) {
              token = line.substr(0, pos);
              line.erase(0, pos + delimiter.length());

              pos = token.find(delimiter_entry);
              token_file = token.substr(0, pos);
              token.erase(0, pos + delimiter_entry.length());
              token_name = token;

              CFG_FUNCTION tmp;
              tmp.file_name = token_file;
              tmp.function_name = token_name;
              cfg_function.called_by.push_back(tmp);
          }

          if (line.length() > 0) {
              pos = line.find(delimiter_entry);
              token_file = line.substr(0, pos);
              line.erase(0, pos + delimiter_entry.length());
              token_name = line;

              CFG_FUNCTION tmp;
              tmp.file_name = token_file;
              tmp.function_name = token_name;
              cfg_function.called_by.push_back(tmp);
          }

          existing_functions.push_back(cfg_function);

        } else if (section_calls) {

          // extract file name
          pos = line.find(delimiter);
          file_name = line.substr(0, pos);
          line.erase(0, pos + delimiter.length());

          // extract function name
          pos = line.find(delimiter);
          function_name = line.substr(0, pos);
          line.erase(0, pos + delimiter.length());

          // extract line_number
          pos = line.find(delimiter);
          line_number = line.substr(0, pos-1);
          line.erase(0, pos + delimiter.length());

          CFG_FUNCTION_CALL cfg_function;
          cfg_function.file_name = file_name;
          cfg_function.function_name = function_name;
          cfg_function.line_number = std::stoi(line_number);
          
          // extract possible function calls
          while ((pos = line.find(delimiter)) != std::string::npos) {
              token = line.substr(0, pos);
              line.erase(0, pos + delimiter.length());

              pos = token.find(delimiter_entry);
              token_file = token.substr(0, pos);
              token.erase(0, pos + delimiter_entry.length());
              token_name = token;

              CFG_FUNCTION tmp;
              tmp.file_name = token_file;
              tmp.function_name = token_name;
              cfg_function.calls.push_back(tmp);
          }

          if (line.length() > 0) {
              pos = line.find(delimiter_entry);
              token_file = line.substr(0, pos);
              line.erase(0, pos + delimiter_entry.length());
              token_name = line;

              CFG_FUNCTION tmp;
              tmp.file_name = token_file;
              tmp.function_name = token_name;
              cfg_function.calls.push_back(tmp);
          }

          function_calls.push_back(cfg_function);
        }
      }
    }
  }

  void GCC_PLUGIN_TRAMPOLINES::print_existing_functions() {
    for(CFG_EXISTING_FUNCTION existing_function : existing_functions) {
      std::cout << "FUNCTION: " << existing_function.function_name << " (" << existing_function.file_name << ")" << '\n';
      for(CFG_FUNCTION called_by : existing_function.called_by) {
        std::cout << "    called by: " << called_by.function_name << " (" << called_by.file_name << ")" << '\n';
      }
    }
  }

  void GCC_PLUGIN_TRAMPOLINES::print_function_call() {
    for(CFG_FUNCTION_CALL function_call : function_calls) {
      std::cout << "FUNCTION: " << function_call.function_name << " (" << function_call.file_name << ":" << function_call.line_number << ")" << '\n';
      for(CFG_FUNCTION calls : function_call.calls) {
        std::cout << "    calls: " << calls.function_name << " (" << calls.file_name << ")" << '\n';
      }
    }
  }

  int GCC_PLUGIN_TRAMPOLINES::get_label_for_existing_function(std::string function_name, std::string file_name) {
    for(CFG_EXISTING_FUNCTION existing_function : existing_functions) {
      if (existing_function.file_name.compare(file_name) == 0) {
        if (existing_function.function_name.compare(function_name) == 0) {
          return existing_function.label;
        }
      }
    }

    printf("NOT FOUND: %s -- %s \n\n", function_name.c_str(), file_name.c_str());

    return -1;
  }

  int GCC_PLUGIN_TRAMPOLINES::get_label_for_function_call(std::string function_name, std::string file_name, int line_number) {
    for(CFG_FUNCTION_CALL function_call : function_calls) {
      if (function_call.file_name.compare(file_name) == 0) {
        if (function_call.function_name.compare(function_name) == 0) {
          if (function_call.line_number == line_number) {
            // possible call targets for this call in function_call.calls
            if (function_call.calls.size() > 0) {
              // assume all functions in calls vector have the same label
              CFG_FUNCTION tmp = function_call.calls.front();

              return get_label_for_existing_function(tmp.function_name, tmp.file_name);
            }
          }
        }
      }
    }

    return -1;
  }

  GCC_PLUGIN_TRAMPOLINES *GCC_PLUGIN_TRAMPOLINES::clone() {
    // We do not clone ourselves
    return this;
  }
  
	void GCC_PLUGIN_TRAMPOLINES::onPluginFinished() {
    remove("tmp.txt");
  }
