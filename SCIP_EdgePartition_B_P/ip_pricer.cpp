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
#include "scip/cons_linking.h"
#include "scip/scipdefplugins.h"
#include "probdata_edgepartition.h"
#include "hash_debugger.h"

struct IP_PRICER_PROBDATA
{
	SCIP* masterscip; // reference to master scip data structure

	SCIP_VAR** edge_vars; // edge variables array
	SCIP_VAR** node_vars; // node variables array

	int nedges;   // number of edges
	int nnodes;   // number of nodes

	SCIP_PRICERDATA* pricedata; // reference to pricedata data structure

	/* control field */
	
	int nVarAdd; // number of priced variables have been added in each IP pricer round
	int maxNVarAdd; // the maximum number of priced variables add in each IP pricer round
	//SCIP_Bool addvar;

	//SCIP_STATUS status; // the exit status of the ip pricer
};

/* free the data probdata data structure of the subscip problem */
static
SCIP_RETCODE freeSubProbdata(
	SCIP*                   subscip,     // the subscip data structure
	IP_PRICER_PROBDATA**     subprobdata  // the sub problem data 
)
{
	assert(subscip != NULL);
	assert(subprobdata != NULL);

	// release all edge variables
	for(int i = 0; i < (*subprobdata) -> nedges; ++i)
	{
		SCIP_CALL( SCIPreleaseVar(subscip, &(*subprobdata) -> edge_vars[i]) );
	}

	// release all node variables
	for(int i = 0; i < (*subprobdata) -> nnodes; ++i)
	{
		SCIP_CALL( SCIPreleaseVar(subscip, &(*subprobdata) -> node_vars[i]) );
	}
	
	SCIPfreeBlockMemoryArray(subscip, &((*subprobdata) -> edge_vars), (*subprobdata) -> nedges);
	SCIPfreeBlockMemoryArray(subscip, &((*subprobdata) -> node_vars), (*subprobdata) -> nnodes);

	SCIPfreeBlockMemory(subscip, subprobdata);

	return SCIP_OKAY;
}

static
SCIP_DECL_PROBDELORIG(subprobdataDelOrig)
{
	//SCIPdebugMsg(scip, "free original problem data\n");///////////////////////////////////////////////////////////////

	SCIP_CALL( freeSubProbdata(scip, (IP_PRICER_PROBDATA**)probdata) );

	return SCIP_OKAY;
}

static
SCIP_RETCODE setAddedVar(IP_PRICER_PROBDATA* sub_probdata, SCIP_Bool addedvar)
{
	sub_probdata -> pricedata -> addedvar = addedvar;

	return SCIP_OKAY;
}

//static
//SCIP_RETCODE setStatus(IP_PRICER_PROBDATA* sub_probdata, SCIP_STATUS status)
//{
//	sub_probdata -> pricedata -> status = status;
//	return SCIP_OKAY;
//}


