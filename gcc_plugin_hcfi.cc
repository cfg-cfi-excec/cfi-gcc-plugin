#include <iostream>
#include <stdio.h>
#include <string.h>

// This is the first gcc header to be included
#include "gcc-plugin.h"
#include "plugin-version.h"
#include <context.h>
#include <basic-block.h>
#include <rtl.h>
#include <tree-pass.h>
#include <tree.h>
#include <print-tree.h>


// We must assert that this plugin is GPL compatible
int plugin_is_GPL_compatible = 1;

static struct plugin_info my_gcc_plugin_info =
{ "1.0", "This is a very simple plugin" };

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

  /**
  * Emits: ADD reg,#number
  */
  rtx_insn* emitAddRegInt(unsigned int regNumber, int number, rtx_insn* attachRtx, basic_block bb, bool after){
    rtx reg = gen_rtx_REG(SImode, regNumber);
    rtx constInt = createConstInt(number);
    rtx add = gen_rtx_PLUS(SImode, reg, constInt);
    rtx set = gen_movsi(reg, add);
    rtx_insn* insn = emitInsn(set, attachRtx, bb, after);
    return insn;
  }

  bool isCall(rtx_insn* expr){
    rtx innerExpr = XEXP(expr, 3);
    return findCode(innerExpr, CALL);
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

  void instrumentFunctionEntry(const tree_node *tree, char *fName, basic_block firstBlock, rtx_insn *firstInsn) {
    //TODO: Set Label propperly
    emitAsmInput("CHECKLABEL 0x42", firstInsn, firstBlock, false);
    printf ("    Generating CHECKLABEL \n");
  }

  void instrumentFunctionExit(const tree_node *tree, char *fName, basic_block lastBlock, rtx_insn *lastInsn) {
    emitAsmInput("CHECKPC", lastInsn, lastBlock, false);
    printf ("    Generating CHECKPC \n");
  }

  void instrumentDirectFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    emitAsmInput("SETPC", insn, block, false);
    printf ("    Generating SETPC \n");
  }

  void instrumentIndirectFunctionCall(const tree_node *tree, char *fName, basic_block block, rtx_insn *insn) {
    //TODO: Set Label propperly
    emitAsmInput("SETPCLABEL 0x42", insn, block, false);
    printf ("    Generating SETPCLABEL \n");
  }

  virtual unsigned int execute(function * fun) override
  {    
    const_tree funTree = get_base_address(current_function_decl);
	  char* funName = (char*)IDENTIFIER_POINTER (DECL_NAME (current_function_decl) );
    printf("\x1b[92m GCC Plugin executing for function \x1b[92;1m %s \x1b[0m\n",funName);

    printf("FUNCTION NAME: %s \n", funName);
    printf ("FUNCTION ADDRESS: %p\n", current_function_decl);

    //debug_tree(current_function_decl);

    try{
      basic_block bb;

      bb = single_succ(ENTRY_BLOCK_PTR_FOR_FN(cfun));
      rtx_insn* firstInsn = firstRealINSN(bb);
      instrumentFunctionEntry(funTree, funName, bb, firstInsn);

      FOR_EACH_BB_FN(bb, cfun){
          
        rtx_insn* insn;
        FOR_BB_INSNS (bb, insn) {

          if (CALL_P (insn) && isCall(insn)) {
            bool isDirectCall = true;
            
            // Get the body of the function call
            rtx body = PATTERN(insn);
            rtx parallel = XVECEXP(body, 0, 0);
            rtx call;

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
              instrumentDirectFunctionCall(funTree, funName, bb, insn);

              const char *fName = (char*)IDENTIFIER_POINTER (DECL_NAME (func) );
              printf("      calling function <%s> DIRECTLY with address %p\n", fName, func);
            } else if (!isDirectCall) {
              instrumentIndirectFunctionCall(funTree, funName, bb, insn);

              printf("      calling function INDIRECTLY \n");
            }
          }
        }

        if (bb->next_bb == EXIT_BLOCK_PTR_FOR_FN(cfun)) {
          rtx_insn* lastInsn = lastRealINSN(bb);
          instrumentFunctionExit(funTree, funName, bb, lastInsn);
        } 
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


int plugin_init(struct plugin_name_args *plugin_info,
                struct plugin_gcc_version *version)
{
  if(!plugin_default_version_check(version, &gcc_version))
  {
    std::cerr << "This GCC plugin is for version " << GCCPLUGIN_VERSION_MAJOR
              << "." << GCCPLUGIN_VERSION_MINOR << "\n";
    return 1;
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
