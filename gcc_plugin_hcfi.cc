#include <iostream>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <vector>

// This is the first gcc header to be included
#include "gcc-plugin.h"
#include "plugin-version.h"
#include <context.h>
#include <basic-block.h>
#include <rtl.h>
#include <tree-pass.h>
#include <tree.h>
#include <print-tree.h>


struct CFG_FUNCTION {
    std::string file_name;
    std::string function_name;
};

struct CFG_EXISTING_FUNCTION {
    std::string file_name;
    std::string function_name;
    std::string cmd_check_label;
    int label;
    std::vector<CFG_FUNCTION> called_by;
};

struct CFG_FUNCTION_CALL {
    std::string file_name;
    std::string function_name;
    int line_number;
    std::vector<CFG_FUNCTION> calls;
};

std::vector<CFG_EXISTING_FUNCTION> existing_functions;
std::vector<CFG_FUNCTION_CALL> function_calls;


// We must assert that this plugin is GPL compatible
int plugin_is_GPL_compatible = 1;

static struct plugin_info my_gcc_plugin_info =
{ "1.0", "This is a very simple plugin" };


void print_existing_functions() {
  for(CFG_EXISTING_FUNCTION existing_function : existing_functions) {
    std::cout << "FUNCTION: " << existing_function.function_name << " (" << existing_function.file_name << ")" << '\n';
    for(CFG_FUNCTION called_by : existing_function.called_by) {
      std::cout << "    called by: " << called_by.function_name << " (" << called_by.file_name << ")" << '\n';
    }
  }
}

void print_function_call() {
  for(CFG_FUNCTION_CALL function_call : function_calls) {
    std::cout << "FUNCTION: " << function_call.function_name << " (" << function_call.file_name << ":" << function_call.line_number << ")" << '\n';
    for(CFG_FUNCTION calls : function_call.calls) {
      std::cout << "    calls: " << calls.function_name << " (" << calls.file_name << ")" << '\n';
    }
  }
}

