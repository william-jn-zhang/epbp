/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */

#include "my_def.h"
#include "ip_pricer.h"
#include "cons_branchinfo.h"
#include "scip/cons_setppc.h"
#include "scip/cons_knapsack.h"
#include "scip/scipdefplugins.h"
#include "probdata_edgepartition.h"

struct IP_PRICER_PROBDATA
{
	SCIP_VAR** edge_vars; // edge variables array
	SCIP_VAR** node_vars; // node variables array

	int nedges;   // number of edges
	int nnodes;   // number of nodes
};


SCIP_RETCODE createIpPricerProblem(
	SCIP**                scip,        // the created subscip instance
	SCIP_PRICERDATA*      pricerdata,  // pricer information
	SCIP_PROBDATA*        probdata,    // the probdata of master problem
	SCIP_CONSDATA*        branchInfo_consdata // the consData of branch_info constraint
)
{
	IP_PRICER_PROBDATA* sub_probdata;

	////////////////////////////////////

	SCIP_VAR** edge_vars; // edge variables array
	SCIP_VAR** node_vars; // node variables array

	int nedgevars; // number of non-null elements in edge_vars array

	SCIP_Real* edgeVarsObjCoef; // the coefficent of edge variables in the objective
	int* edgeVarsSizeConsCoef; // the coefficent of edge variables in the size-constraint
	
	SCIP_VAR*  var;
	char elename[MAX_NAME_LEN];
	SCIP_CONS* cons;

	int nedges;
	int nnodes;

	MY_GRAPH* graph;

	SCIP_VAR** tmpvars;
	double*      tmpvals;
	long long*   tmpvals1;
	int tmpsize;
	long long    capacity;

	SCIP_Bool addvar;
	int*      setArray;
	int       setArrayLength;
	int       newvaridx;
	int       newvarcoef;
	int*      varnodeset;


	///////////////////////////////////

	assert(pricerdata != NULL);

	SCIP_CALL(SCIPcreate(scip));
	
	SCIP* subscip = *scip;

	assert(subscip != NULL);
	assert(branchInfo_consdata != NULL);

	SCIP_CALL( SCIPincludeDefaultPlugins(subscip) );

	SCIP_CALL( SCIPcreateProbBasic(subscip, "pricing") );
	SCIP_CALL( SCIPsetObjsense(subscip, SCIP_OBJSENSE_MINIMIZE) );

#ifndef SUBSCIP_OUTPUT_ENABLE
	SCIP_CALL( SCIPsetIntParam(subscip, "display/verblevel", 0) );
#endif

	SCIP_CALL( SCIPenableReoptimization(subscip, TRUE) );

	/* create and set sub problem probdata */
	SCIP_CALL( SCIPallocBuffer(subscip, &sub_probdata) );
	SCIP_CALL( SCIPsetProbData(subscip, (SCIP_PROBDATA*)sub_probdata) );

	nedges = probdata -> nedges;
	nnodes = probdata -> nnodes;
	graph = probdata -> orgngraph;

	edge_vars = NULL;
	node_vars = NULL;

	edgeVarsObjCoef = NULL;
	edgeVarsSizeConsCoef = NULL;

	SCIP_CALL( SCIPallocBufferArray(subscip, &edge_vars, nedges) );
	SCIP_CALL( SCIPallocBufferArray(subscip, &node_vars, nnodes) );

	sub_probdata -> edge_vars = edge_vars;
	sub_probdata -> node_vars = node_vars;

	SCIP_CALL( SCIPallocBufferArray(subscip, &edgeVarsObjCoef, nedges) );
	SCIP_CALL( SCIPallocBufferArray(subscip, &edgeVarsSizeConsCoef, nedges) );
	memset(edgeVarsObjCoef, 0, nedges * sizeof(SCIP_Real));
	memset(edgeVarsSizeConsCoef, 0, nedges * sizeof(int));

	// construct coefficents
	for(int i = 0; i < nedges; ++i)
	{
		edgeVarsObjCoef[RepFind(branchInfo_consdata -> rep, i)] += pricerdata -> pi[i];
		edgeVarsSizeConsCoef[RepFind(branchInfo_consdata -> rep, i)] += 1;
	}

	/* add variables */

	// add node variables
	for(int i = 0; i < nnodes; ++i)
	{
		generateElementName(elename, "node", i, -1, -1);
		SCIP_CALL( SCIPcreateVarBasic(subscip, &var, elename, 0.0, 1.0, 1.0, SCIP_VARTYPE_BINARY) );
		SCIP_CALL( SCIPaddVar(subscip, var) );

		node_vars[i] = var;

		SCIP_CALL( SCIPreleaseVar(subscip, &var) );
	}

	// add edge variables
	nedgevars = 0;
	for(int i = 0; i < nedges; ++i)
	{
		generateElementName(elename, "edge", i, -1, -1);
		SCIP_CALL( SCIPcreateVarBasic(subscip, &var, elename, 0.0, 1.0, -edgeVarsObjCoef[i], SCIP_VARTYPE_BINARY) );
		SCIP_CALL( SCIPaddVar(subscip, var) );

		edge_vars[i] = var;

		SCIP_CALL( SCIPreleaseVar(subscip, &var) );
		++nedgevars;
	}

	// add edge-node relation constraints
	tmpvars = NULL;
	tmpvals = NULL;
	SCIP_CALL( SCIPallocBufferArray(subscip, &tmpvars, 2) );
	SCIP_CALL( SCIPallocBufferArray(subscip, &tmpvals, 2) );
	for(int i = 0; i < nedges; ++i)
	{
		MY_EDGE e = MY_GRAPHgetEdge(graph, i);

		tmpvars[0] = node_vars[e.node1];
		tmpvars[1] = edge_vars[RepFind(branchInfo_consdata -> rep, i)];
		tmpvals[0] = 1.0;
		tmpvals[1] = -1.0;
		assert(tmpvars[0] != NULL);
		assert(tmpvars[1] != NULL);
		generateElementName(elename, "relation_cons", i, e.node1, -1);
		SCIP_CALL( SCIPcreateConsBasicLinear(subscip, &cons, elename, 2, tmpvars, tmpvals, 0, SCIP_DEFAULT_INFINITY) );
		SCIP_CALL( SCIPaddCons(subscip, cons) );
		SCIP_CALL( SCIPreleaseCons(subscip, &cons) );

		tmpvars[0] = node_vars[e.node2];
		tmpvars[1] = edge_vars[RepFind(branchInfo_consdata -> rep, i)];
		tmpvals[0] = 1.0;
		tmpvals[1] = -1.0;
		assert(tmpvars[0] != NULL);
		assert(tmpvars[1] != NULL);
		generateElementName(elename, "relation_cons", i, e.node2, -1);
		SCIP_CALL( SCIPcreateConsBasicLinear(subscip, &cons, elename, 2, tmpvars, tmpvals, 0, SCIP_DEFAULT_INFINITY) );
		SCIP_CALL( SCIPaddCons(subscip, cons) );
		SCIP_CALL( SCIPreleaseCons(subscip, &cons) );
	}
	SCIPfreeBufferArray(subscip, &tmpvars);
	SCIPfreeBufferArray(subscip, &tmpvals);

	//add size constraint
	tmpsize = 0;
	tmpvals1 = NULL;
	SCIP_CALL( SCIPallocBufferArray(subscip, &tmpvars, nedgevars) );
	SCIP_CALL( SCIPallocBufferArray(subscip, &tmpvals1, nedgevars) );
	for(int i = 0; i < nedges; ++i)
	{
		if(edgeVarsSizeConsCoef[i] != 0)
		{
			tmpvars[tmpsize] = edge_vars[i];
			tmpvals1[tmpsize] = edgeVarsSizeConsCoef[i];
			++tmpsize;
		}
	}
	assert(tmpsize == nedgevars);
	capacity = ceil((double)(probdata -> alpha) * nedges / (probdata -> nparts));
	SCIP_CALL( SCIPcreateConsBasicKnapsack(subscip, &cons, "size_cons", nedgevars, tmpvars, tmpvals1, capacity) );
	SCIP_CALL( SCIPaddCons(subscip, cons) );
	SCIP_CALL( SCIPreleaseCons(subscip, &cons) );
	SCIPfreeBufferArray(subscip, &tmpvars);
	SCIPfreeBufferArray(subscip, &tmpvals1);

	// add differ branch-info constraints
	SCIP_CALL( SCIPallocBufferArray(subscip, &tmpvars, 2) );
	for(int i = 0; i < branchInfo_consdata -> ndiffer; ++i)
	{
		int e1 = branchInfo_consdata -> differ_branch[i][0];
		int e2 = branchInfo_consdata -> differ_branch[i][1];
		assert(e1 != e2);
		tmpvars[0] = edge_vars[RepFind(branchInfo_consdata -> rep, e1)];
		tmpvars[1] = edge_vars[RepFind(branchInfo_consdata -> rep, e2)];

		generateElementName(elename, "differ_cons", i, -1, -1);
		SCIP_CALL( SCIPcreateConsBasicSetpack(subscip, &cons, elename, 2, tmpvars) );
		SCIP_CALL( SCIPaddCons(subscip, cons) );
		SCIP_CALL( SCIPreleaseCons(subscip, &cons) );
	}
	SCIPfreeBufferArray(subscip, &tmpvars);

	/* finished build problem */

	//add heuristic
#ifdef SUBSCIP_USEHEUR
	SCIP_CALL( SCIPincludeHeurActconsdiving(subscip) );
	SCIP_CALL( SCIPincludeHeurCoefdiving(subscip) );
	SCIP_CALL( SCIPincludeHeurCrossover(subscip) );
	SCIP_CALL( SCIPincludeHeurDins(subscip) );
	SCIP_CALL( SCIPincludeHeurFixandinfer(subscip) );
	SCIP_CALL( SCIPincludeHeurFracdiving(subscip) );
	SCIP_CALL( SCIPincludeHeurGuideddiving(subscip) );
	SCIP_CALL( SCIPincludeHeurIntdiving(subscip) );
	SCIP_CALL( SCIPincludeHeurIntshifting(subscip) );
	SCIP_CALL( SCIPincludeHeurLinesearchdiving(subscip) );
	SCIP_CALL( SCIPincludeHeurLocalbranching(subscip) );
	SCIP_CALL( SCIPincludeHeurMutation(subscip) );
	SCIP_CALL( SCIPincludeHeurObjpscostdiving(subscip) );
	SCIP_CALL( SCIPincludeHeurOctane(subscip) );
	SCIP_CALL( SCIPincludeHeurOneopt(subscip) );
	SCIP_CALL( SCIPincludeHeurPscostdiving(subscip) );
	SCIP_CALL( SCIPincludeHeurRens(subscip) );
	SCIP_CALL( SCIPincludeHeurRins(subscip) );
	SCIP_CALL( SCIPincludeHeurRootsoldiving(subscip) );
	SCIP_CALL( SCIPincludeHeurRounding(subscip) );
	SCIP_CALL( SCIPincludeHeurShifting(subscip) );
	SCIP_CALL( SCIPincludeHeurSimplerounding(subscip) );
	SCIP_CALL( SCIPincludeHeurTrivial(subscip) );
	SCIP_CALL( SCIPincludeHeurTrySol(subscip) );
	SCIP_CALL( SCIPincludeHeurTwoopt(subscip) );
	SCIP_CALL( SCIPincludeHeurUndercover(subscip) );
	SCIP_CALL( SCIPincludeHeurVeclendiving(subscip) );
	SCIP_CALL( SCIPincludeHeurZirounding(subscip) );
#endif

	SCIPfreeBufferArray(subscip, &edgeVarsObjCoef);
	SCIPfreeBufferArray(subscip, &edgeVarsSizeConsCoef);

	return SCIP_OKAY;
}