static
SCIP_RETCODE addNewPricedVar(
	SCIP* subscip,
	SCIP_SOL* bestsol
)
{
	IP_PRICER_PROBDATA* subprobdata;
	SCIP* scip;  // master scip
	SCIP_Real solval;
	SCIP_CONSDATA* branchInfo_consdata;
	MY_GRAPH* graph;
	SCIP_PRICERDATA* pricerdata;
	double threshold;

	int nedges;
	int nnodes;
	int newvarcoef;

	int* setArray;
	int* varnodeset;
	double* solnodeset;
	int setArrayLength;

	SCIP_VAR** edge_vars;
	SCIP_VAR** node_vars;
	SCIP_VAR* var;
	int newvaridx;

	//SCIPdebugMessage("Enter function: addNewPricedVar \n");///////////////////////////////////////////////////

	assert(subscip != NULL);
	assert(bestsol != NULL);

	subprobdata = (IP_PRICER_PROBDATA*)SCIPgetProbData(subscip);
	assert(subprobdata != NULL);
	scip = subprobdata -> masterscip;
	assert(scip != NULL);

	nedges = subprobdata -> nedges;
	nnodes = subprobdata -> nnodes;
	edge_vars = subprobdata -> edge_vars;
	node_vars = subprobdata -> node_vars;
	
	pricerdata = subprobdata -> pricedata;

	threshold = pricerdata -> pi[pricerdata -> constraintssize - 1];

	branchInfo_consdata = SCIPconsGetData(SCIPconsGetActiveBranchInfoCons(scip));
	graph = SCIPgetProbData(scip) -> orgngraph;



	solval = SCIPgetSolOrigObj(subscip, bestsol);

	/* once we find a feasible solution, terminate */
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

	/* 
	avoid duplicate setArray
	baseed on the following observation:
	the duplicated solution's node set is larger than the varnodeset
	*/
	solnodeset = NULL;
	SCIP_CALL( SCIPallocBufferArray(scip, &solnodeset, nnodes) );
	memset(solnodeset, 0, nnodes * sizeof(int));
	SCIP_CALL( SCIPgetSolVals(scip, bestsol, nnodes, subprobdata -> node_vars, solnodeset) );
	for(int i = 0; i < nnodes; ++i)
	{
		if(!SCIPisFeasEQ(scip, varnodeset[i], solnodeset[i]))
		{
			SCIPfreeBufferArray(scip, &solnodeset);
			SCIPfreeBufferArray(scip, &varnodeset);
			SCIPfreeBufferArray(scip, &setArray);

			return SCIP_OKAY;
		}
	}


#ifdef PRICER_DEBUG
	calcHash_wrap(setArray, setArrayLength * sizeof(int));
	SCIPdebugMessage("ippricer-setArray:");
	printIntArray(setArray, setArrayLength);
	printf("\n");
	printHash_wrap("");
#endif

#ifdef PRICER_DEBUG
	calcHash_wrap(pricerdata -> pi, pricerdata -> constraintssize * sizeof(double));
	SCIPdebugMessage("ippricer-pi:");
	printDoubleArray(pricerdata -> pi, pricerdata -> constraintssize);
	printf("\n");
	printHash_wrap("");
#endif

#ifdef PRICER_DEBUG
	calcHash_wrap(varnodeset, nnodes * sizeof(int));
	SCIPdebugMessage("ippricer-varnodeset:");
	printIntArray(varnodeset, nnodes);
	printf("\n");
	printHash_wrap("");
#endif

	SCIPfreeBufferArray(scip, &solnodeset);
	SCIPfreeBufferArray(scip, &varnodeset);


	// add the new find var set
	SCIP_CALL( SCIPprobdataADDVarSet(scip, setArray, setArrayLength, &newvaridx) );

	// create the new find var
	SCIP_CALL( SCIPcreateVar(scip, &var, NULL, 0.0, 1.0, newvarcoef, SCIP_VARTYPE_BINARY,
				TRUE, TRUE, NULL, NULL, NULL, NULL, (SCIP_VARDATA*)(size_t)newvaridx) );

	// add new var
	SCIP_CALL( SCIPprobdataADDVar(scip, var, newvaridx) );

	// add the new var into problem
	SCIP_CALL( SCIPaddPricedVar(scip, var, -(threshold + solval)) );
	SCIP_CALL( SCIPchgVarUbLazy(scip, var, 1.0) );

	// add the new variable to the corresponding constraints
	for(int i = 0; i < setArrayLength; ++i)
	{
		SCIP_CALL( SCIPaddCoefSetppc(scip, pricerdata -> constraints[setArray[i]], var) );
	}
	SCIP_CALL( SCIPaddCoefKnapsack(scip, pricerdata -> constraints[pricerdata -> constraintssize -1], var, 1) );

	SCIPfreeBufferArray(scip, &setArray);

	subprobdata -> nVarAdd += 1;
	setAddedVar(subprobdata, TRUE);


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
	IP_PRICER_PROBDATA* subprobdata;

	assert(scip != NULL);
	assert(eventhdlr != NULL);
	assert(strcmp(SCIPeventhdlrGetName(eventhdlr), EVENTHDLR_NAME) == 0);
	assert(event != NULL);
	assert(SCIPeventGetType(event) == SCIP_EVENTTYPE_BESTSOLFOUND);

	bestsol = SCIPgetBestSol(scip);
	threshold = (double)(size_t)SCIPeventhdlrGetData(eventhdlr);
	assert(bestsol != NULL);

	//TODO: get all best solution

	subprobdata = (IP_PRICER_PROBDATA*)SCIPgetProbData(scip);

	solval = SCIPgetSolOrigObj(scip, bestsol);

#ifdef PRICER_DEBUG
	calcHash_wrap(&solval, sizeof(double));
	SCIPdebugMessage("ippricer-solval:%lf,", solval);
	printHash_wrap("");
#endif

	/* once we find a feasible solution, terminate */
	//if(threshold + solval >= 1.0)

	//SCIP_CALL( SCIPprintSol(scip, bestsol, NULL, TRUE) );

	if(SCIPisFeasLE(scip, (threshold + solval), -1.0))
	{
		if(subprobdata -> nVarAdd < subprobdata -> maxNVarAdd)
		{
			SCIP_CALL( addNewPricedVar(scip, bestsol) );
		}
		else
		{
			SCIP_CALL( SCIPinterruptSolve(scip) );
		}
		//if(subprobdata -> nVarAdd == subprobdata -> maxNVarAdd || subprobdata -> pricedata -> addedvar == FALSE || SCIPgetStatus(scip) == SCIP_STATUS_OPTIMAL)
		//{
		//	SCIP_CALL( SCIPinterruptSolve(scip) );
		//}
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



SCIP_RETCODE createIpPricerProblem(
	SCIP**                scip,        // the created subscip instance
	SCIP*                 masterscip,  // the master scip instance
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
	double threshold;

	SCIP_Bool addvar;
	int*      setArray;
	int       setArrayLength;
	int       newvaridx;
	int       newvarcoef;
	int*      varnodeset;


	///////////////////////////////////

#ifdef PRICER_DEBUG
	SCIPdebugMessage("Enter function: createIpPricerProblem \n");//////////////////////////////////////////////////
#endif

	assert(pricerdata != NULL);

	SCIP_CALL(SCIPcreate(scip));
	
	SCIP* subscip = *scip;

	assert(subscip != NULL);
	assert(branchInfo_consdata != NULL);

	SCIP_CALL( SCIPincludeDefaultPlugins(subscip) );

	SCIP_CALL( SCIPcreateProbBasic(subscip, "pricing") );
	SCIP_CALL( SCIPsetObjsense(subscip, SCIP_OBJSENSE_MINIMIZE) );

	SCIP_CALL( SCIPsetProbDelorig(subscip, subprobdataDelOrig) );

#ifndef SUBSCIP_OUTPUT_ENABLE
	SCIP_CALL( SCIPsetIntParam(subscip, "display/verblevel", 0) );
#endif

	SCIP_CALL( SCIPenableReoptimization(subscip, TRUE) );

	/* create and set sub problem probdata */
	sub_probdata = NULL;
	SCIP_CALL( SCIPallocBlockMemory(subscip, &sub_probdata) );
	SCIP_CALL( SCIPsetProbData(subscip, (SCIP_PROBDATA*)sub_probdata) );

	nedges = probdata -> nedges;
	nnodes = probdata -> nnodes;
	graph = probdata -> orgngraph;

	edge_vars = NULL;
	node_vars = NULL;

	edgeVarsObjCoef = NULL;
	edgeVarsSizeConsCoef = NULL;

	SCIP_CALL( SCIPallocBlockMemoryArray(subscip, &edge_vars, nedges) );
	SCIP_CALL( SCIPallocBlockMemoryArray(subscip, &node_vars, nnodes) );

	sub_probdata -> edge_vars = edge_vars;
	sub_probdata -> node_vars = node_vars;
	sub_probdata -> nedges = nedges;
	sub_probdata -> nnodes = nnodes;
	sub_probdata -> masterscip = masterscip;
	sub_probdata -> pricedata = pricerdata;

	sub_probdata -> nVarAdd = 0;
	sub_probdata -> maxNVarAdd = 10;

	setAddedVar(sub_probdata, FALSE);

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
		generateElementName(elename, "relation_cons", i, e.node1, 0);
		SCIP_CALL( SCIPcreateConsBasicLinear(subscip, &cons, elename, 2, tmpvars, tmpvals, 0.0, SCIP_DEFAULT_INFINITY) );
		SCIP_CALL( SCIPaddCons(subscip, cons) );
		SCIP_CALL( SCIPreleaseCons(subscip, &cons) );

		//tmpvars[1] = node_vars[e.node1];
		//tmpvars[0] = edge_vars[RepFind(branchInfo_consdata -> rep, i)];
		//tmpvals[0] = 1.0;
		//tmpvals[1] = -1.0;
		//assert(tmpvars[0] != NULL);
		//assert(tmpvars[1] != NULL);
		//generateElementName(elename, "relation_cons", i, e.node1, 1);
		//SCIP_CALL( SCIPcreateConsBasicLinear(subscip, &cons, elename, 2, tmpvars, tmpvals, 0.0, SCIP_DEFAULT_INFINITY) );
		//SCIP_CALL( SCIPaddCons(subscip, cons) );
		//SCIP_CALL( SCIPreleaseCons(subscip, &cons) );

		tmpvars[0] = node_vars[e.node2];
		tmpvars[1] = edge_vars[RepFind(branchInfo_consdata -> rep, i)];
		tmpvals[0] = 1.0;
		tmpvals[1] = -1.0;
		assert(tmpvars[0] != NULL);
		assert(tmpvars[1] != NULL);
		generateElementName(elename, "relation_cons", i, e.node2, 0);
		SCIP_CALL( SCIPcreateConsBasicLinear(subscip, &cons, elename, 2, tmpvars, tmpvals, 0.0, SCIP_DEFAULT_INFINITY) );
		SCIP_CALL( SCIPaddCons(subscip, cons) );
		SCIP_CALL( SCIPreleaseCons(subscip, &cons) );

		//tmpvars[1] = node_vars[e.node2];
		//tmpvars[0] = edge_vars[RepFind(branchInfo_consdata -> rep, i)];
		//tmpvals[0] = 1.0;
		//tmpvals[1] = -1.0;
		//assert(tmpvars[0] != NULL);
		//assert(tmpvars[1] != NULL);
		//generateElementName(elename, "relation_cons", i, e.node2, 1);
		//SCIP_CALL( SCIPcreateConsBasicLinear(subscip, &cons, elename, 2, tmpvars, tmpvals, 0.0, SCIP_DEFAULT_INFINITY) );
		//SCIP_CALL( SCIPaddCons(subscip, cons) );
		//SCIP_CALL( SCIPreleaseCons(subscip, &cons) );
	}
	SCIPfreeBufferArray(subscip, &tmpvars);
	SCIPfreeBufferArray(subscip, &tmpvals);

	// add edge-node relation constraints [linking constraints]
	//tmpvars = NULL;
	//tmpvals = NULL;
	//SCIP_CALL( SCIPallocBufferArray(subscip, &tmpvars, 2) );
	//SCIP_CALL( SCIPallocBufferArray(subscip, &tmpvals, 2) );
	//for(int i = 0; i < nedges; ++i)
	//{
	//	MY_EDGE e = MY_GRAPHgetEdge(graph, i);

	//	tmpvars[0] = node_vars[e.node1];
	//	tmpvars[1] = edge_vars[RepFind(branchInfo_consdata -> rep, i)];
	//	tmpvals[0] = 1.0;
	//	tmpvals[1] = -1.0;
	//	assert(tmpvars[0] != NULL);
	//	assert(tmpvars[1] != NULL);
	//	generateElementName(elename, "relation_cons", i, e.node1, -1);
	//	SCIP_CALL( SCIPcreateConsBasicLinking(subscip, &cons, elename, tmpvars[1], tmpvars, tmpvals, 1) );
	//	SCIP_CALL( SCIPaddCons(subscip, cons) );
	//	SCIP_CALL( SCIPreleaseCons(subscip, &cons) );

	//	tmpvars[0] = node_vars[e.node2];
	//	tmpvars[1] = edge_vars[RepFind(branchInfo_consdata -> rep, i)];
	//	tmpvals[0] = 1.0;
	//	tmpvals[1] = -1.0;
	//	assert(tmpvars[0] != NULL);
	//	assert(tmpvars[1] != NULL);
	//	generateElementName(elename, "relation_cons", i, e.node2, -1);
	//	SCIP_CALL( SCIPcreateConsBasicLinking(subscip, &cons, elename, tmpvars[1], tmpvars, tmpvals, 1) );
	//	SCIP_CALL( SCIPaddCons(subscip, cons) );
	//	SCIP_CALL( SCIPreleaseCons(subscip, &cons) );
	//}
	//SCIPfreeBufferArray(subscip, &tmpvars);
	//SCIPfreeBufferArray(subscip, &tmpvals);

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
	//assert(tmpsize == nedgevars);
	capacity = ceil((double)(probdata -> alpha) * nedges / (probdata -> nparts));
	SCIP_CALL( SCIPcreateConsBasicKnapsack(subscip, &cons, "size_cons", tmpsize, tmpvars, tmpvals1, capacity) );
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


	SCIP_CALL( subscipIncludeEventHdlrBestsol(subscip) );
	threshold = pricerdata -> pi[pricerdata -> constraintssize - 1];
	SCIP_CALL( setSubscipBestsolEventHdlrData(subscip, EVENTHDLR_NAME, threshold) );

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
	SCIP_Real* varsObjCoef; // the coefficent of edge variables in the objective
	IP_PRICER_PROBDATA* sub_probdata;
	int nedges;
	int nnodes;

	double threshold;

	SCIP_VAR** edge_vars;
	SCIP_VAR** node_vars;
	SCIP_VAR** vars;

#ifdef PRICER_DEBUG
	SCIPdebugMessage("Enter function: changIpPricerObjCoef \n");/////////////////////////////////////////////////////////////
#endif

	assert(subscip != NULL);
	assert(pricerdata != NULL);
	assert(branchInfo_consdata != NULL);

	sub_probdata = (IP_PRICER_PROBDATA*)SCIPgetProbData(subscip);
	nedges = sub_probdata -> nedges;
	nnodes = sub_probdata -> nnodes;
	edge_vars = sub_probdata -> edge_vars;
	node_vars = sub_probdata -> node_vars;

	setAddedVar(sub_probdata, FALSE);
	sub_probdata -> nVarAdd = 0;

	varsObjCoef = NULL;
	vars = NULL;
	SCIP_CALL( SCIPallocBufferArray(subscip, &varsObjCoef, nedges + nnodes) );
	SCIP_CALL( SCIPallocBufferArray(subscip, &vars, nedges + nnodes) );
	memset(varsObjCoef, 0, (nedges + nnodes) * sizeof(SCIP_Real));

	// construct coefficents
	for(int i = 0; i < nedges; ++i)
	{
		varsObjCoef[RepFind(branchInfo_consdata -> rep, i)] -= pricerdata -> pi[i];
		vars[i] = edge_vars[i];
	}

	// append the node var coef to the edgeVarsObjCoef

	for(int i = 0; i < nnodes; ++i)
	{
		varsObjCoef[nedges + i] = 1.0;
		vars[nedges + i] = node_vars[i];
	}

	SCIP_CALL( SCIPfreeReoptSolve(subscip) );

	// change the obj val of each edge variable
	SCIP_CALL( SCIPchgReoptObjective(subscip, SCIP_OBJSENSE_MINIMIZE, vars, varsObjCoef, nedges + nnodes) );

	SCIPfreeBufferArray(subscip, &varsObjCoef);
	SCIPfreeBufferArray(subscip, &vars);

	threshold = pricerdata -> pi[pricerdata -> constraintssize - 1];
	SCIP_CALL( setSubscipBestsolEventHdlrData(subscip, EVENTHDLR_NAME, threshold) );

	return SCIP_OKAY;
}