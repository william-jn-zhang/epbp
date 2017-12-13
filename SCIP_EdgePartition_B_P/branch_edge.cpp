/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */

#include <assert.h>
#include <string.h>

#include "branch_edge.h"
#include "my_def.h"
#include "probdata_edgepartition.h"

#define BRANCHRULE_NAME         "branch edge"
#define BRANCHRULE_DESC         "branch rule ryan forster on edge"
#define BRANCHRULE_PRIORITY     50000
#define BRANCHRULE_MAXDEPTH     -1
#define BRANCHRULE_MAXBOUNDDIST 1.0


static
SCIP_DECL_BRANCHEXECLP(branchEdgeExeLp)
{
	/* array of candidates for branching and its fractionality */
	SCIP_VAR** lpcands;
	SCIP_Real* lpcandsfrac;
	int nlpcands;

	/* variables for finding the most fractional column */
	SCIP_Real frac;
	SCIP_Real bestfrac;
	int bestcand;

	/* variables in a constraint */
	SCIP_VAR** vars;
	int nvars;

	SCIP_VAR* s1;
	SCIP_VAR* s2;
	int s1idx;

	int* set1;
	int nset1;
	int* set2;
	int nset2;

	int edge1;
	int edge2;

	SCIP_CONS* cons1;

	SCIP_NODE* childsame;
	SCIP_NODE* childdiffer;

	SCIP_CONS* conssame;
	SCIP_CONS* consdiffer;

	SCIP_CONS* currentcons_branchInfo;

	assert(scip != NULL);
	assert(branchrule != NULL);
	assert(strcmp(SCIPbranchruleGetName(branchrule), BRANCHRULE_NAME) == 0);
	assert(result != NULL);

	*result = SCIP_DIDNOTRUN;

	SCIP_CALL( SCIPgetLPBranchCands(scip, &lpcands, NULL, &lpcandsfrac, NULL, &nlpcands, NULL) );
	assert(nlpcands > 0);

	bestcand = -1;
	bestfrac = 1;

	for(int i = 0; i < nlpcands; ++i)
	{
		assert(lpcands[i] != NULL);
		frac = lpcandsfrac[i];
		frac = MIN(frac, 1.0 - frac);
		if( frac < bestfrac)
		{
			bestfrac = frac;
			bestcand = i;
		}
	}

	assert(bestcand >= 0);
	assert(SCIPisFeasPositive(scip, bestfrac));

	s1 = lpcands[bestcand];
	s1idx = (int)(size_t)SCIPvarGetData(s1);

	SCIP_CALL( SCIPprobdataGETVarSet(scip, s1idx, &set1, &nset1) );

	edge1 = -1;
	edge2 = -1;
	s2 = NULL;

	for(int i = 0; ((i < nset1) && (edge2 == -1)); ++i)
	{
		edge1 = set1[i];
		cons1 = SCIPprobdataGETConstraint(scip, edge1);
		vars = SCIPgetVarsSetppc(scip, cons1);

	}

	return SCIP_OKAY;
}

SCIP_RETCODE SCIPincludeEdgeBranchRule(
	SCIP* scip
	)
{
	SCIP_BRANCHRULE* branchrule;

	assert(scip != NULL);

	branchrule = NULL;

	SCIP_CALL( SCIPincludeBranchruleBasic(scip, &branchrule, BRANCHRULE_NAME, BRANCHRULE_DESC, BRANCHRULE_PRIORITY, BRANCHRULE_MAXDEPTH,
	 BRANCHRULE_MAXBOUNDDIST, NULL) );



	return SCIP_OKAY;
}