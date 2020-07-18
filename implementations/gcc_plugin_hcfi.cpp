#include "gcc_plugin_hcfi.h"

struct CFG_FUNCTION;
struct CFG_EXISTING_FUNCTION;
struct CFG_FUNCTION_CALL;

GCC_PLUGIN_HCFI::GCC_PLUGIN_HCFI(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
		: GCC_PLUGIN(ctxt, arguments, argcounter) {
  init();
}

void GCC_PLUGIN_HCFI::print_existing_functions() {
  for(CFG_EXISTING_FUNCTION existing_function : existing_functions) {
    std::cout << "FUNCTION: " << existing_function.function_name << " (" << existing_function.file_name << ")" << '\n';
    for(CFG_FUNCTION called_by : existing_function.called_by) {
      std::cout << "    called by: " << called_by.function_name << " (" << called_by.file_name << ")" << '\n';
    }
  }
}

void GCC_PLUGIN_HCFI::print_function_call() {
  for(CFG_FUNCTION_CALL function_call : function_calls) {
    std::cout << "FUNCTION: " << function_call.function_name << " (" << function_call.file_name << ":" << function_call.line_number << ")" << '\n';
    for(CFG_FUNCTION calls : function_call.calls) {
      std::cout << "    calls: " << calls.function_name << " (" << calls.file_name << ")" << '\n';
    }
  }
}

int GCC_PLUGIN_HCFI::get_label_for_existing_function(std::string function_name, std::string file_name) {
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

int GCC_PLUGIN_HCFI::get_label_for_function_call(std::string function_name, std::string file_name, int line_number) {
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

  void GCC_PLUGIN_HCFI::onFunctionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {
    // Don't instrument function entry of MAIN
    if (strcmp(function_name.c_str(), "main") != 0) {
      int label = get_label_for_existing_function(function_name, file_name);

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

  void GCC_PLUGIN_HCFI::onFunctionExit(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) {
    
  }

  void GCC_PLUGIN_HCFI::onDirectFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    emitAsmInput("SETPC", insn, block, false);
    //printf ("    Generating SETPC \n");
  }

  void GCC_PLUGIN_HCFI::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    int label = get_label_for_function_call(function_name, file_name, line_number);
    std::string tmp = "SETPCLABEL " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    emitAsmInput(buff, insn, block, false);
  }

  void GCC_PLUGIN_HCFI::onNamedLabel(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {

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

void GCC_PLUGIN_HCFI::read_cfg_file(char * filename) {
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

      read_cfg_file(argv[i].value);
      //print_existing_functions();
      //print_function_call();
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