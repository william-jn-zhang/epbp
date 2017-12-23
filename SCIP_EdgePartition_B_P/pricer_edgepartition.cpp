/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */

#include <assert.h>
#include <string.h>
#include <math.h>

#include "pricer_edgepartition.h"
#include "cons_branchinfo.h"
#include "scip/cons_setppc.h"
#include "scip/cons_knapsack.h"
#include "scip/scipdefplugins.h"
#include "probdata_edgepartition.h"

#include "my_def.h"
#include "utils.h"

#include "scip/heur_actconsdiving.h"
#include "scip/heur_coefdiving.h"
#include "scip/heur_crossover.h"
#include "scip/heur_dins.h"
#include "scip/heur_feaspump.h"
#include "scip/heur_fixandinfer.h"
#include "scip/heur_fracdiving.h"
#include "scip/heur_guideddiving.h"
#include "scip/heur_intdiving.h"
#include "scip/heur_intshifting.h"
#include "scip/heur_linesearchdiving.h"
#include "scip/heur_localbranching.h"
#include "scip/heur_mutation.h"
#include "scip/heur_objpscostdiving.h"
#include "scip/heur_octane.h"
#include "scip/heur_oneopt.h"
#include "scip/heur_pscostdiving.h"
#include "scip/heur_rens.h"
#include "scip/heur_rins.h"
#include "scip/heur_rootsoldiving.h"
#include "scip/heur_rounding.h"
#include "scip/heur_shifting.h"
#include "scip/heur_simplerounding.h"
#include "scip/heur_trivial.h"
#include "scip/heur_trysol.h"
#include "scip/heur_twoopt.h"
#include "scip/heur_undercover.h"
#include "scip/heur_veclendiving.h"
#include "scip/heur_zirounding.h"

/*enum PRICER_RESULT
{
	OPTIMAL,
	FIND_IMPROVE,
	DIDNOTRUN
};*/

struct SCIP_PricerData
{
	/* control infomation */
	SCIP*                                scip;                   // scip data structure
	int                                  maxvarsround;           // the maximum number of variable created in each round
	int                                  oldmaxvarsround;        // 

	SCIP_NODE*                           bbnode;                 // the current b&b tree node, used for limiting the number of pricing rounds
	int                                  noderounds;             // number of remaining pricing round in the current node
	int                                  maxroundroot; //user set// the maximum pricing round in the root node
	int                                  maxroundnode; //user set// maximum number of pricing rounds in the B&B-nodes, -1 for infinity, attention: positive value may lead to a non-optimal solution
	SCIP_Real                            lowerbound;             // lower bound computed by the pricer

	int                                  nsetsfound;             // number of improving set found in the current round

	SCIP_Bool                            usePriceHeur; //user set// using the heuristic to solve the price problem

	/* data */
	SCIP_Real*                           pi;                     // dual variable value
	int                                  constraintssize;        // number of constraints
	SCIP_CONS**                          constraints;            // a reference to the probdata -> constraints

};

static
SCIP_DECL_PRICERINITSOL(pricerInitSolEdgepartition)
{
	SCIP_PRICERDATA* pricerdata;
	SCIP_PROBDATA*   probdata;

	assert(scip != NULL);
	assert(pricer != NULL);

	pricerdata = SCIPpricerGetData(pricer);
	assert(pricerdata != NULL);

	probdata = SCIPgetProbData(scip);
	assert(probdata != NULL);

	pricerdata -> maxvarsround = 
		MAX(5, probdata -> nSets) * MAX(50, probdata -> nnodes) / 50;
	pricerdata -> constraintssize = probdata -> constraintssize;
	pricerdata -> constraints = probdata -> constraints;


	SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(pricerdata -> pi), probdata -> constraintssize) );

	pricerdata -> bbnode = NULL;

	return SCIP_OKAY;
}

