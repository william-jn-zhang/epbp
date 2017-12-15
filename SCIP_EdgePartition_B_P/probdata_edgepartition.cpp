/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */
#include <string.h>

#include "scip/cons_setppc.h"
#include "scip/cons_knapsack.h"
#include "scip/cons_linear.h"

#include "probdata_edgepartition.h"

#define VARDEL_EVENTHDLR_NAME "probdata var del"
#define VARDEL_EVENTHDLR_DESC "event handler for variable deleted event"

/**
* release memory of problem data
*/
static
SCIP_DECL_PROBDELORIG(DelOrigProbdata)
{
	assert(scip != NULL);
	assert(probdata != NULL);

	/* release graph */
	//MYGRAPHdelGraph()

	/* release constraints */
	for(int i = 0; i < (*probdata) -> constraintssize; ++i)
	{
		SCIP_CALL( SCIPreleaseCons(scip, &((*probdata) -> constraints[i])) );
	}
	SCIPfreeBlockMemoryArray(scip, &((*probdata) -> constraints), (*probdata) -> constraintssize);

	/* release variable */
	for(int i = 0; i < (*probdata) -> nSets; ++i)
	{
		SCIP_CALL( SCIPreleaseVar(scip, &((*probdata) -> vars[i])) );
		SCIPfreeBlockMemoryArray(scip, &((*probdata) -> setArray[i]), (*probdata) -> setsSize[i]);
	}
	SCIPfreeBlockMemoryArray(scip, &((*probdata) -> vars), (*probdata) -> maxnSet);
	SCIPfreeBlockMemoryArray(scip, &((*probdata) -> setArray), (*probdata) -> maxnSet);
	SCIPfreeBlockMemoryArray(scip, &((*probdata) -> setsSize), (*probdata) -> maxnSet);
	
	SCIPfreeBlockMemory(scip, probdata);
	return SCIP_OKAY;
}

/**
* get transformed problem data
*/
static
SCIP_DECL_PROBTRANS(GetTransProbdata)
{
	assert(scip != NULL);
	assert(sourcedata != NULL);
	assert(targetdata != NULL);

	SCIP_CALL( SCIPallocBlockMemory(scip, targetdata) );

	/* shallow copy the graph */

	(*targetdata) -> orgngraph = sourcedata -> orgngraph;
	(*targetdata) -> graph = sourcedata -> graph;

	(*targetdata) -> alpha = sourcedata -> alpha;
	(*targetdata) -> nparts = sourcedata -> nparts;
	(*targetdata) -> nnodes = sourcedata -> nnodes;
	(*targetdata) -> nedges = sourcedata -> nedges;

	/* copy the constraints */

	(*targetdata) -> constraintssize = sourcedata -> constraintssize;
	SCIP_CALL( SCIPallocBlockMemoryArray(scip, &((*targetdata) -> constraints), (*targetdata) -> constraintssize) );
	SCIP_CALL( SCIPtransformConss(scip, (*targetdata) -> constraintssize, sourcedata -> constraints, (*targetdata) -> constraints) );

	/* copy the variables and corresponding sets */

	(*targetdata) -> maxnSet = sourcedata -> maxnSet;
	(*targetdata) -> nSets = sourcedata -> nSets;

	//allocate the array memory
	SCIP_CALL( SCIPallocBlockMemoryArray(scip, &((*targetdata) -> setsSize), (*targetdata) -> maxnSet) );
	SCIP_CALL( SCIPallocBlockMemoryArray(scip, &((*targetdata) -> vars), (*targetdata) -> maxnSet) );
	SCIP_CALL( SCIPallocBlockMemoryArray(scip, &((*targetdata) -> setArray), (*targetdata) -> maxnSet) );

	for(int i = 0; i < (*targetdata) -> nSets; ++i)
	{
		assert(sourcedata -> vars[i] != NULL);
		(*targetdata) -> setsSize[i] = sourcedata -> setsSize[i];
		SCIP_CALL( SCIPtransformVar(scip, sourcedata -> vars[i], &((*targetdata) -> vars[i])) );
		SCIP_CALL( SCIPallocBlockMemoryArray(scip, &((*targetdata) -> setArray[i]), (*targetdata) -> setsSize[i]) );
		for(int j = 0; j < (*targetdata) -> setsSize[i]; ++j)
		{
			(*targetdata) -> setArray[i][j] = sourcedata -> setArray[i][j];
		}
	}
	return SCIP_OKAY;
}

/**
* release memory of problem data
*/
static
SCIP_DECL_PROBDELTRANS(DelTransProbdata)
{
	return DelOrigProbdata(scip, probdata);
}

