#include "gcc_plugin.h"

GCC_PLUGIN::GCC_PLUGIN(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
		: rtl_opt_pass(cfi_plugin_pass_data, ctxt)
{
		argc = argcounter;
		argv = arguments;
}


  // Credits to https://github.com/MGroupKULeuvenBrugesCampus/CFED_Plugin for these functions

  /**
  * Method to find the provided rtx_code in the given
  * rtx. Is a recursive method to make sure all fields of
  * the provided rtx is examined.
  */
  bool GCC_PLUGIN::findCode(rtx expr, rtx_code code){
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
  * Actually emits the codelabel at the desired place
  */
  rtx_insn* GCC_PLUGIN::emitLabel(rtx label, rtx_insn* attachRtx, bool after){
    if(after){
      return emit_label_after(label, attachRtx);
    }
    else{
      return emit_label_before(label, attachRtx);
    }
  }

  /*
  * Actually emits the insn at the desired place.
  */
  rtx_insn* GCC_PLUGIN::emitInsn(rtx rtxInsn,rtx_insn* attachRtx, basic_block bb, bool after){
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
  rtx_insn* GCC_PLUGIN::firstRealINSN(basic_block bb){
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
  rtx_insn* GCC_PLUGIN::lastRealINSN(basic_block bb){
    rtx_insn* lastInsn = BB_END(bb);
    while(!NONDEBUG_INSN_P(lastInsn)){
      lastInsn = PREV_INSN(lastInsn);
    }
    return lastInsn;
  }

  /**
  * Emits the provided assembly instruction
  */
  rtx_insn* GCC_PLUGIN::emitAsmInput(const char* asmInstr, rtx_insn* attachRtx, basic_block bb, bool after){
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
  * Emits: .codeLabel
  */
  rtx_insn* GCC_PLUGIN::emitCodeLabel(unsigned int insnID, rtx_insn* attachRtx, basic_block bb, bool after){
    //rtx_insn* next = NEXT_INSN(attachRtx);
    rtx codeLab = gen_label_rtx();
    rtx_insn* insn = emitLabel(codeLab, attachRtx, after);
    return insn;
  }

  /**
  * Method to easily create a CONST_INT rtx
  */
  rtx GCC_PLUGIN::createConstInt(int number){
    rtx constInt = rtx_alloc(CONST_INT);
    XWINT(constInt,0) = number;
    return constInt;
  }

  bool GCC_PLUGIN::isCall(rtx_insn* expr){
    rtx innerExpr = XEXP(expr, 3);
    return findCode(innerExpr, CALL);
  }
  
  /**
  * Method to determine whether or not the provided
  * rtx_insn is a return instruction
  */
  bool GCC_PLUGIN::isReturn(rtx_insn* expr){
    rtx innerExpr = XEXP(expr, 3);
    bool ret =  findCode(innerExpr, RETURN);
    bool simpRet = findCode(innerExpr, SIMPLE_RETURN);
    return (ret || simpRet);
  }

  void GCC_PLUGIN::printRtxClass(rtx_code code) {
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

  unsigned int GCC_PLUGIN::execute(function * fun) {    
    const_tree funTree = get_base_address(current_function_decl);
	  char* function_name = (char*)IDENTIFIER_POINTER (DECL_NAME (current_function_decl) );
    printf("\x1b[92m GCC Plugin executing for function \x1b[92;1m %s \x1b[0m\n",function_name);

    printf("FUNCTION NAME: %s \n", function_name);
    printf("FUNCTION ADDRESS: %p\n", current_function_decl);

    //debug_tree(current_function_decl);

    try{
      basic_block bb;
      bool recursiveFunction = false;

      basic_block firstBb = single_succ(ENTRY_BLOCK_PTR_FOR_FN(cfun));
      rtx_insn* firstInsn = firstRealINSN(firstBb);
      onFunctionEntry(LOCATION_FILE(INSN_LOCATION (firstInsn)), function_name, LOCATION_LINE(INSN_LOCATION (firstInsn)), firstBb, firstInsn);
      //printf("%s %s ###: \n", LOCATION_FILE(INSN_LOCATION (firstInsn)), function_name);

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
              if (strcmp(fName, function_name) == 0) {
                recursiveFunction = true;
                onRecursiveFunctionCall(funTree, fName, bb, insn);
                //printf("      onRecursiveFunctionCall \n");
              } else if (strcmp(fName, "setjmp") == 0) {
                onSetJumpFunctionCall(funTree, fName, bb, insn);
                printf("      setjmp \n");
              } else if (strcmp(fName, "longjmp") == 0) {
                onLongJumpFunctionCall(funTree, fName, bb, insn);
                printf("      longjmp \n");

                //TODO: Find better way to exclude library functions
              } else if (strcmp(fName, "strcmp") == 0 || strcmp(fName, "strlen") == 0 || strcmp(fName, "memcpy") == 0  || strcmp(fName, "__builtin_putchar") == 0
                  || strcmp(fName, "printf") == 0 || strcmp(fName, "__builtin_puts") == 0 || strcmp(fName, "modf") == 0) {
                // do nothing
              } else {
                onDirectFunctionCall(funTree, fName, bb, insn);
                printf("      calling function <%s> DIRECTLY with address %p\n", fName, func);
                //printf("%s %s %d: %s\n", LOCATION_FILE(INSN_LOCATION (insn)), function_name, LOCATION_LINE(INSN_LOCATION (insn)), fName);
              }
            } else if (!isDirectCall) {
              onIndirectFunctionCall(LOCATION_FILE(INSN_LOCATION (insn)), function_name, LOCATION_LINE(INSN_LOCATION (insn)), bb, insn);
              printf("      calling function INDIRECTLY \n");
              //printf("%s %s %d\n", LOCATION_FILE(INSN_LOCATION (insn)), function_name, LOCATION_LINE(INSN_LOCATION (insn)));
            }
          } else if (JUMP_P(insn)) {
            rtx ret = XEXP(insn, 0);
            ret = PATTERN(insn);
            if (GET_CODE (ret) == PARALLEL) {
              ret = XVECEXP(ret, 0, 0);
              if (ANY_RETURN_P(ret)) {
              onFunctionReturn(funTree, function_name, bb, insn);
              }
            } else if (ANY_RETURN_P(ret)) {
              onFunctionReturn(funTree, function_name, bb, insn);
            } else if (GET_CODE (ret) == SET) {
              rtx reg = XEXP(PATTERN(insn), 1);
              if (((rtx_code)reg->code) == REG) {
                onIndirectJump(funTree, function_name, bb, insn);
                //debug_rtx(insn);
                //debug_rtx(REG_NOTES(reg));
                //printf("INDIRECT JUMP: %s\n\n", XSTR(reg, 0));
                //printf("INDIRECT JUMP: %s %d\n\n", LOCATION_FILE(INSN_LOCATION ((insn))), LOCATION_LINE(INSN_LOCATION ((insn))));
              }
            }
          } else if (LABEL_P(insn) && LABEL_NAME (insn) != NULL) {             
            rtx_insn *tmp = NEXT_INSN(insn);
            while (NOTE_P(tmp)) {
              tmp = NEXT_INSN(tmp);
            }

            //debug_rtx(insn);
            onNamedLabel(funTree, function_name, bb, tmp);
            //printf("LABEL: %s %d\n", LOCATION_FILE(INSN_LOCATION (insn)), LOCATION_LINE(INSN_LOCATION (insn)));
            //printf("LABEL: %s %d\n\n", LOCATION_FILE(INSN_LOCATION (tmp)), LOCATION_LINE(INSN_LOCATION (tmp)));
          }
        }

/*
        if (bb->next_bb == EXIT_BLOCK_PTR_FOR_FN(cfun)) {
          rtx_insn* lastInsn = lastRealINSN(bb);
          onFunctionExit(funTree, function_name, bb, lastInsn);
        } */
      }

      if (recursiveFunction) {
        onFunctionRecursionEntry(LOCATION_FILE(INSN_LOCATION (firstInsn)), function_name, LOCATION_LINE(INSN_LOCATION (firstInsn)), firstBb, firstInsn);
      }

      printf("\x1b[92m--------------------- Plugin fully ran -----------------------\n\x1b[0m");
      return 0;
    } catch (const char* e){
      printf("\x1b[91m--------------------- Plugin did not execute completely!  -----------------------\x1b[0m\n\t%s\n", e);
      return 1;
    }
  }

  void GCC_PLUGIN::writeLabelToTmpFile(unsigned label) {
    clearTmpFile();

    std::ofstream tmp ("tmp.txt");
    tmp << std::to_string(label) << "\n";
    tmp.close();
  }

  unsigned GCC_PLUGIN::readLabelFromTmpFile() {
    std::ifstream tmp ("tmp.txt");
    std::string label;
    std::getline(tmp, label);
    tmp.close();

    return atoi(label.c_str());
  }

  void GCC_PLUGIN::clearTmpFile() {
    std::ofstream tmp ("tmp.txt", std::ofstream::out | std::ofstream::trunc);
    tmp.close();
  }

  void GCC_PLUGIN::prinExistingFunctions() {
    for(CFG_EXISTING_FUNCTION existing_function : existing_functions) {
      std::cout << "FUNCTION: " << existing_function.function_name << " (" << existing_function.file_name << ")" << '\n';
      for(CFG_FUNCTION called_by : existing_function.called_by) {
        std::cout << "    called by: " << called_by.function_name << " (" << called_by.file_name << ")" << '\n';
      }
    }
  }

  void GCC_PLUGIN::printFunctionCalls() {
    for(CFG_FUNCTION_CALL function_call : function_calls) {
      std::cout << "FUNCTION: " << function_call.function_name << " (" << function_call.file_name << ":" << function_call.line_number << ")" << '\n';
      for(CFG_FUNCTION calls : function_call.calls) {
        std::cout << "    calls: " << calls.function_name << " (" << calls.file_name << ")" << '\n';
      }
    }
  }

  int GCC_PLUGIN::getLabelForExistingFunction(std::string function_name, std::string file_name) {
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

  int GCC_PLUGIN::getLabelForFunctionCall(std::string function_name, std::string file_name, int line_number) {
    for(CFG_FUNCTION_CALL function_call : function_calls) {
      if (function_call.file_name.compare(file_name) == 0) {
        if (function_call.function_name.compare(function_name) == 0) {
          if (function_call.line_number == line_number) {
            // possible call targets for this call in function_call.calls
            if (function_call.calls.size() > 0) {
              // assume all functions in calls vector have the same label
              CFG_FUNCTION tmp = function_call.calls.front();

              return getLabelForExistingFunction(tmp.function_name, tmp.file_name);
            }
          }
        }
      }
    }

    return -1;
  }

  void GCC_PLUGIN::readConfigFile(char * filename) {
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