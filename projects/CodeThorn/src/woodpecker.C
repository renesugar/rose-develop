// Author: Markus Schordan, 2013.

#include "rose.h"

#include "inliner.h"

#include <iostream>
#include "VariableIdMapping.h"
#include "Labeler.h"
#include "AstAnnotator.h"
#include "Miscellaneous.h"
#include "ProgramStats.h"
#include "CommandLineOptions.h"
#include "AnalysisAbstractionLayer.h"
#include "AbstractValue.h"
#include "SgNodeHelper.h"
#include "FIConstAnalysis.h"
#include "TrivialInlining.h"
#include "Threadification.h"
#include "RewriteSystem.h"
#include "Lowering.h"

#include <vector>
#include <set>
#include <list>
#include <string>

#include "SprayException.h"
#include "CodeThornException.h"

#include "limits.h"
#include <cmath>
#include "assert.h"

using namespace std;
using namespace CodeThorn;

#include "Diagnostics.h"
using namespace Sawyer::Message;

#include "PropertyValueTable.h"
#include "DeadCodeElimination.h"
#include "ReachabilityAnalysis.h"

#include "ConversionFunctionsGenerator.h"

//static  VariableIdSet variablesOfInterest;
static bool detailedOutput=0;
const char* csvAssertFileName=0;
const char* csvConstResultFileName=0;
bool global_option_multiconstanalysis=false;

size_t numberOfFunctions(SgNode* node) {
  RoseAst ast(node);
  size_t num=0;
  for(RoseAst::iterator i=ast.begin();i!=ast.end();i++) {
    if(isSgFunctionDefinition(*i))
      num++;
  }
  return num;
}



void printCodeStatistics(SgNode* root) {
  SgProject* project=isSgProject(root);
  VariableIdMapping variableIdMapping;
  variableIdMapping.computeVariableSymbolMapping(project);
  VariableIdSet setOfUsedVars=AnalysisAbstractionLayer::usedVariablesInsideFunctions(project,&variableIdMapping);
  DeadCodeElimination dce;
  cout<<"----------------------------------------------------------------------"<<endl;
  cout<<"Statistics:"<<endl;
  cout<<"Number of empty if-statements: "<<dce.listOfEmptyIfStmts(root).size()<<endl;
  cout<<"Number of functions          : "<<SgNodeHelper::listOfFunctionDefinitions(project).size()<<endl;
  cout<<"Number of global variables   : "<<SgNodeHelper::listOfGlobalVars(project).size()<<endl;
  cout<<"Number of used variables     : "<<setOfUsedVars.size()<<endl;
  cout<<"----------------------------------------------------------------------"<<endl;
  cout<<"VariableIdMapping-size       : "<<variableIdMapping.getVariableIdSet().size()<<endl;
  cout<<"----------------------------------------------------------------------"<<endl;
}

