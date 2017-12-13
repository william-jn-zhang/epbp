/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */

#ifndef __CONS_BRANCHINFO_H__
#define __CONS_BRANCHINFO_H__

#include <assert.h>
#include <string.h>

#include "scip/scip.h"
#include "scip/type_cons.h"
#include "my_graph.h"
#include "my_def.h"


/* constraint handler properties */
#define CONSHDLR_NAME          "branchInfo"
#define CONSHDLR_DESC          "storing branch infomation at nodes of the tree constraint handler"
#define CONSHDLR_ENFOPRIORITY         0 /**< priority of the constraint handler for constraint enforcing */
#define CONSHDLR_CHECKPRIORITY  2000000 /**< priority of the constraint handler for checking feasibility */
#define CONSHDLR_PROPFREQ             1 /**< frequency for propagating domains; zero means only preprocessing propagation */
#define CONSHDLR_EAGERFREQ          100 /**< frequency for using all instead of only the useful constraints in separation,
                                              * propagation and enforcement, -1 for no eager evaluations, 0 for first only */
#define CONSHDLR_DELAYPROP        FALSE /**< should propagation method be delayed, if other propagators found reductions? */
#define CONSHDLR_NEEDSCONS         TRUE /**< should the constraint handler be skipped, if no constraints are available? */

#define CONSHDLR_PROP_TIMING       SCIP_PROPTIMING_BEFORELP

enum Branch_ConsType
{
	BRANCH_CONSTYPE_DIFFER = 0,
	BRANCH_CONSTYPE_SAME   = 1,
	BRANCH_CONSTYPE_ROOT   = 2
};

typedef enum Branch_ConsType BRANCH_CONSTYPE;

struct SCIP_ConsData
{
	SCIP_CONS*         fathercons;    // the constraint at the father node in the B&B search tree 
	                                  // [set int the BRANCH stage]
	SCIP_NODE*         stickingatnode;// the node in B&B tree at which the current constraint is sticking
	                                  // [set int the BRANCH stage]

	int                edge1;         // first edge for the SAME/DIFFER
	                                  // [set int the BRANCH stage]
	int                edge2;         // second edge for the SAME/DIFFER
	                                  // [set int the BRANCH stage]
	BRANCH_CONSTYPE    type;          // the branch type, SAME OR DIFFER
	                                  // [set int the BRANCH stage]
	

	int                (*same_branch)[2];   // all the SAME branch in the current constraint
	                                        // [alloc memory in CONS_ACTIVE event]
	int                (*differ_branch)[2]; // all the DIFFER branch int the current constraint
	                                        // [alloc memory in CONS_ACTIVE event]
	int                nsame;         // number of same branch info
	                                  // [set in CONS_ACTIVE event]
	int                ndiffer;       // number of differ branch info
	                                  // [set in CONS_ACTIVE event]
};

struct SCIP_ConshdlrData
{
	SCIP_CONS**        stack;         // the stack use to remember the branching constraint in the 
	                                  // in the branching path to the current node
	int                ncons;         // number of constraint in the stack
	int                maxstacksize;  // the maximum size of the stack
};


/* create the handler for branchInfo constraints and includes it into scip */
extern
SCIP_RETCODE SCIPincludeConshdlrBranchInfo(
	SCIP* scip
	);


#endif