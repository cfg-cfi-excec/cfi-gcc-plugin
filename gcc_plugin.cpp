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
              } else if (strcmp(fName, "printf") == 0 || strcmp(fName, "__builtin_puts") == 0 || strcmp(fName, "modf") == 0) {
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
            }
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