int main(int argc, char* argv[]) {
  ROSE_INITIALIZE;

  Rose::Diagnostics::mprefix->showProgramName(false);
  Rose::Diagnostics::mprefix->showThreadId(false);
  Rose::Diagnostics::mprefix->showElapsedTime(false);

  Sawyer::Message::Facility logger;
  Rose::Diagnostics::initAndRegister(&logger, "Woodpecker");

  try {
    if(argc==1) {
      logger[ERROR] << "wrong command line options."<<endl;
      exit(1);
    }

  // Command line option handling.
#ifdef USE_SAWYER_COMMANDLINE
    namespace po = Sawyer::CommandLine::Boost;
#else
    namespace po = boost::program_options;
#endif

    po::options_description desc
    ("Woodpecker V0.1\n"
     "Written by Markus Schordan\n"
     "Supported options");

  desc.add_options()
    ("help,h", "produce this help message.")
    ("rose-help", "show help for compiler frontend options.")
    ("version,v", "display the version.")
    ("stats", "display code statistics.")
    ("normalize-compound-stmts", po::value< bool >()->default_value(false)->implicit_value(true), "normalize code (eliminate compound assignment operators).")
    ("lowering", po::value< bool >()->default_value(false)->implicit_value(true), "apply lowering to code (eliminates for,while,do-while,continue,break; inlines functions).")
    ("inline", po::value< bool >()->default_value(false)->implicit_value(true), "inlines functions.")
    ("inline-non-param-functions", po::value< bool >()->default_value(false)->implicit_value(true), "inlines only functions that have no return value and no parameters.")
    ("eliminate-empty-if", po::value< bool >()->default_value(false)->implicit_value(true), "eliminate if-statements that have only empty branches.")
    ("eliminate-fi-dead-code", po::value< bool >()->default_value(false)->implicit_value(true), "eliminate flow-insensitive dead variables and dead expressions.")
    ("unparse",po::value< bool >()->default_value(false)->implicit_value(true), "unparse transformed code with prefix \"rose_\".")
    ("verbose", po::value< bool >()->default_value(false)->implicit_value(true), "print detailed output during analysis and transformation.")
    ("csv-assert",po::value< string >(), "name of csv file with reachability assert results'")
    ("csv-const-result",po::value< string >(), "generate csv-file <arg> with const-analysis data.")
    ("fi-const-analysis", po::value< bool >()->default_value(false)->implicit_value(true), "perform flow-insensitive const analysis.")
    ("enable-multi-const-analysis", po::value< bool >()->default_value(false)->implicit_value(true), "enable multi-const analysis.")
    ("generate-conversion-functions","generate code for conversion functions between variable names and variable addresses.")
    ("transform-thread-variable", "transform code to use additional thread variable.")
    ("log-level",po::value< string >()->default_value("none,>=warn"),"Set the log level (\"x,>=y\" with x,y in: (none|info|warn|trace|debug)).")
    ;
  //    ("int-option",po::value< int >(),"option info")


  po::store(po::command_line_parser(argc, argv).
            options(desc).allow_unregistered().run(), args);
  po::notify(args);

  if (args.count("help")) {
    cout << "woodpecker <filename> [OPTIONS]"<<endl;
    cout << desc << "\n";
    exit(0);
  }
  if (args.count("rose-help")) {
    argv[1] = strdup("--help");
  }

  if (args.count("version")) {
    cout << "Woodpecker version 0.1\n";
    cout << "Written by Markus Schordan 2013\n";
    exit(0);
  }
  if (args.count("csv-assert")) {
    csvAssertFileName=args["csv-assert"].as<string>().c_str();
  }
  if (args.count("csv-const-result")) {
    csvConstResultFileName=args["csv-const-result"].as<string>().c_str();
  }

  if(args.getBool("verbose"))
    detailedOutput=1;

  mfacilities.control(args["log-level"].as<string>());
  logger[TRACE] << "Log level is " << args["log-level"].as<string>() << endl;

  // clean up string-options in argv
  for (int i=1; i<argc; ++i) {
    if (string(argv[i]) == "--csv-assert"
        || string(argv[i]) == "--csv-const-result"
        ) {
      // do not confuse ROSE frontend
      argv[i] = strdup("");
      assert(i+1<argc);
        argv[i+1] = strdup("");
    }
  }

  global_option_multiconstanalysis=args.getBool("enable-multi-const-analysis");

  logger[TRACE] << "INIT: Parsing and creating AST started."<<endl;
  SgProject* root = frontend(argc,argv);
  //  AstTests::runAllTests(root);
  // inline all functions
  logger[TRACE] << "INIT: Parsing and creating AST finished."<<endl;

  if(args.getBool("lowering")) {
    logger[TRACE] <<"STATUS: Lowering started."<<endl;
    SPRAY::Lowering lowering;
    lowering.runLowering(root);
    logger[TRACE] <<"STATUS: Lowering finished."<<endl;
  }

  VariableIdMapping variableIdMapping;
  variableIdMapping.computeVariableSymbolMapping(root);

  if(args.count("transform-thread-variable")) {
    Threadification* threadTransformation=new Threadification(&variableIdMapping);
    threadTransformation->transform(root);
    root->unparse(0,0);
    delete threadTransformation;
    logger[TRACE] <<"STATUS: generated program with introduced thread-variable."<<endl;
    exit(0);
  }

  SgFunctionDefinition* mainFunctionRoot=0;
  if(args.getBool("inline-non-param-functions")) {
    logger[TRACE] <<"STATUS: eliminating non-called non-param functions."<<endl;
    // inline functions
    TrivialInlining tin;
    tin.setDetailedOutput(detailedOutput);
    tin.inlineFunctions(root);
    DeadCodeElimination dce;
    // eliminate non called functions
    int numEliminatedFunctions=dce.eliminateNonCalledTrivialFunctions(root);
    logger[TRACE] <<"STATUS: eliminated "<<numEliminatedFunctions<<" functions."<<endl;
  } else {
    logger[INFO] <<"inlining of non-param functions: turned off."<<endl;
  }

  if(args.getBool("eliminate-empty-if")) {
    DeadCodeElimination dce;
    logger[TRACE] <<"STATUS: Eliminating empty if-statements."<<endl;
    size_t num=0;
    size_t numTotal=num;
    do {
      num=dce.eliminateEmptyIfStmts(root);
      logger[INFO] <<"Number of if-statements eliminated: "<<num<<endl;
      numTotal+=num;
    } while(num>0);
    logger[TRACE] <<"STATUS: Total number of empty if-statements eliminated: "<<numTotal<<endl;
  }

  if(args.getBool("normalize-compound-stmts")) {
    logger[TRACE] <<"STATUS: normalization of compound statements started."<<endl;
    RewriteSystem rewriteSystem;
    rewriteSystem.resetStatistics();
    rewriteSystem.rewriteCompoundAssignmentsInAst(root,&variableIdMapping);
    logger[TRACE] <<"STATUS: normalization of compound statements finished."<<endl;
  }

  if(args.getBool("eliminate-fi-dead-code")) {
    FIConstAnalysis fiConstAnalysis(&variableIdMapping);
    DeadCodeElimination dce;
    cout<<"STATUS: Performing dead code elimination."<<endl;
    dce.setDetailedOutput(detailedOutput);
    fiConstAnalysis.runAnalysis(root, mainFunctionRoot);
    VariableIdSet variablesOfInterest;
    variablesOfInterest=fiConstAnalysis.determinedConstantVariables();
    VariableConstInfo vci=*(fiConstAnalysis.getVariableConstInfo());
    dce.setVariablesOfInterest(variablesOfInterest);
    dce.eliminateDeadCodePhase1(root,&variableIdMapping,vci);
    cout<<"DCE: Eliminated "<<dce.numElimVars()<<" variable declarations."<<endl;
    cout<<"DCE: Eliminated "<<dce.numElimAssignments()<<" variable assignments."<<endl;
    cout<<"DCE: Replaced "<<dce.numElimVarUses()<<" uses of variables with constant."<<endl;
    cout<<"DCE: Eliminated "<<dce.numElimVars()<<" dead variables."<<endl;
    cout<<"DCE: Dead code elimination finished."<<endl;
  } else {
    //cout<<"STATUS: Dead code elimination: turned off."<<endl;
  }

  if(args.getBool("fi-const-analysis")) {
    logger[TRACE] <<"STATUS: performing flow-insensitive const analysis."<<endl;
    FIConstAnalysis fiConstAnalysis(&variableIdMapping);
    VarConstSetMap varConstSetMap;
    VariableIdSet variablesOfInterest;
    fiConstAnalysis.setOptionMultiConstAnalysis(global_option_multiconstanalysis);
    fiConstAnalysis.setDetailedOutput(detailedOutput);
    fiConstAnalysis.runAnalysis(root, mainFunctionRoot);
    if(detailedOutput)
      FIConstAnalysis::printResult(variableIdMapping,varConstSetMap);
    variablesOfInterest=fiConstAnalysis.determinedConstantVariables();
    cout <<"constant variables: "<<variablesOfInterest.size()<<endl;
  }

  if(csvConstResultFileName) {
    FIConstAnalysis fiConstAnalysis(&variableIdMapping);
    fiConstAnalysis.setOptionMultiConstAnalysis(global_option_multiconstanalysis);
    fiConstAnalysis.runAnalysis(root, mainFunctionRoot);
    VariableIdSet setOfUsedVarsInFunctions=AnalysisAbstractionLayer::usedVariablesInsideFunctions(root,&variableIdMapping);
    VariableIdSet setOfUsedVarsGlobalInit=AnalysisAbstractionLayer::usedVariablesInGlobalVariableInitializers(root,&variableIdMapping);
    VariableIdSet setOfAllUsedVars = setOfUsedVarsInFunctions;
    setOfAllUsedVars.insert(setOfUsedVarsGlobalInit.begin(), setOfUsedVarsGlobalInit.end());
    logger[INFO]<<"number of used vars inside functions: "<<setOfUsedVarsInFunctions.size()<<endl;
    logger[INFO]<<"number of used vars in global initializations: "<<setOfUsedVarsGlobalInit.size()<<endl;
    logger[INFO]<<"number of vars inside functions or in global inititializations: "<<setOfAllUsedVars.size()<<endl;
    fiConstAnalysis.filterVariables(setOfAllUsedVars);
    fiConstAnalysis.writeCvsConstResult(variableIdMapping, string(csvConstResultFileName));
  }

  if(args.count("generate-conversion-functions")) {
    ConversionFunctionsGenerator gen;
    gen.generateFile(root,"conversionFunctions.C");
    return 0;
  }

  if(csvAssertFileName) {
    cout<<"STATUS: performing flow-insensensitive condition-const analysis."<<endl;
    Labeler labeler(root);
    FIConstAnalysis fiConstAnalysis(&variableIdMapping);
    fiConstAnalysis.setOptionMultiConstAnalysis(global_option_multiconstanalysis);
    fiConstAnalysis.runAnalysis(root, mainFunctionRoot); // is this required for conditionConstAnalysis?
    fiConstAnalysis.performConditionConstAnalysis(&labeler);
    logger[INFO]<<"Number of true-conditions     : "<<fiConstAnalysis.getTrueConditions().size()<<endl;
    logger[INFO]<<"Number of false-conditions    : "<<fiConstAnalysis.getFalseConditions().size()<<endl;
    logger[INFO]<<"Number of non-const-conditions: "<<fiConstAnalysis.getNonConstConditions().size()<<endl;
    logger[TRACE]<<"STATUS: performing flow-insensensitive reachability analysis."<<endl;
    ReachabilityAnalysis ra;
    PropertyValueTable reachabilityResults=ra.fiReachabilityAnalysis(labeler, fiConstAnalysis);
    logger[TRACE]<<"STATUS: generating file "<<csvAssertFileName<<endl;
    reachabilityResults.writeFile(csvAssertFileName,true);
  }
  logger[INFO]<< "Remaining functions in program: "<<numberOfFunctions(root)<<endl;

  if(args.getBool("unparse")) {
    logger[TRACE]<< "STATUS: unparsing - generating transformed source code."<<endl;
    root->unparse(0,0);
  }

  if(args.count("stats")) {
    printCodeStatistics(root);
  }

  logger[TRACE]<< "STATUS: finished."<<endl;

  // main function try-catch
  } catch(CodeThorn::Exception& e) {
    cerr << "CodeThorn::Exception raised: " << e.what() << endl;
    mfacilities.shutdown();
    return 1;
  } catch(SPRAY::Exception& e) {
    cerr << "Spray::Exception raised: " << e.what() << endl;
    mfacilities.shutdown();
    return 1;
  } catch(std::exception& e) {
    cerr << "std::exception raised: " << e.what() << endl;
    mfacilities.shutdown();
    return 1;
  } catch(char* str) {
    cerr << "*Exception raised: " << str << endl;
    mfacilities.shutdown();
    return 1;
  } catch(const char* str) {
    cerr << "Exception raised: " << str << endl;
    mfacilities.shutdown();
    return 1;
  } catch(string str) {
    cerr << "Exception raised: " << str << endl;
    mfacilities.shutdown();
    return 1;
  }
  mfacilities.shutdown();
  return 0;
}