/**
* callback methods of event handler of delete variable
* DO:
* delete the corresponding variable in the array of variable
* delete the corresponding set int he set array
*/
static
SCIP_DECL_EVENTEXEC(eventExecProbdatavardeleted)
{
	SCIP_VAR* var;
	SCIP_PROBDATA* probdata;
	int idx;

	assert(SCIPeventGetType(event) == SCIP_EVENTTYPE_VARDELETED);
	var = SCIPeventGetVar(event);
	probdata = (SCIP_PROBDATA*) eventdata;

	assert(probdata != NULL);
	assert(var != NULL);

	/* get index of variable in stablesets array */  ////////////////注意vardata中存储的数据 
	idx = (int)(size_t) SCIPvarGetData(var);

	assert(probdata -> vars[idx] == var);

	//delete option

	//release variable
	SCIP_CALL( SCIPreleaseVar(scip, &(probdata -> vars[idx])) );

	//release corresponding set
	SCIPfreeBlockMemoryArray(scip, &(probdata -> setArray[idx]), probdata -> setsSize[idx]);

	//move all subsequent variables and corresponding sets to the front

	for(; idx < probdata -> nSets - 1; ++idx)
	{
		probdata -> setArray[idx] = probdata -> setArray[idx + 1];
		probdata -> setsSize[idx] = probdata -> setsSize[idx + 1];
		probdata -> vars[idx] = probdata -> vars[idx + 1];
		
		//modify the corresponding var data, write the new index of variable
		SCIPvarSetData(probdata -> vars[idx], (SCIP_VARDATA*) (size_t) idx);
	}
	probdata -> nSets --;
	return SCIP_OKAY;
}

/*
* generate element (var, cons) name
* param: eleName, i, j, k
* generate: eleName_i_j_k
* note that if j,k < 0, it will not put into result
*/
static
void generateElementName(char* result, const char* eleName, int i, int j, int k)
{
	char buf[MAX_NAME_LEN];
	strcpy(result, eleName);
	strcpy(buf, result);
	strcat(buf, "_%d");
	sprintf(result, buf, i);
	if(j >= 0)
	{
		strcpy(buf, result);
		strcat(buf, "_%d");
		sprintf(result, buf, j);
	}
	if(k >= 0)
	{
		strcpy(buf, result);
		strcat(buf, "_%d");
		sprintf(result, buf, k);
	}
}

SCIP_RETCODE ProbdataSetBasicData(
	SCIP* scip,             //scip data structure
	SCIP_PROBDATA* probdata,//scip problem data
	int nnodes,             //number of nodes in the graph
	int nedges,             //number of edges in the graph
	int nparts,              //required number of partition
	int alpha,              //a parameter that control the size of each partition
	int **edges             //array of start and end points of the edges
	)
{
	int i;
	/* create graph */

	MY_GRAPHcreateGraph(&(probdata -> orgngraph), nnodes, nedges, FALSE);

	//add all edges
	for(i = 0; i < nedges; ++i)
	{
		assert((edges[i][0] > 0) && (edges[i][0] <= nnodes));
		assert((edges[i][1] > 0) && (edges[i][1] <= nnodes));

		//change each node's index that ensure which is started from 0
		MY_GRAPHaddEdge(probdata -> orgngraph, edges[i][0] - 1, edges[i][1] - 1);
	}
	MY_GRAPHflush(probdata -> orgngraph);

	/* add info */

	probdata -> nedges = nedges;
	probdata -> nnodes = nnodes;
	probdata -> nparts = nparts;
	probdata -> alpha = alpha;

	return SCIP_OKAY;
}

/**
* create edge partition problem data
*/
SCIP_RETCODE SCIPprobdataEPPCreate(
	SCIP* scip,             //scip data structure
	const char* probname,   //problem name
	int nnodes,             //number of nodes in the graph
	int nedges,             //number of edges in the graph
	int nparts,              //required number of partition
	int alpha,              //a parameter that control the size of each partition
	int **edges             //array of start and end points of the edges
	)
{
	SCIP_PROBDATA* probdata = NULL;
	int i;
	int j;
	
	assert(nnodes > 0);
	assert(nedges > 0);
	
	//allocate memory
	SCIP_CALL( SCIPallocBlockMemory(scip, &probdata) );

	SCIP_CALL( ProbdataSetBasicData(scip, probdata, nnodes, nedges, nparts, alpha, edges) );


	/* allocate the array memory */

	probdata -> constraintssize = nedges;
	SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(probdata -> constraints), probdata -> constraintssize) );

	/* allocate the initial memory for set array */

	probdata -> maxnSet = 2 * nparts;

	SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(probdata -> setsSize), probdata -> maxnSet) );
	SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(probdata -> setArray), probdata -> maxnSet) );
	SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(probdata -> vars), probdata -> maxnSet) );

	probdata -> nSets = 0;

	/* include the [variable delete] event handler into scip */

	SCIP_CALL( SCIPincludeEventhdlrBasic(scip, NULL, VARDEL_EVENTHDLR_NAME, VARDEL_EVENTHDLR_DESC,
		eventExecProbdatavardeleted, NULL) );

	/* create problem in scip */
	SCIP_CALL( SCIPcreateProbBasic(scip, probname) );
	SCIP_CALL( SCIPsetProbTrans(scip, GetTransProbdata) );
	SCIP_CALL( SCIPsetProbDelorig(scip, DelOrigProbdata) );
	SCIP_CALL( SCIPsetProbDeltrans(scip, DelTransProbdata) );

