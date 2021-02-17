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
    return ((char const*[])REGISTER_NAMES)[regno];
  }

  unsigned int GCC_PLUGIN::execute(function * fun) {   
	  char *function_name = (char*)IDENTIFIER_POINTER (DECL_NAME (current_function_decl) );
    char *file_name = (char*)DECL_SOURCE_FILE (current_function_decl);

    if (file_name == NULL) {
      file_name = (char*)"";
    }

    std::cerr << "FUNCTION NAME: " << function_name << std::endl;
    //std::cerr << "FILE NAME: " << file_name << std::endl;
    //std::cerr << "FUNCTION ADDRESS: " << static_cast<void*>(current_function_decl) << std::endl;

    try{
      basic_block bb;
      bool recursiveFunction = false;

      basic_block firstBb = single_succ(ENTRY_BLOCK_PTR_FOR_FN(cfun));
      rtx_insn* firstInsn = UpdatePoint::firstRealINSN(firstBb);

      //std::cerr << LOCATION_FILE(INSN_LOCATION (firstInsn)) << ":" << function_name << std::endl;

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

              // Check whether the function returns a value or not:
              // - If it returns a value, the first element of the body is a SET
              // - Otherwiese it is of type CALL directly
              if (GET_CODE (parallel) == SET) {
                call = XEXP(parallel, 1);
              } else if (GET_CODE (parallel) == CALL) {
                call = parallel;
              } else {
                std::cout << "An error occured..." << std::endl;
                return 1;
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
                std::cout << "An error occured..." << std::endl;
                return 1;
            }

            if (func != 0) {
              char *fName = (char*)IDENTIFIER_POINTER (DECL_NAME (func) );

              if (strcmp(fName, function_name) == 0) {
                recursiveFunction = true;
                onRecursiveFunctionCall(file_name, function_name, bb, insn);
              } else if (strcmp(fName, "setjmp") == 0) {
                onSetJumpFunctionCall(file_name, function_name, bb, insn);
              } else if (strcmp(fName, "longjmp") == 0) {
                onLongJumpFunctionCall(file_name, function_name, bb, insn);
              } else {
                if (GET_CODE (set) != SET) {  
                  // This is a direct JAL
                  onDirectFunctionCall(file_name, fName, bb, insn);
                  //std::cerr << "CALLING directly: " << fName << std::endl;
                }
              }
            } else if (!isDirectCall) {
              onIndirectFunctionCall(file_name, function_name, LOCATION_LINE(INSN_LOCATION (insn)), bb, insn);
              //std::cerr << "CALLING indirectly" << std::endl;
              //std::cerr << "CALLING indirectly (" << file_name << ":" << function_name << ":" << std::to_string(LOCATION_LINE(INSN_LOCATION (insn))) << ")" << std::endl;
            } else if (((rtx_code)subExpr->code) == SYMBOL_REF) {
              // This is a JALR to a shared library function
              if (!isLibGccFunction(XSTR(subExpr, 0))) {
                std::cerr << "Missing exclusion: " << XSTR(subExpr, 0) << std::endl;
                exit(1);
              }

              onDirectFunctionCall(file_name, XSTR(subExpr, 0), bb, insn); 
            }
          } else if (JUMP_P(insn)) {
            rtx ret = PATTERN(insn);
            if (GET_CODE (ret) == PARALLEL) {
              ret = XVECEXP(ret, 0, 0);
              if (ANY_RETURN_P(ret)) {
                onFunctionReturn(file_name, function_name, bb, insn);
              } else if (GET_CODE (ret) == SET && GET_CODE(XEXP(ret,1)) == REG) {
                // indirect jump with jump-table (e.g. from switch statement)
                //std::cerr << "JUMP TABLE FOUND (" << file_name << ":" << function_name << ":" << std::to_string(LOCATION_LINE(INSN_LOCATION (insn))) << ")" << std::endl;

                // instrument switch statement (i.e. the indirect jump)
                int label = onIndirectJumpWithJumpTable(file_name, function_name, LOCATION_LINE(INSN_LOCATION (insn)), bb, insn);

                for (rtx_insn * next = NEXT_INSN (insn); next != NULL; next = NEXT_INSN (next)) {
                  if (GET_CODE (next) == JUMP_TABLE_DATA) {
                    rtx body = PATTERN(next);
                    int len = XVECLEN (body, 0);

                    // iterate all cases of the switch
                    for (int i = 0; i < len; i++) {
                      rtx label_symbol = XVECEXP(body, 0, i);

                      if (GET_CODE (label_symbol) == LABEL_REF) {
                        rtx_insn * code_label = (rtx_insn *)XEXP(label_symbol, 0);
                        onSwitchCase(label, BLOCK_FOR_INSN(code_label), code_label);
                      }
                    }

                    break;
                  }
                }
              }
            } else if (ANY_RETURN_P(ret)) {
              onFunctionReturn(file_name, function_name, bb, insn);
            } else if (GET_CODE (ret) == SET && GET_CODE(XEXP(ret,1)) == REG) {
              //It is not possible to extract line number here unfortunately, so passing -1 as line number
              onIndirectJump(file_name, function_name, bb, insn);
              //std::cerr << "JUMPING to label indirectly: " << std::endl;
            }
            
          } else if (LABEL_P(insn) && LABEL_NAME (insn) != NULL) {             
            rtx_insn *tmp = NEXT_INSN(insn);
            while (NOTE_P(tmp)) {
              tmp = NEXT_INSN(tmp);
            }

            onNamedLabel(file_name, function_name, LABEL_NAME (insn), bb, tmp);
            //std::cerr << "    NAMED LABEL found: " << LABEL_NAME (insn) << std::endl;
          }
        }


        if (bb->next_bb == EXIT_BLOCK_PTR_FOR_FN(cfun)) {
          rtx_insn* lastInsn = UpdatePoint::lastRealINSN(bb);
          onFunctionExit(file_name, function_name, bb, lastInsn);
        } 
      }

      if (recursiveFunction) {
        onFunctionRecursionEntry(file_name, function_name, firstBb, firstInsn);
      } else {
        onFunctionEntry(file_name, function_name, firstBb, firstInsn);
      }

      return 0;
    } catch (const char* e){
      std::cout << "An error occured..." << std::endl;
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
      std::cerr << "Indirectly called function: " << cfg_symbol.symbol_name << " (" << cfg_symbol.file_name << ") with label " << std::to_string(cfg_symbol.label) << '\n';
    }
  }

  void GCC_PLUGIN::printFunctionCalls() {
    for(CFG_FUNCTION_CALL function_call : function_calls) {
      std::cerr << "CALL (indirect call): " << function_call.function_name << " (" << function_call.file_name << ":" << function_call.line_number << ")" << '\n';
      for(CFG_SYMBOL calls : function_call.calls) {
        std::cerr << "    calls: " << calls.symbol_name << " (" << calls.file_name << ")" << '\n';
      }
    }
  }

  void GCC_PLUGIN::printLabelJumps() {
    for(CFG_LABEL_JUMP label_jump : label_jumps) {
      std::cerr << "GOTO (indirect jump): " << label_jump.function_name << " (" << label_jump.file_name << ")" << '\n';
      for(CFG_SYMBOL jumps : label_jump.jumps_to) {
        std::cerr << "    jumps to: " << jumps.file_name << ":" << jumps.function_name << ":" << jumps.symbol_name  << '\n';
      }
    }
  }

  bool GCC_PLUGIN::isFunctionUsedInMultipleIndirectCalls(std::string file_name, std::string function_name) {
    std::vector<CFG_FUNCTION_CALL> calls = getIndirectFunctionCalls();
    unsigned numberOfUses = 0;

    for(CFG_FUNCTION_CALL function_call : calls) {
      //std::cerr << "indirect call: " << function_call.function_name << std::endl;
      for(CFG_SYMBOL function : function_call.calls) {
        //std::cerr << "indirectly called: " << function.symbol_name << std::endl;
        //std::cerr << "current: " << function_name << std::endl;
        if (function.file_name.compare(file_name) == 0 
            && function.symbol_name.compare(function_name) == 0) {
          numberOfUses++;
        }
      }
    }

    return (numberOfUses > 1);
  }

  bool GCC_PLUGIN::areTrampolinesNeeded(std::string file_name, std::string function_name, int line_number) {
    std::vector<CFG_FUNCTION_CALL> function_calls = getIndirectFunctionCalls();

    for(CFG_FUNCTION_CALL function_call : function_calls) {
      if (function_call.function_name.compare(function_name) == 0) {
        if (function_call.line_number == line_number) {
          for(CFG_SYMBOL call : function_call.calls) {
            bool trampolineNeeded = isFunctionUsedInMultipleIndirectCalls(call.file_name, call.symbol_name);
            
            if (trampolineNeeded) {
              // generate trampoline even when only one single function is used within multiple indirect calls
              return true;
            }        
          }
        }
      }
    } 

    return false;
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

    std::cerr << "NOT FOUND (getLabelForIndirectJumpSymbol): " << file_name << ":" << function_name << ":" << symbol_name << std::endl;
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

    //std::cerr << "NO LABEL FOUND (getLabelForIndirectJump): " << file_name << " -- " << function_name << std::endl;
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


    //std::cerr << "NO LABEL FOUND (getLabelForIndirectlyCalledFunction): " << function_name << " -- " << file_name << std::endl;
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

	void GCC_PLUGIN::handleIndirectJumpWithoutConfigEntry(std::string file_name, std::string function_name) {
    std::cerr << "ERROR: No CFI rules for indirect jump found: " << file_name.c_str() << ":" 
      << function_name.c_str() << "\n";
    exit(1);
  }

  /* Some indirect function calls are not CFI-enforced because setup code is excluded from CFI.
     This function checks whether a function should be excluded.
     This is used for functions, where no config-file entry with CFG infos can be found. */
  void GCC_PLUGIN::handleIndirectFunctionCallWithoutConfigEntry(std::string file_name, std::string function_name, int line_number) {
    bool found = false;
    std::vector<std::string> exclusions {
      "__rt_event_execute",
      "__rt_cbsys_exec",
      "__rt_init",
      "__rt_deinit",
      "_prf",
      "perform_attack.constprop",
      "do_ctors",
      "do_dtors"
    };

    for (std::string excl : exclusions) {
      if (function_name.rfind(excl,0) == 0) {
        found = true;
        break;
      }
    }

    if (!found) {
      std::cerr << "ERROR: No CFI rules for indirect call found: " << file_name.c_str() << ":" 
        << function_name.c_str() << ":" << std::to_string( line_number) << "\n";
      exit(1);
    }
  }

  bool GCC_PLUGIN::isExcludedFromBackwardEdgeCfi(std::string function_name) {
    std::vector<std::string> exclusions {
      // This exclusion is required because the caller is in libgcc, e.g. in the function exp,
      // but the callee is in the SDK.
      // Thus, the caller cannot be instrumented.
      "__errno"
    };

    for (std::string excl : exclusions) {
      if (excl.compare(function_name) == 0) {
        //std::cerr << "FOUND EXCLUSION: " << excl << std::endl;
        return true;
      }
    }

    return false;
  }

  // This exclusion list is a (temoporary) fix for functions in libgcc (like soft fp lib and math functions).
  // Those cannot be instrumented because LTO does not work for libgcc.
  bool GCC_PLUGIN::isLibGccFunction(std::string function_name) {
    std::vector<std::string> exclusions {
      "__floatsidf",
      "__adddf3",
      "__fixdfsi",
      "__divdf3",
      "__subdf3",
      "__muldf3",
      "__gedf2",
      "__ltdf2",
      "__ledf2",
      "__multf3",
      "__subtf3",
      "__addtf3",
      "__extenddftf2",
      "__extendsfdf2",
      "__lttf2",
      "__divtf3",
      "__truncdfsf2",
      "__trunctfdf2",
      "__floatunsidf",
      "__eqdf2",
      "__nedf2",
      "__gtdf2",
      "__fixunsdfsi",
      "__clzsi2",
      "__floatunsisf",
      "__divsf3",
      "__ltsf2",
      "__fixunssfsi",
      "__gtsf2",
      "__gesf2",
      "__lesf2",
      "__nesf2",
      "__mulsf2",
      "__mulsf3",
      "__subsf3",
      "__addsf3",
      "__floatsisf",
      "exp",
      "pow",
      "sqrt",
      "cos",
      "sin",
      "acos",
      "tan",
      "atan",
      "fabs",
      "fabsf"
    };

    for (std::string excl : exclusions) {
      if (excl.compare(function_name) == 0) {
        //std::cerr << "FOUND EXCLUSION: " << excl << std::endl;
        return true;
      }
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

    bool section_calls = false;
    bool section_jumps = false;
    bool section_functions = false;

    size_t pos = 0;
    std::string token, token_name, token_file, file_name, function_name, line_number, label;
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

              // extract file name
              pos = token.find(delimiter_entry);
              token_file = token.substr(0, pos);
              token.erase(0, pos + delimiter_entry.length());

              // extract function name
              pos = token.find(delimiter_entry);
              function_name = token.substr(0, pos);
              token.erase(0, pos + delimiter_entry.length());

              token_name = token;

              if (!token_file.empty() && !token_name.empty()) {
                CFG_SYMBOL tmp;
                tmp.file_name = token_file;
                tmp.function_name = function_name;
                tmp.symbol_name = token_name;
                cfg_label_jump.jumps_to.push_back(tmp);
              }
          }

          if (line.length() > 0) {
              // extract file name
              pos = line.find(delimiter_entry);
              token_file = line.substr(0, pos);
              line.erase(0, pos + delimiter_entry.length());

              // extract function name
              pos = line.find(delimiter_entry);
              function_name = line.substr(0, pos);
              line.erase(0, pos + delimiter_entry.length());

              token_name = line;

              if (!token_file.empty() && !token_name.empty()) {
                CFG_SYMBOL tmp;
                tmp.file_name = token_file;
                tmp.function_name = function_name;
                tmp.symbol_name = token_name;
                cfg_label_jump.jumps_to.push_back(tmp);
              }
          }

          label_jumps.push_back(cfg_label_jump);
        }
      }
    }
  }