int get_label_for_existing_function(std::string function_name, std::string file_name) {
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

int get_label_for_function_call(std::string function_name, std::string file_name, int line_number) {
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

namespace {
  const struct pass_data my_first_pass_data =
{
		.type = RTL_PASS,
		.name = "my_first_pass",
		.optinfo_flags = OPTGROUP_NONE,
		.tv_id = TV_TREE_CLEANUP_CFG,
		.properties_required = 0,//(PROP_rtl | PROP_cfglayout),
		.properties_provided = 0,
		.properties_destroyed = 0,
		.todo_flags_start = 0,
		.todo_flags_finish = 0,
};

struct my_first_pass : gimple_opt_pass
{
  my_first_pass(gcc::context * ctx) :
    gimple_opt_pass(my_first_pass_data, ctx)
  {
  }

  // Credits to https://github.com/MGroupKULeuvenBrugesCampus/CFED_Plugin for these functions

  /**
  * Method to find the provided rtx_code in the given
  * rtx. Is a recursive method to make sure all fields of
  * the provided rtx is examined.
  */
  bool findCode(rtx expr, rtx_code code){
    if( expr == 0x00 ){
      return false;
    }

    rtx_code exprCode = (rtx_code) expr->code;			// Get the code of the expression
    const char* format = GET_RTX_FORMAT(exprCode);		// Get the format of the expression, tells what operands are expected

    if(exprCode == code){					// Test if expression is a CODE expression
      return true;
    }
    else if(exprCode == ASM_OPERANDS){
      return false;
    }
    else{
      for (int x=0; x < GET_RTX_LENGTH(exprCode); x++){	// Loop over all characters in the format
        if(format[x] == 'e'){							// Test if they are an expression
          rtx subExpr = XEXP(expr,x);					// Get the expression
          if (findCode(subExpr, code)){				// Recursive call to this function
            return true;
          }
        }
        else if(format[x] == 'E'){						// Test if a Vector
          for(int i=0; i<XVECLEN(expr,0);i++){		// Loop over all expressions in the vector
            rtx subExpr = XVECEXP(expr, 0, i);		// Get the expression
            if(findCode(subExpr, code)){			// Recursive call to this function
              return true;
            }
          }
        }
      }
    }
    return false;
  }

  /*
  * Actually emits the insn at the desired place.
  */
  rtx_insn* emitInsn(rtx rtxInsn,rtx_insn* attachRtx, basic_block bb, bool after){
    if (after){
      return emit_insn_after_noloc(rtxInsn, attachRtx, bb);
    }
    else{
      return emit_insn_before_noloc(rtxInsn, attachRtx, bb);
    }
  }

  /**
  * Function that returns the first real INSN of
  * the basic block bb.
  * (No Debug or Note insn)
  */
  rtx_insn* firstRealINSN(basic_block bb){
    rtx_insn* next = BB_HEAD(bb);
    while(!NONDEBUG_INSN_P(next)){
      next = NEXT_INSN(next);
    }
    return next;
  }

  /**
  * Function that returns the last real INSN
  * of the basic block
  * (No Debug or Note insn)
  */
  rtx_insn* lastRealINSN(basic_block bb){
    rtx_insn* lastInsn = BB_END(bb);
    while(!NONDEBUG_INSN_P(lastInsn)){
      lastInsn = PREV_INSN(lastInsn);
    }
    return lastInsn;
  }

  /**
  * Emits the provided assembly instruction
  */
  rtx_insn* emitAsmInput(const char* asmInstr, rtx_insn* attachRtx, basic_block bb, bool after){
    rtx asmBxLR = gen_rtx_ASM_INPUT_loc(VOIDmode, asmInstr, 1);
    asmBxLR->volatil=1;
    rtx memRTX = gen_rtx_MEM(BLKmode, gen_rtx_SCRATCH(VOIDmode));
    rtx clobber = gen_rtx_CLOBBER(VOIDmode, memRTX);
    rtvec vec = rtvec_alloc(2);
    vec->elem[0] = asmBxLR;
    vec->elem[1] = clobber;
    rtx par = gen_rtx_PARALLEL(VOIDmode, vec);
    rtx_insn* insn = emitInsn(par, attachRtx, bb, after);
    return insn;
  }

  /**
  * Method to easily create a CONST_INT rtx
  */
  rtx createConstInt(int number){
    rtx constInt = rtx_alloc(CONST_INT);
    XWINT(constInt,0) = number;
    return constInt;
  }

  bool isCall(rtx_insn* expr){
    rtx innerExpr = XEXP(expr, 3);
    return findCode(innerExpr, CALL);
  }
  
  /**
  * Method to determine whether or not the provided
  * rtx_insn is a return instruction
  */
  bool isReturn(rtx_insn* expr){
    rtx innerExpr = XEXP(expr, 3);
    bool ret =  findCode(innerExpr, RETURN);
    bool simpRet = findCode(innerExpr, SIMPLE_RETURN);
    return (ret || simpRet);
  }

  void printRtxClass(rtx_code code) {
    if (GET_RTX_CLASS(code) == RTX_OBJ) {
      printf("    RTX_CLASS: RTX_OBJ\n");
    } else if (GET_RTX_CLASS(code) == RTX_CONST_OBJ) {
      printf("    RTX_CLASS: RTX_CONST_OBJ\n");
    } else if (GET_RTX_CLASS(code) == RTX_INSN) {
      printf("    RTX_CLASS: RTX_INSN\n");
    } else if (GET_RTX_CLASS(code) == RTX_EXTRA) {
      printf("    RTX_CLASS: RTX_EXTRA\n");
    } else  {
      printf("    RTX_CLASS: -------- ");
    }     
  }

  rtx_insn* instrumentFunctionEntry(std::string file_name, std::string function_name, int line_number, basic_block firstBlock, rtx_insn *firstInsn) {
    int label = get_label_for_existing_function(function_name, file_name);

    //printf("LABEL: %d \n\n\n", label);
    std::string tmp = "CHECKLABEL " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    return emitAsmInput(buff, firstInsn, firstBlock, false);
  }

  void instrumentFunctionReturn(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) {
    emitAsmInput("CHECKPC", lastInsn, lastBlock, false);
    //printf ("    Generating CHECKPC \n");
  }

  void instrumentFunctionExit(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) {

  }

  void instrumentDirectFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    emitAsmInput("SETPC", insn, block, false);
    //printf ("    Generating SETPC \n");
  }

  rtx_insn* instrumentIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    int label = get_label_for_function_call(function_name, file_name, line_number);
    std::string tmp = "SETPCLABEL " + std::to_string(label);  

    char *buff = new char[tmp.size()+1];
    std::copy(tmp.begin(), tmp.end(), buff);
    buff[tmp.size()] = '\0';

    return emitAsmInput(buff, insn, block, false);
  }

  void instrumentSetJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    //TODO: Set Label propperly
    emitAsmInput("SJCFI 0x42", insn, block, false);
    //printf ("    Generating SJCFI \n");
  }

  void instrumentLongJumpFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    emitAsmInput("LJCFI", insn, block, false);
    //printf ("    Generating LJCFI \n");
  }

  virtual unsigned int execute(function * fun) override
  {    
    const_tree funTree = get_base_address(current_function_decl);
	  char* function_name = (char*)IDENTIFIER_POINTER (DECL_NAME (current_function_decl) );
    printf("\x1b[92m GCC Plugin executing for function \x1b[92;1m %s \x1b[0m\n",function_name);

    printf("FUNCTION NAME: %s \n", function_name);
    printf("FUNCTION ADDRESS: %p\n", current_function_decl);

    //debug_tree(current_function_decl);

    try{
      basic_block bb;

      // Don't instrument function entry of MAIN
      if (strcmp(function_name, "main") != 0) {
        bb = single_succ(ENTRY_BLOCK_PTR_FOR_FN(cfun));
        rtx_insn* firstInsn = firstRealINSN(bb);
        instrumentFunctionEntry(LOCATION_FILE(INSN_LOCATION (firstInsn)), function_name, LOCATION_LINE(INSN_LOCATION (firstInsn)), bb, firstInsn);
        //printf("%s %s ###: \n", LOCATION_FILE(INSN_LOCATION (firstInsn)), function_name);
      }

      FOR_EACH_BB_FN(bb, cfun){
          
        rtx_insn* insn;
        FOR_BB_INSNS (bb, insn) {
          if (CALL_P (insn) && isCall(insn)) {
            bool isDirectCall = true;
            
            // Get the body of the function call
            rtx body = PATTERN(insn);
            rtx parallel = XVECEXP(body, 0, 0);
            rtx call;

            //printf("CALL INSTRUCTION in %s : %d \n", LOCATION_FILE(INSN_LOCATION (insn)), LOCATION_LINE(INSN_LOCATION (insn)));

            // Check whether the function returns a value or not:
            // - If it returns a value, the first element of the body is a SET
            // - Otherwiese it is of type CALL directly
            if (GET_CODE (parallel) == SET) {
              call = XEXP(parallel, 1);
            } else if (GET_CODE (parallel) == CALL) {
              call = parallel;
            } else {
              printf("ERROR \n");
            }
            
            rtx mem = XEXP(call, 0);
            rtx subExpr = XEXP(mem, 0);
            tree func = 0;

            if (((rtx_code)subExpr->code) == SYMBOL_REF) {
              func = SYMBOL_REF_DECL(subExpr);
            } else  if (((rtx_code)subExpr->code) == CONST) {
              rtx tmp = XEXP(subExpr, 0);
              tmp = XEXP(tmp, 0);
              func = SYMBOL_REF_DECL(tmp);
            } else  if (((rtx_code)subExpr->code) == REG) {
              isDirectCall = false;
            } else {
              printf("ERROR (RTX other - %d)\n", ((rtx_code)subExpr->code));
            }

            if (func != 0) {
              char *fName = (char*)IDENTIFIER_POINTER (DECL_NAME (func) );

              //TODO: Is this a sufficient check for setjmp/longjmp?
              if (strcmp(fName, "setjmp") == 0) {
                instrumentSetJumpFunctionCall(funTree, fName, bb, insn);
                printf("      setjmp \n");
              } else if (strcmp(fName, "longjmp") == 0) {
                instrumentLongJumpFunctionCall(funTree, fName, bb, insn);
                printf("      longjmp \n");
              } else if (strcmp(fName, "printf") == 0 || strcmp(fName, "__builtin_puts") == 0 || strcmp(fName, "modf") == 0) {
                // do nothing
              } else {
                instrumentDirectFunctionCall(funTree, fName, bb, insn);
                printf("      calling function <%s> DIRECTLY with address %p\n", fName, func);
                //printf("%s %s %d: %s\n", LOCATION_FILE(INSN_LOCATION (insn)), function_name, LOCATION_LINE(INSN_LOCATION (insn)), fName);
              }
            } else if (!isDirectCall) {
              instrumentIndirectFunctionCall(LOCATION_FILE(INSN_LOCATION (insn)), function_name, LOCATION_LINE(INSN_LOCATION (insn)), bb, insn);
              printf("      calling function INDIRECTLY \n");
              //printf("%s %s %d\n", LOCATION_FILE(INSN_LOCATION (insn)), function_name, LOCATION_LINE(INSN_LOCATION (insn)));
            }
          } else if (JUMP_P(insn) && strcmp(function_name, "main") != 0) {
            // Don't instrument function return of main
            rtx ret = XEXP(insn, 0);
            ret = PATTERN(insn);
            if (GET_CODE (ret) == PARALLEL) {
              ret = XVECEXP(ret, 0, 0);
              if (ANY_RETURN_P(ret)) {
              instrumentFunctionReturn(funTree, function_name, bb, insn);
              }
            } else if (ANY_RETURN_P(ret)) {
              instrumentFunctionReturn(funTree, function_name, bb, insn);
            }
          }
        }

/*
        if (bb->next_bb == EXIT_BLOCK_PTR_FOR_FN(cfun)) {
          rtx_insn* lastInsn = lastRealINSN(bb);
          instrumentFunctionExit(funTree, function_name, bb, lastInsn);
        } */
      }

      printf("\x1b[92m--------------------- Plugin fully ran -----------------------\n\x1b[0m");
      return 0;
    } catch (const char* e){
      printf("\x1b[91m--------------------- Plugin did not execute completely!  -----------------------\x1b[0m\n\t%s\n", e);
      return 1;
    }
  }

  virtual my_first_pass *clone() override
  {
    // We do not clone ourselves
    return this;
  }
};
}