#ifndef TEST_READ
	//create constraints
	for(int i = 0; i < probdata -> constraintssize; ++i)
	{
		char consname[MAX_NAME_LEN];
		generateElementName(consname, "edge_constraint", i, -1, -1);

		SCIP_CALL( SCIPcreateConsSetcover(scip, &(probdata -> constraints[i]), consname, 0, NULL, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, FALSE) );
		SCIP_CALL( SCIPaddCons(scip, probdata -> constraints[i]) );
		//decrease the reference count
		//SCIP_CALL( SCIPreleaseCons(scip, &(probdata -> constraints[i])) );
	}
#endif

	SCIP_CALL( SCIPsetProbData(scip, probdata) );

	return SCIP_OKAY;
}


SCIP_RETCODE SCIPprobdataADDVarSet(
	SCIP* scip,
	int* setArray,
	int nsetEdges,
	int* idx
	)
{
	SCIP_PROBDATA* probdata;
	int newsize;

	assert(scip != NULL);

	probdata = SCIPgetProbData(scip);
	assert(probdata != NULL);

	*idx = -1;

	/* check enough memory */
	if(probdata -> nSets >= probdata -> maxnSet)
	{
		newsize = SCIPcalcMemGrowSize(scip, probdata -> maxnSet + 1);
		assert(newsize > probdata -> maxnSet + 1);
		SCIP_CALL( SCIPreallocBlockMemoryArray(scip, &(probdata -> setArray), probdata -> maxnSet, newsize) );
		SCIP_CALL( SCIPreallocBlockMemoryArray(scip, &(probdata -> setsSize), probdata -> maxnSet, newsize) );
		SCIP_CALL( SCIPreallocBlockMemoryArray(scip, &(probdata -> vars), probdata -> maxnSet, newsize) );
		probdata -> maxnSet = newsize;
	}

	/* alloc memory for new set array */
	SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(probdata -> setArray[probdata -> nSets]), nsetEdges) );

	/* set corresponding values */
	probdata -> setsSize[probdata -> nSets] = nsetEdges;
	probdata -> vars[probdata -> nSets] = NULL;

	for(int i = 0; i < nsetEdges; ++i)
	{
		assert(setArray[i] >= 0);
		probdata -> setArray[probdata -> nSets][i] = setArray[i];
	}
	*idx = probdata -> nSets;

	++(probdata -> nSets);

	return SCIP_OKAY;
}


SCIP_RETCODE SCIPprobdataADDVar(
	SCIP* scip,
	SCIP_VAR* var,
	int idx
	)
{
	assert(scip != NULL);
	SCIP_PROBDATA* probdata;

	probdata = SCIPgetProbData(scip);
	assert(probdata != NULL);
	assert((idx >= 0) && (idx < probdata -> nSets));

	/* the usage of the following code is not sure */
	SCIP_CALL( SCIPcatchVarEvent(scip, var, SCIP_EVENTTYPE_VARDELETED, SCIPfindEventhdlr(scip, VARDEL_EVENTHDLR_NAME),
         (SCIP_EVENTDATA*) probdata, NULL) );

	probdata -> vars[idx] = var;

	return SCIP_OKAY;
}

SCIP_RETCODE SCIPprobdataGETVarSet(
	SCIP* scip,
	int   idx,
	int** set,
	int*  setlength
	)
{
	SCIP_PROBDATA* probdata;

	assert(scip != NULL);
	assert(idx >= 0);

	probdata = NULL;
	probdata = SCIPgetProbData(scip);
	assert(probdata != NULL);
	assert(idx < probdata ->nSets);

	*set = probdata -> setArray[idx];
	*setlength = probdata -> setsSize[idx];

	return SCIP_OKAY;
}

SCIP_CONS* SCIPprobdataGETConstraint(
	SCIP* scip, 
	int edge
	)
{
	SCIP_PROBDATA* probdata;
	
	assert(scip != NULL);
	probdata = SCIPgetProbData(scip);
	assert(probdata != NULL);
	assert(edge >= 0 && edge < probdata -> constraintssize);

	return probdata -> constraints[edge];
}

SCIP_Bool SCIPprobdataIsEdgeInSet(
	SCIP* scip,                    // scip data structure
	int idx,                       // the index of the corresponding variable/set
	int edge                       // the edge
	)
{
	SCIP_PROBDATA* probdata;
	int l;
	int u;
	int m;

	assert(scip != NULL);
	probdata = SCIPgetProbData(scip);
	assert(probdata != NULL);
	assert(edge >= 0 && edge < probdata -> nedges);
	assert(idx >= 0 && idx < probdata -> nSets);

	//bi-search

	l = 0;
	u = probdata -> setsSize[idx] - 1;
	while(l <= u)
	{
		m = (l + u) / 2;
		if(probdata -> setArray[idx][m] == edge)
		{
			return TRUE;
		}
		if(probdata -> setArray[idx][m] > edge)
		{
			l = m + 1;
		}
		if(probdata -> setArray[idx][m] < edge)
		{
			u = m - 1;
		}
	}
	return FALSE;

}