SCIP_RETCODE changIpPricerObjCoef(
	SCIP*                 subscip,     // the created subscip instance
	SCIP_PRICERDATA*      pricerdata,   // pricer information, contain the dual val of master problem
	SCIP_CONSDATA*        branchInfo_consdata // the consData of branch_info constraint
)
{
	//SCIPchgVarObj
	SCIP_Real* edgeVarsObjCoef; // the coefficent of edge variables in the objective
	IP_PRICER_PROBDATA* sub_probdata;
	int nedges;
	int nnodes;
	SCIP_VAR** edge_vars;
	SCIP_VAR** node_vars;

	assert(subscip != NULL);
	assert(pricerdata != NULL);
	assert(branchInfo_consdata != NULL);

	sub_probdata = (IP_PRICER_PROBDATA*)SCIPgetProbData(subscip);
	nedges = sub_probdata -> nedges;
	nnodes = sub_probdata -> nnodes;
	edge_vars = sub_probdata -> edge_vars;
	node_vars = sub_probdata -> node_vars;

	edgeVarsObjCoef = NULL;
	SCIP_CALL( SCIPallocBufferArray(subscip, &edgeVarsObjCoef, nedges) );
	memset(edgeVarsObjCoef, 0, nedges * sizeof(SCIP_Real));

	// construct coefficents
	for(int i = 0; i < nedges; ++i)
	{
		edgeVarsObjCoef[RepFind(branchInfo_consdata -> rep, i)] += pricerdata -> pi[i];
	}

	SCIP_CALL( SCIPfreeReoptSolve(subscip) );

	// change the obj val of each edge variable
	for(int i = 0; i < nedges; ++i)
	{
		SCIP_CALL( SCIPchgVarObj(subscip, edge_vars[i], -edgeVarsObjCoef[i]) );
	}

	

	return SCIP_OKAY;
}