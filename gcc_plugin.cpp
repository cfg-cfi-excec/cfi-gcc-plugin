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
	  char *function_name = (char*)IDENTIFIER_POINTER (DECL_NAME (current_function_decl) );
    const char *file_name = LOCATION_FILE(INSN_LOCATION (firstRealINSN(single_succ(ENTRY_BLOCK_PTR_FOR_FN(cfun)))));

    printf("\x1b[92m GCC Plugin executing for function \x1b[92;1m %s \x1b[0m\n",function_name);
    printf("FUNCTION NAME: %s \n", function_name);
    printf("FUNCTION ADDRESS: %p\n", current_function_decl);

    //debug_tree(current_function_decl);

    try{
      basic_block bb;
      bool recursiveFunction = false;

      basic_block firstBb = single_succ(ENTRY_BLOCK_PTR_FOR_FN(cfun));
      rtx_insn* firstInsn = firstRealINSN(firstBb);
      onFunctionEntry(file_name, function_name, LOCATION_LINE(INSN_LOCATION (firstInsn)), firstBb, firstInsn);
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

            //printf("CALL INSTRUCTION in %s : %d \n", file_name, LOCATION_LINE(INSN_LOCATION (insn)));

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
              } else if (strcmp(fName, "strcmp") == 0 || strcmp(fName, "strlen") == 0 || strcmp(fName, "memcpy") == 0  
                  || strcmp(fName, "__builtin_putchar") == 0 || strcmp(fName, "printf") == 0 
                  || strcmp(fName, "__builtin_puts") == 0 || strcmp(fName, "__builtin_memcpy") == 0 
                  || strcmp(fName, "modf") == 0) {
                // do nothing
              } else {
                onDirectFunctionCall(funTree, fName, bb, insn);
                printf("      calling function <%s> DIRECTLY with address %p\n", fName, func);
                //printf("%s %s %d: %s\n", file_name, function_name, LOCATION_LINE(INSN_LOCATION (insn)), fName);
              }
            } else if (!isDirectCall) {
              onIndirectFunctionCall(file_name, function_name, LOCATION_LINE(INSN_LOCATION (insn)), bb, insn);
              printf("      calling function INDIRECTLY \n");
              //printf("%s %s %d\n", file_name, function_name, LOCATION_LINE(INSN_LOCATION (insn)));
            }
          } else if (JUMP_P(insn)) {
            rtx ret = PATTERN(insn);
            if (GET_CODE (ret) == PARALLEL) {
              ret = XVECEXP(ret, 0, 0);
              if (ANY_RETURN_P(ret)) {
              onFunctionReturn(funTree, function_name, bb, insn);
              }
            } else if (ANY_RETURN_P(ret)) {
              onFunctionReturn(funTree, function_name, bb, insn);
            } else if (GET_CODE (ret) == SET && GET_CODE(XEXP(ret,1)) == REG) {
              //It is not possible to extract line number here unfortunately
              onIndirectJump(file_name, function_name, bb, insn);
              printf("      jumping to label INDIRECTLY\n");
            }
            
          } else if (LABEL_P(insn) && LABEL_NAME (insn) != NULL) {             
            rtx_insn *tmp = NEXT_INSN(insn);
            while (NOTE_P(tmp)) {
              tmp = NEXT_INSN(tmp);
            }

            //debug_rtx(insn);
            onNamedLabel(file_name, function_name, LABEL_NAME (insn), bb, tmp);
            printf("      NAMED LABEL found: %s\n", LABEL_NAME (insn));
          }
        }


        if (bb->next_bb == EXIT_BLOCK_PTR_FOR_FN(cfun)) {
          rtx_insn* lastInsn = lastRealINSN(bb);
          onFunctionExit(file_name, function_name, bb, lastInsn);
        } 
      }

      if (recursiveFunction) {
        onFunctionRecursionEntry(file_name, function_name, LOCATION_LINE(INSN_LOCATION (firstInsn)), firstBb, firstInsn);
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

  void GCC_PLUGIN::printFunctionCalls() {
    for(CFG_FUNCTION_CALL function_call : function_calls) {
      std::cout << "CALL (indirect call): " << function_call.function_name << " (" << function_call.file_name << ":" << function_call.line_number << ")" << '\n';
      for(CFG_SYMBOL calls : function_call.calls) {
        std::cout << "    calls: " << calls.symbol_name << " (" << calls.file_name << ")" << '\n';
      }
    }
  }

  void GCC_PLUGIN::printLabelJumps() {
    for(CFG_LABEL_JUMP label_jump : label_jumps) {
      std::cout << "GOTO (indirect jump): " << label_jump.function_name << " (" << label_jump.file_name << ")" << '\n';
      for(CFG_SYMBOL jumps : label_jump.jumps_to) {
        std::cout << "    jumps to: " << label_jump.function_name << ":" << jumps.symbol_name << " (" << jumps.file_name << ")" << '\n';
      }
    }
  }

  int GCC_PLUGIN::getLabelForIndirectJumpSymbol(std::string file_name, std::string function_name, std::string symbol_name) {
    for(CFG_LABEL_JUMP label_jump : label_jumps) {
      if (label_jump.file_name.compare(file_name) == 0) {
        if (label_jump.function_name.compare(function_name) == 0) {
          for(CFG_SYMBOL jumps_to : label_jump.jumps_to) {
            if (jumps_to.symbol_name.compare(symbol_name) == 0) {
              return label_jump.label;
            }
          }
        }
      }
    }

    printf("NOT FOUND (getLabelForIndirectJumpSymbol): %s -- %s -- %s \n\n", function_name.c_str(), symbol_name.c_str(), file_name.c_str());

    return -1;
  }

  int GCC_PLUGIN::getLabelForIndirectJump(std::string file_name, std::string function_name) {
    for(CFG_LABEL_JUMP label_jump : label_jumps) {
      if (label_jump.file_name.compare(file_name) == 0) {
        if (label_jump.function_name.compare(function_name) == 0) {
          return label_jump.label;
        }
      }
    }

    printf("NOT FOUND (getLabelForIndirectJump): %s -- %s \n\n", file_name.c_str(), function_name.c_str());

    return -1;
  }

  int GCC_PLUGIN::getLabelForIndirectlyCalledFunction(std::string function_name, std::string file_name) {
    for(CFG_FUNCTION_CALL function_call : function_calls) {
      if (function_call.file_name.compare(file_name) == 0) {
        if (function_call.function_name.compare(function_name) == 0) {
          return function_call.label;
        }
      }
    }

    printf("NOT FOUND (getLabelForIndirectlyCalledFunction): %s -- %s \n\n", function_name.c_str(), file_name.c_str());

    return -1;
  }

  int GCC_PLUGIN::getLabelForIndirectFunctionCall(std::string function_name, std::string file_name, int line_number) {
    for(CFG_FUNCTION_CALL function_call : function_calls) {
      if (function_call.file_name.compare(file_name) == 0) {
        if (function_call.function_name.compare(function_name) == 0) {
          if (function_call.line_number == line_number) {
            return function_call.label;
          }
        }
      }
    }

    return -1;
  }

  std::vector<CFG_FUNCTION_CALL> GCC_PLUGIN::getIndirectFunctionCalls() {
    return function_calls;
  }

  std::vector<CFG_LABEL_JUMP> GCC_PLUGIN::getIndirectLabelJumps() {
    return label_jumps;
  }

  void GCC_PLUGIN::readConfigFile(char * filename) {
    std::ifstream input( filename );

    std::string calls_title = "# indirect calls";
    std::string jumps_title = "# indirect jumps";

    bool section_calls = false;
    bool section_jumps = false;

    size_t pos = 0;
    std::string token, token_name, token_file, file_name, function_name, line_number_with_offset, offset, line_number, label;
    std::string delimiter = " ";
    std::string delimiter_entry = ":";

    for(std::string line; getline( input, line ); ) {
      pos = 0;
      //std::cout << "LINE READ: " << line << std::endl;

      if (line.find(calls_title) != std::string::npos) {
        section_jumps = false;
        section_calls = true;
      } else if (line.find(jumps_title) != std::string::npos) {
        section_jumps = true;
        section_calls = false;
      } else if (line.length() > 0) {

        if (section_calls) {
          // extract file name
          pos = line.find(delimiter);
          file_name = line.substr(0, pos);
          line.erase(0, pos + delimiter.length());

          // extract function name
          pos = line.find(delimiter);
          function_name = line.substr(0, pos);
          line.erase(0, pos + delimiter.length());

          // extract line_number and offset
          pos = line.find(delimiter);
          line_number_with_offset = line.substr(0, pos);
          line.erase(0, pos + delimiter.length());

          pos = line_number_with_offset.find(":");
          if (pos > 0) {
            line_number = line_number_with_offset.substr(0, pos);
            line_number_with_offset.erase(0, pos + delimiter.length());

            offset = line_number_with_offset;
          } else {
            line_number = line_number_with_offset;
          }

          // extract call label
          pos = line.find(delimiter);
          label = line.substr(0, pos-1);
          line.erase(0, pos + delimiter.length());

          CFG_FUNCTION_CALL cfg_function;
          cfg_function.file_name = file_name;
          cfg_function.function_name = function_name;
          cfg_function.line_number = std::stoi(line_number);
          cfg_function.offset = std::stoi(offset);
          cfg_function.label = std::stoi(label);

          // extract possible function calls
          while ((pos = line.find(delimiter)) != std::string::npos) {
              token = line.substr(0, pos);
              line.erase(0, pos + delimiter.length());

              pos = token.find(delimiter_entry);
              token_file = token.substr(0, pos);
              token.erase(0, pos + delimiter_entry.length());
              token_name = token;

              if (!token_file.empty() && !token_name.empty()) {
                CFG_SYMBOL tmp;
                tmp.file_name = token_file;
                tmp.symbol_name = token_name;
                cfg_function.calls.push_back(tmp);
            }
          }

          if (line.length() > 0) {
              pos = line.find(delimiter_entry);
              token_file = line.substr(0, pos);
              line.erase(0, pos + delimiter_entry.length());
              token_name = line;

              if (!token_file.empty() && !token_name.empty()) {
                CFG_SYMBOL tmp;
                tmp.file_name = token_file;
                tmp.symbol_name = token_name;
                cfg_function.calls.push_back(tmp);
              }
          }

          function_calls.push_back(cfg_function);
        } else if (section_jumps) {

          // extract file name
          pos = line.find(delimiter);
          file_name = line.substr(0, pos);
          line.erase(0, pos + delimiter.length());

          // extract function name
          pos = line.find(delimiter);
          function_name = line.substr(0, pos);
          line.erase(0, pos + delimiter.length());

          // extract jump label
          pos = line.find(delimiter);
          label = line.substr(0, pos-1);
          line.erase(0, pos + delimiter.length());

          CFG_LABEL_JUMP cfg_label_jump;
          cfg_label_jump.file_name = file_name;
          cfg_label_jump.function_name = function_name;
          cfg_label_jump.label = std::stoi(label);

          // extract possible jump targets
          while ((pos = line.find(delimiter)) != std::string::npos) {
              token = line.substr(0, pos);
              line.erase(0, pos + delimiter.length());

              pos = token.find(delimiter_entry);
              token_file = token.substr(0, pos);
              token.erase(0, pos + delimiter_entry.length());
              token_name = token;

              if (!token_file.empty() && !token_name.empty()) {
                CFG_SYMBOL tmp;
                tmp.file_name = token_file;
                tmp.symbol_name = token_name;
                cfg_label_jump.jumps_to.push_back(tmp);
              }
          }

          if (line.length() > 0) {
              pos = line.find(delimiter_entry);
              token_file = line.substr(0, pos);
              line.erase(0, pos + delimiter_entry.length());
              token_name = line;

              if (!token_file.empty() && !token_name.empty()) {
                CFG_SYMBOL tmp;
                tmp.file_name = token_file;
                tmp.symbol_name = token_name;
                cfg_label_jump.jumps_to.push_back(tmp);
              }
          }

          label_jumps.push_back(cfg_label_jump);
        }
      }
    }
  }