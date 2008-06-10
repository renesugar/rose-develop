/******************************************
 * Category: DFA
 * DefUse Analysis Declaration
 * created by tps in Feb 2007
 *****************************************/

#ifndef __DefUseAnalysis_HXX_LOADED__
#define __DefUseAnalysis_HXX_LOADED__
#include <string>

#include "filteredCFG.h"
#include "DFAnalysis.h"
#include "support.h"
#include "DFAFilter.h"

class DefUseAnalysis : public DFAnalysis, Support {
 private:
  SgProject* project;
  bool DEBUG_MODE;
  bool DEBUG_MODE_EXTRA;
  std::vector<SgInitializedName*> globalVarList;

  // def-use-specific --------------------
  typedef std::multimap < SgInitializedName* , SgNode* > multitype;
  typedef std::map< SgNode* , multitype > tabletype;
  typedef std::map< SgNode* , int > convtype;
  //typedef std::multimap< SgNode* , SgInitializedName* > ideftype;

  // local functions ---------------------
  void find_all_global_variables();
  void start_traversal_of_functions();
  bool searchMap(const tabletype* ltable, SgNode* node);
  bool searchVizzMap(SgNode* node);
  std::string getInitName(SgNode* sgNode);

  // the main table of all entries
  tabletype table;
  tabletype usetable;
  // table for indirect definitions
  //ideftype idefTable;
  // the helper table for visualization
  convtype vizzhelp;
  static int sgNodeCounter ;
  int nrOfNodesVisited;

  // functions to be printed in DFAtoDOT
  std::vector <FilteredCFGNode < IsDFAFilter > > dfaFunctions;

  void addAnyElement(tabletype* tabl, SgNode* sgNode, SgInitializedName* initName, SgNode* defNode);
  void mapAnyUnion(tabletype* tabl, SgNode* before, SgNode* other, SgNode* current);
  void printAnyMap(tabletype* tabl);

 public:
  DefUseAnalysis(SgProject* proj): project(proj), DEBUG_MODE(true), DEBUG_MODE_EXTRA(false){};
       virtual ~DefUseAnalysis() {}
       
  // def-use-public-functions -----------
  int run();
  int run(bool debug);
  std::multimap < SgInitializedName* , SgNode* >  getDefMultiMapFor(SgNode* node);
  std::multimap < SgInitializedName* , SgNode* >  getUseMultiMapFor(SgNode* node);
  std::vector < SgNode* > getAnyFor(const multitype* mul, SgInitializedName* initName);
  std::vector < SgNode* > getDefFor(SgNode* node, SgInitializedName* initName);
  std::vector < SgNode* > getUseFor(SgNode* node, SgInitializedName* initName);
  bool isNodeGlobalVariable(SgInitializedName* node);
  std::vector <SgInitializedName*> getGlobalVariables();
  // the following one is used for parallel traversal
  int start_traversal_of_one_function(SgFunctionDefinition* proc);

  // helpers -----------------------------
  bool searchMap(SgNode* node);
  int getDefSize();
  int getUseSize();
  void printMultiMap(const multitype* type);
  void printDefMap();  
  void printUseMap();

  void addID(SgNode* sgNode);
  void addDefElement(SgNode* sgNode, SgInitializedName* initName, SgNode* defNode);
  void addUseElement(SgNode* sgNode, SgInitializedName* initName, SgNode* defNode);
  void replaceElement(SgNode* sgNode, SgInitializedName* initName);
  void mapDefUnion(SgNode* before, SgNode* other, SgNode* current);
  void mapUseUnion(SgNode* before, SgNode* other, SgNode* current);

  void clearUseOfElement(SgNode* sgNode, SgInitializedName* initName);

  int getIntForSgNode(SgNode* node);
  void dfaToDOT();
  //void addIDefElement(SgNode* sgNode, SgInitializedName* initName);

  // clear the tables if necessary
  void flush() {
   table.clear();
   usetable.clear();
  }
};

#endif