static
SCIP_DECL_PRICEREXITSOL(pricerExitSolEdgepartition)
{
	SCIP_PRICERDATA* pricerdata;
	assert(scip != NULL);
	assert(pricer != NULL);

	pricerdata = SCIPpricerGetData(pricer);
	assert(pricerdata != NULL);

	SCIPfreeBlockMemoryArray(scip, &(pricerdata -> pi), pricerdata -> constraintssize);

	return SCIP_OKAY;
}

/* <start> subscip BESTSOL event handler code block */
#define EVENTHDLR_NAME         "bestsol"
#define EVENTHDLR_DESC         "event handler for best solutions found"

/* init method of BESTSOL event handler */
static
SCIP_DECL_EVENTINIT(subscipEventInitBestsol)
{
	assert(scip != NULL);
	assert(eventhdlr != NULL);
	assert(strcmp(SCIPeventhdlrGetName(eventhdlr), EVENTHDLR_NAME) == 0);

	SCIP_CALL( SCIPcatchEvent(scip, SCIP_EVENTTYPE_BESTSOLFOUND, eventhdlr, NULL, NULL) );
	return SCIP_OKAY;
}

/* exit method of BESTSOL event handler */
static
SCIP_DECL_EVENTEXIT(subscipEventExitBestsol)
{
	assert(scip != NULL);
	assert(eventhdlr != NULL);
	assert(strcmp(SCIPeventhdlrGetName(eventhdlr), EVENTHDLR_NAME) == 0);

	SCIP_CALL( SCIPdropEvent(scip, SCIP_EVENTTYPE_BESTSOLFOUND, eventhdlr, NULL, -1) );
	return SCIP_OKAY;
}

/* execute method of BESTSOL event handler */
static
SCIP_DECL_EVENTEXEC(subscipEventExecBestsol)
{
	SCIP_SOL* bestsol;
	SCIP_Real solval;
	SCIP_Real threshold;
	assert(scip != NULL);
	assert(eventhdlr != NULL);
	assert(strcmp(SCIPeventhdlrGetName(eventhdlr), EVENTHDLR_NAME) == 0);
	assert(event != NULL);
	assert(SCIPeventGetType(event) == SCIP_EVENTTYPE_BESTSOLFOUND);

	bestsol = SCIPgetBestSol(scip);
	threshold = (double)(size_t)SCIPeventhdlrGetData(eventhdlr);
	assert(bestsol != NULL);

	solval = SCIPgetSolOrigObj(scip, bestsol);

	/* once we find a feasible solution, terminate */
	//if(threshold + solval >= 1.0)
	if(SCIPisFeasLE(scip, (threshold + solval), -1.0))
	{
		SCIP_CALL( SCIPinterruptSolve(scip) );
	}

	return SCIP_OKAY;
}

static
SCIP_RETCODE subscipIncludeEventHdlrBestsol(
	SCIP* subscip
	)
{
	SCIP_EVENTHDLRDATA* eventhdlrdata;
	SCIP_EVENTHDLR* eventhdlr;
	eventhdlr = NULL;
	eventhdlrdata = NULL;

	SCIP_CALL( SCIPincludeEventhdlrBasic(subscip, &eventhdlr, EVENTHDLR_NAME, EVENTHDLR_DESC, subscipEventExecBestsol, NULL) );

	SCIP_CALL( SCIPsetEventhdlrInit(subscip, eventhdlr, subscipEventInitBestsol) );
	SCIP_CALL( SCIPsetEventhdlrExit(subscip, eventhdlr, subscipEventExitBestsol) );

	return SCIP_OKAY;
}

static
SCIP_RETCODE setSubscipBestsolEventHdlrData(
	SCIP*          subscip,
	const char*    eventhdlrName,
	double         threshold
)
{
	SCIP_EVENTHDLR* eventhdlr;
	assert(subscip != NULL);
	eventhdlr = SCIPfindEventhdlr(subscip, eventhdlrName);
	assert(eventhdlr != NULL);

	SCIPeventhdlrSetData(eventhdlr, (SCIP_EVENTHDLRDATA*)(size_t)threshold);

	return SCIP_OKAY;
}