void read_cfg_file(char * filename) {
  //TODO: set relative path
  std::ifstream input( filename );

  std::string allowed_calls_title = "# allowed calls";
  std::string calls_title = "# calls";

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

int plugin_init(struct plugin_name_args *plugin_info,
                struct plugin_gcc_version *version)
{
  if(!plugin_default_version_check(version, &gcc_version))
  {
    std::cerr << "This GCC plugin is for version " << GCCPLUGIN_VERSION_MAJOR
              << "." << GCCPLUGIN_VERSION_MINOR << "\n";
    return 1;
  }

  for (int i = 0; i < plugin_info->argc; i++) {
    if (std::strcmp(plugin_info->argv[i].key, "cfg_file") == 0) {
      std::cout << "CFG file for instrumentation: " << plugin_info->argv[i].value << "\n";

      read_cfg_file(plugin_info->argv[i].value);
      //print_existing_functions();
      //print_function_call();
    }
  }

  register_callback(plugin_info->base_name,
                    /* event */ PLUGIN_INFO,
                    /* callback */ NULL,
                    /* user_data */
                    &my_gcc_plugin_info);

  // Register the phase right after cfg
  struct register_pass_info pass_info;

  pass_info.pass = new my_first_pass(g);
  pass_info.reference_pass_name = "*free_cfg";
  pass_info.ref_pass_instance_number = 1;
  pass_info.pos_op = PASS_POS_INSERT_AFTER;

  register_callback(plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &pass_info);

  return 0;
}
