#include "gcc_plugin.h"

GCC_PLUGIN::GCC_PLUGIN(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
		: rtl_opt_pass(cfi_plugin_pass_data, ctxt)
{
		argc = argcounter;
		argv = arguments;
}

	rtx_insn* GCC_PLUGIN::generateAndEmitAsm(std::string insn, rtx_insn* attachRtx, basic_block bb, bool after) {
    if (!insn.empty()){
      char *buff = new char[insn.size()+1];
      std::copy(insn.begin(), insn.end(), buff);
      buff[insn.size()] = '\0';
      return AsmGen::emitAsmInput(buff, attachRtx, bb, after);
    }

    return attachRtx;
  }

  basic_block GCC_PLUGIN::lastRealBlockInFunction() {
    return EXIT_BLOCK_PTR_FOR_FN(cfun)->prev_bb;
  }

  std::string GCC_PLUGIN::getRegisterNameForNumber(unsigned regno) {
    switch (regno) {
      case  0: return "zero";
      case  1: return "ra";
      case  2: return "sp";
      case  3: return "gp";
      case  4: return "tp";
      case  5: return "t0";
      case  6: return "t1";
      case  7: return "t2";
      case  8: return "fp";
      case  9: return "s1";
      case 10: return "a0";
      case 11: return "a1";
      case 12: return "a2";
      case 13: return "a3";
      case 14: return "a4";
      case 15: return "a5";
      case 16: return "a6";
      case 17: return "a7";
      case 18: return "s2";
      case 19: return "s3";
      case 20: return "s4";
      case 21: return "s5";
      case 22: return "s6";
      case 23: return "s7";
      case 24: return "s8";
      case 25: return "s9";
      case 26: return "s10";
      case 27: return "s11";
      case 28: return "t3";
      case 29: return "t4";
      case 30: return "t5";
      case 31: return "t6";
      default: return NULL;
    };
  }

  unsigned int GCC_PLUGIN::execute(function * fun) {   
	  char *function_name = (char*)IDENTIFIER_POINTER (DECL_NAME (current_function_decl) );
    char *file_name = (char*)DECL_SOURCE_FILE (current_function_decl);

    if (file_name == NULL) {
      file_name = (char*)"";
    }

   // printf("\x1b[92m GCC Plugin executing for function \x1b[92;1m %s \x1b[0m\n",function_name);
    std::cerr << "FUNCTION NAME: " << function_name << std::endl;
    //std::cerr << "FILE NAME: " << file_name << std::endl;
    //printf("FUNCTION ADDRESS: %p\n", current_function_decl);

    //debug_tree(current_function_decl);

    try{
      basic_block bb;
      bool recursiveFunction = false;

      basic_block firstBb = single_succ(ENTRY_BLOCK_PTR_FOR_FN(cfun));
      rtx_insn* firstInsn = UpdatePoint::firstRealINSN(firstBb);

      onFunctionEntry(file_name, function_name, firstBb, firstInsn);
      //printf("%s %s ###: \n", LOCATION_FILE(INSN_LOCATION (firstInsn)), function_name);

      FOR_EACH_BB_FN(bb, cfun){
          
        rtx_insn* insn;
        FOR_BB_INSNS (bb, insn) {
          if (CALL_P (insn) && InstrType::findCode(XEXP(insn, 3), CALL)) {
            bool isDirectCall = true;
            rtx call;
            
            // Get the body of the function call
            rtx body = PATTERN(insn);
            rtx set = body;          
            
            if (GET_CODE (set) == SET) {              
              call = XEXP(set, 1);;
            } else if (GET_CODE (set) == CALL) {              
              call = set;
            } else {
              rtx parallel = XVECEXP(body, 0, 0);
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
                onRecursiveFunctionCall(file_name, function_name, bb, insn);
                //printf("      onRecursiveFunctionCall \n");
              } else if (strcmp(fName, "setjmp") == 0) {
                onSetJumpFunctionCall(file_name, function_name, bb, insn);
                printf("      setjmp \n");
              } else if (strcmp(fName, "longjmp") == 0) {
                onLongJumpFunctionCall(file_name, function_name, bb, insn);
                printf("      longjmp \n");
              } else {
                // This is a direct JAL
                onDirectFunctionCall(file_name, fName, bb, insn);
                //std::cerr << "CALLING: " << fName << std::endl;
                //printf("      calling function <%s> DIRECTLY with address %p\n", fName, func);
                //printf("%s %s %d: %s\n", file_name, function_name, LOCATION_LINE(INSN_LOCATION (insn)), fName);
              }
            } else if (!isDirectCall) {
              onIndirectFunctionCall(file_name, function_name, LOCATION_LINE(INSN_LOCATION (insn)), bb, insn);
              //printf("      calling function INDIRECTLY \n");
              //printf("%s %s %d\n", file_name, function_name, LOCATION_LINE(INSN_LOCATION (insn)));
            } else if (((rtx_code)subExpr->code) == SYMBOL_REF) {
              // This is a direct JALR
              //std::cerr << "CALLING: " << XSTR(subExpr, 0) << std::endl;
              onDirectFunctionCall(file_name, XSTR(subExpr, 0), bb, insn); 
            }
          } else if (JUMP_P(insn)) {
            rtx ret = PATTERN(insn);
            if (GET_CODE (ret) == PARALLEL) {
              ret = XVECEXP(ret, 0, 0);
              if (ANY_RETURN_P(ret)) {
              onFunctionReturn(file_name, function_name, bb, insn);
              }
            } else if (ANY_RETURN_P(ret)) {
              onFunctionReturn(file_name, function_name, bb, insn);
            } else if (GET_CODE (ret) == SET && GET_CODE(XEXP(ret,1)) == REG) {
              //It is not possible to extract line number here unfortunately
              onIndirectJump(file_name, function_name, bb, insn);
              //printf("      jumping to label INDIRECTLY\n");
            }
            
          } else if (LABEL_P(insn) && LABEL_NAME (insn) != NULL) {             
            rtx_insn *tmp = NEXT_INSN(insn);
            while (NOTE_P(tmp)) {
              tmp = NEXT_INSN(tmp);
            }

            //debug_rtx(insn);
            onNamedLabel(file_name, function_name, LABEL_NAME (insn), bb, tmp);
            //printf("      NAMED LABEL found: %s\n", LABEL_NAME (insn));
          }
        }


        if (bb->next_bb == EXIT_BLOCK_PTR_FOR_FN(cfun)) {
          rtx_insn* lastInsn = UpdatePoint::lastRealINSN(bb);
          onFunctionExit(file_name, function_name, bb, lastInsn);
        } 
      }

      if (recursiveFunction) {
        onFunctionRecursionEntry(file_name, function_name, firstBb, firstInsn);
      }

      //printf("\x1b[92m--------------------- Plugin fully ran -----------------------\n\x1b[0m");
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

  void GCC_PLUGIN::printIndirectlyCalledFunctions() {
    for(CFG_SYMBOL cfg_symbol : indirectly_called_functions) {
      std::cout << "Indirectly called function: " << cfg_symbol.symbol_name << " (" << cfg_symbol.file_name << ") with label " << std::to_string(cfg_symbol.label) << '\n';
    }
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

    //printf("NO LABEL FOUND (getLabelForIndirectJump): %s -- %s \n\n", file_name.c_str(), function_name.c_str());

    return -1;
  }

  int GCC_PLUGIN::getLabelForIndirectlyCalledFunction(std::string function_name, std::string file_name) {
    for(CFG_SYMBOL cfg_symbol : indirectly_called_functions) {
      if (cfg_symbol.file_name.compare(file_name) == 0) {
        if (cfg_symbol.symbol_name.compare(function_name) == 0) {
          return cfg_symbol.label;
        }
      }
    }


    //printf("NO LABEL FOUND (getLabelForIndirectlyCalledFunction): %s -- %s \n\n", function_name.c_str(), file_name.c_str());

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

  // TODO: remove exclusion list here (tmp fix for soft fp lib functions)
  bool GCC_PLUGIN::isFunctionExcludedFromCFI(std::string function_name) {
    std::vector<std::string> exclusions {
      "__floatsidf",
      "__adddf3",
      "__fixdfsi",
      "__divdf3",
      "__subdf3",
      "__muldf3",
      "__gedf2",
      "__ltdf2",
      "__ledf2"
    };

    if (std::find(std::begin(exclusions), std::end(exclusions), function_name) != std::end(exclusions)) {
      return true;
    }

    return false;
  }

  void GCC_PLUGIN::readConfigFile(char * filename) {
    std::ifstream input( filename );

    if(input.fail()){
      //std::cerr << "CFG file " << filename << " does not exist\n";
    }

    std::string indirectly_called_functions_title = "# indirectly called functions";
    std::string calls_title = "# indirect calls";
    std::string jumps_title = "# indirect jumps";
    std::string attr_line = "line:";
    std::string attr_label = "label:";
    std::string attr_offset = "offset:";

    bool section_calls = false;
    bool section_jumps = false;
    bool section_functions = false;

    size_t pos = 0;
    std::string token, token_name, token_file, file_name, function_name, line_number, label, offset;
    std::string delimiter = " ";
    std::string delimiter_entry = ":";

    for(std::string line; getline( input, line ); ) {
      pos = 0;
      //std::cout << "LINE READ: " << line << std::endl;

      if (line.find(calls_title) != std::string::npos) {
        section_jumps = false;
        section_functions = false;
        section_calls = true;
      } else if (line.find(jumps_title) != std::string::npos) {
        section_jumps = true;
        section_functions = false;
        section_calls = false;
      } else if (line.find(indirectly_called_functions_title) != std::string::npos) {
        section_jumps = false;
        section_functions = true;
        section_calls = false;
      } else if (line.length() > 0) {

        if (section_functions) {
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
          pos = label.find(attr_label);
          label.erase(pos, attr_label.length());

          CFG_SYMBOL cfg_symbol;
          cfg_symbol.file_name = file_name;
          cfg_symbol.symbol_name = function_name;
          cfg_symbol.label = std::stoi(label);

          indirectly_called_functions.push_back(cfg_symbol);
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
          line_number = line.substr(0, pos);
          line.erase(0, pos + delimiter.length());
          pos = line_number.find(attr_line);
          line_number.erase(pos, attr_line.length());

          // extract offset in function
          pos = line.find(delimiter);
          offset = line.substr(0, pos);
          line.erase(0, pos + delimiter.length());
          pos = offset.find(attr_offset);
          offset.erase(pos, attr_offset.length());

          // extract call label
          pos = line.find(delimiter);
          label = line.substr(0, pos-1);
          line.erase(0, pos + delimiter.length());
          pos = label.find(attr_label);
          label.erase(pos, attr_label.length());

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
          pos = label.find(attr_label);
          label.erase(pos, attr_label.length());

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