/* <end> subscip BESTSOL event handler code block */


/*
* alloc subscip memory and construct problem instance of subscip
*/
static
SCIP_RETCODE IPPricer(
	SCIP*            scip,
	SCIP_PRICERDATA* pricerdata,
	SCIP_RESULT*   result
	)
{
	SCIP_PROBDATA* probdata;

	SCIP* subscip;
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

	/* branch-info constraint */
	SCIP_CONSHDLR* conshdlr;
	SCIP_CONSDATA* branchInfo_consdata;
	SCIP_CONSHDLRDATA* conshdlrData;

	/* the ip solution */
	SCIP_SOL* bestsol;
	SCIP_Real solval;
	SCIP_Real threshold;

	assert(scip != NULL);
	assert(pricerdata != NULL);
	
	/* collect data */

	probdata = SCIPgetProbData(scip);
	assert(probdata != NULL);

	nedges = probdata -> nedges;
	nnodes = probdata -> nnodes;
	graph = probdata -> orgngraph;

	conshdlr = SCIPfindConshdlr(scip, CONSHDLR_NAME);

	assert(conshdlr != NULL);

	conshdlrData = SCIPconshdlrGetData(conshdlr);
	assert(conshdlrData != NULL);
	assert(conshdlrData -> stack != NULL);
	cons = conshdlrData -> stack[conshdlrData -> ncons - 1];
	assert(cons != NULL);
	branchInfo_consdata = SCIPconsGetData(cons);
	assert(branchInfo_consdata != NULL);

	(*result) = SCIP_DIDNOTRUN;

	SCIP_CALL( SCIPcreate(&subscip) );
	assert(subscip != NULL);
	SCIP_CALL( SCIPincludeDefaultPlugins(subscip) );

	SCIP_CALL( SCIPcreateProbBasic(subscip, "pricing") );
	SCIP_CALL( SCIPsetObjsense(subscip, SCIP_OBJSENSE_MINIMIZE) );

	edge_vars = NULL;
	node_vars = NULL;

	edgeVarsObjCoef = NULL;
	edgeVarsSizeConsCoef = NULL;

	SCIP_CALL( SCIPallocBufferArray(scip, &edge_vars, nedges) );
	SCIP_CALL( SCIPallocBufferArray(scip, &node_vars, nnodes) );

	SCIP_CALL( SCIPallocBufferArray(scip, &edgeVarsObjCoef, nedges) );
	SCIP_CALL( SCIPallocBufferArray(scip, &edgeVarsSizeConsCoef, nedges) );
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
		if(edgeVarsSizeConsCoef[i] != 0)
		{
			generateElementName(elename, "edge", i, -1, -1);
			SCIP_CALL( SCIPcreateVarBasic(subscip, &var, elename, 0.0, 1.0, -edgeVarsObjCoef[i], SCIP_VARTYPE_BINARY) );
			SCIP_CALL( SCIPaddVar(subscip, var) );

			edge_vars[i] = var;

			SCIP_CALL( SCIPreleaseVar(subscip, &var) );
			++nedgevars;
		}
		else
		{
			edge_vars[i] = NULL;
		}
	}

	/* add constraints */

	// add edge-node relation constraints
	tmpvars = NULL;
	tmpvals = NULL;
	SCIP_CALL( SCIPallocBufferArray(scip, &tmpvars, 2) );
	SCIP_CALL( SCIPallocBufferArray(scip, &tmpvals, 2) );
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
	SCIPfreeBufferArray(scip, &tmpvars);
	SCIPfreeBufferArray(scip, &tmpvals);

	//add size constraint
	tmpsize = 0;
	tmpvals1 = NULL;
	SCIP_CALL( SCIPallocBufferArray(scip, &tmpvars, nedgevars) );
	SCIP_CALL( SCIPallocBufferArray(scip, &tmpvals1, nedgevars) );
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
	SCIPfreeBufferArray(scip, &tmpvars);
	SCIPfreeBufferArray(scip, &tmpvals1);

	// add differ branch-info constraints
	SCIP_CALL( SCIPallocBufferArray(scip, &tmpvars, 2) );
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
	SCIPfreeBufferArray(scip, &tmpvars);

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

	/* 
	* include BESTSOL event handler 
	* in order to early terminate when find a suitable var
	*/
	SCIP_CALL( subscipIncludeEventHdlrBestsol(subscip) );
	threshold = pricerdata -> pi[pricerdata -> constraintssize - 1];
	SCIP_CALL( setSubscipBestsolEventHdlrData(subscip, EVENTHDLR_NAME, threshold) );

	// solve subscip
	SCIP_CALL( SCIPsolve(subscip) );

	bestsol = SCIPgetBestSol(subscip);
	assert(bestsol != NULL);

	solval = SCIPgetSolOrigObj(subscip, bestsol);

	addvar = FALSE;

	/* once we find a feasible solution, terminate */
	if(SCIPisFeasLE(subscip, (threshold + solval), -1.0))
	{
		// TODO: add the new find var
		setArray = NULL;
		varnodeset = NULL;

		setArrayLength = 0;
		SCIP_CALL( SCIPallocBufferArray(scip, &setArray, nedges) );

		// create the new add var edge set
		for(int i = 0; i < nedges; ++i)
		{
			if(SCIPisFeasEQ(
				subscip, 
				SCIPgetSolVal(subscip, bestsol, edge_vars[RepFind(branchInfo_consdata -> rep, i)]),
				1.0))
			{
				setArray[setArrayLength] = i;
				++setArrayLength;
			}
		}

		// count the number of nodes covered by the var edge set
		SCIP_CALL( SCIPallocBufferArray(scip, &varnodeset, nnodes) );
		memset(varnodeset, 0, nnodes * sizeof(int));
		newvarcoef = 0;
		for(int i = 0; i < setArrayLength; ++i)
		{
			MY_EDGE e = MY_GRAPHgetEdge(graph, setArray[i]);
			if(varnodeset[e.node1] == 0)
			{
				++newvarcoef;
				varnodeset[e.node1] = 1;
			}
			if(varnodeset[e.node2] == 0)
			{
				++newvarcoef;
				varnodeset[e.node2] = 1;
			}
		}

		SCIPfreeBufferArray(scip, &varnodeset);

		// add the new find var set
		SCIP_CALL( SCIPprobdataADDVarSet(scip, setArray, setArrayLength, &newvaridx) );

		// create the new find var
		SCIP_CALL( SCIPcreateVar(scip, &var, NULL, 0.0, 1.0, newvarcoef, SCIP_VARTYPE_BINARY,
				 TRUE, TRUE, NULL, NULL, NULL, NULL, (SCIP_VARDATA*)(size_t)newvaridx) );

		// add new var
		SCIP_CALL( SCIPprobdataADDVar(scip, var, newvaridx) );

		// add the new var into problem
		SCIP_CALL( SCIPaddPricedVar(scip, var, (threshold + solval)) );
		SCIP_CALL( SCIPchgVarUbLazy(scip, var, 1.0) );

		// add the new variable to the corresponding constraints
		for(int i = 0; i < setArrayLength; ++i)
		{
			SCIP_CALL( SCIPaddCoefSetppc(scip, pricerdata -> constraints[setArray[i]], var) );
		}
		SCIP_CALL( SCIPaddCoefKnapsack(scip, pricerdata -> constraints[pricerdata -> constraintssize -1], var, 1) );

		SCIPfreeBufferArray(scip, &setArray);
		addvar = TRUE;
	}

	if(addvar || SCIPgetStatus(subscip) == SCIP_STATUS_OPTIMAL)
	{
		(*result) = SCIP_SUCCESS;
	}

	SCIP_CALL( SCIPfree(&subscip) );

	SCIPfreeBufferArray(scip, &edge_vars);
	SCIPfreeBufferArray(scip, &node_vars);
	SCIPfreeBufferArray(scip, &edgeVarsObjCoef);
	SCIPfreeBufferArray(scip, &edgeVarsSizeConsCoef);

	return SCIP_OKAY;
}


static
SCIP_DECL_PRICERREDCOST(pricerRedcostEdgePartition)
{
	SCIP_PRICERDATA* pricerdata;

	assert(scip != NULL);
	assert(pricer != NULL);

	pricerdata = SCIPpricerGetData(pricer);
	assert(pricerdata != NULL);

	if( pricerdata -> bbnode == SCIPgetCurrentNode(scip))
	{
		if(pricerdata -> noderounds > 0)
			pricerdata -> noderounds --;
	}
	else
	{
		if(pricerdata -> bbnode == NULL)
		{
			pricerdata -> noderounds = pricerdata -> maxroundroot;
			pricerdata -> lowerbound = -SCIPinfinity(scip);
		}
		else
		{
			pricerdata -> noderounds = pricerdata -> maxroundnode;
			pricerdata -> lowerbound = -SCIPinfinity(scip);
		}
		pricerdata -> bbnode = SCIPgetCurrentNode(scip);
	}

	if(pricerdata -> noderounds == 0)
	{
		/* maxrounds reached, pricing interrupted */
		*result = SCIP_DIDNOTRUN;
		*lowerbound = pricerdata -> lowerbound;

		return SCIP_OKAY;
	}

	*result = SCIP_SUCCESS;

	/* get the dual variable value */
	for(int i = 0; i < pricerdata -> constraintssize - 1; ++i)
	{
		pricerdata -> pi[i] = SCIPgetDualsolSetppc(scip, pricerdata -> constraints[i]);
	}
	pricerdata -> pi[pricerdata -> constraintssize - 1] = SCIPgetDualsolKnapsack(scip, pricerdata -> constraints[pricerdata -> constraintssize -1]);
	
	pricerdata -> nsetsfound = 0;

	// debug print
#ifdef DEBUG_PRINT
	printArray("dual val", pricerdata -> pi, pricerdata -> constraintssize);
#endif

	//pricer heuristic
	if(pricerdata -> usePriceHeur)
	{
		/* using pricer heurisitc */
	}

	/* 
	* using exact method (integer programming) to prove the optimal lp-sol of current master problem 
	* of find a new variable which can improve the current master problem
	*/
	if(pricerdata -> nsetsfound == 0)
	{
		SCIP_CALL( IPPricer(scip, pricerdata, result) );
	}

#ifdef DEBUG_PRINT
	printf("%d", *result);
#endif

	return SCIP_OKAY;
}

SCIP_RETCODE SCIPincludePricerEdgePartition(
	SCIP* scip
	)
{
	SCIP_PRICERDATA* pricerdata;
	SCIP_PRICER* pricer;
	assert(scip != NULL);

	pricerdata = NULL;
	SCIP_CALL( SCIPallocBlockMemory(scip, &pricerdata) );
	pricerdata -> scip = scip;
	pricerdata -> maxvarsround = 0;
	pricerdata -> oldmaxvarsround = 0;

	pricer = NULL;

	/* include variable pricer */
	SCIP_CALL( SCIPincludePricerBasic(scip, &pricer, PRICER_NAME, PRICER_DESC, PRICER_PRIORITY, PRICER_DELAY,
         pricerRedcostEdgePartition, NULL, pricerdata) );

	assert(pricer != NULL);

	SCIP_CALL( SCIPsetPricerInitsol(scip, pricer, pricerInitSolEdgepartition) );
	SCIP_CALL( SCIPsetPricerExitsol(scip, pricer, pricerExitSolEdgepartition) );

	return SCIP_OKAY;
}