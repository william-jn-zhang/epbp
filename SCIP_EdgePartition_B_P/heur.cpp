/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */
#include <string.h>

#include "heur.h"
#include "probdata_edgepartition.h"
#include "scip/cons_setppc.h"
#include "scip/cons_knapsack.h"

#define HEUR_NAME             "init heuristic"
#define HEUR_DESC             "initial primal heuristic"
#define HEUR_DISPCHAR         't'
#define HEUR_PRIORITY         1
#define HEUR_FREQ             1
#define HEUR_FREQOFS          0
#define HEUR_MAXDEPTH         0
#define HEUR_TIMING           SCIP_HEURTIMING_BEFORENODE
#define HEUR_USESSUBSCIP      FALSE  /**< does the heuristic use a secondary SCIP instance? */

//static
SCIP_RETCODE initHeur(SCIP* scip, MY_GRAPH* graph, int nparts, int alpha, int** sol, int* obj_val)
{
	assert(scip    != NULL);
	assert(graph   != NULL);
	assert(sol     != NULL);
	assert(obj_val != NULL);

	int nedge = graph -> nedges;
	int nnode = graph -> nnodes;
	double _sizebound = (double)alpha * nedge / nparts;
	int sizebound;
	if(_sizebound - (int)_sizebound > 0)
	{
		sizebound = _sizebound + 1;
	}
	else
	{
		sizebound = _sizebound;
	}

	//get clique

	int* _sol = *sol;
	int _obj_val = 0;
	int** parts = NULL;
	int* part_nedge = NULL;
	SCIP_CALL( SCIPallocBufferArray(scip, &part_nedge, nparts) );
	memset(part_nedge, 0, nparts * sizeof(int));
	SCIP_CALL( SCIPallocBufferArray(scip, &parts, nparts) );
	for(int i = 0; i < nparts; ++i)
	{
		SCIP_CALL( SCIPallocBufferArray(scip, &(parts[i]), nnode) );
		memset(parts[i], 0, nnode * sizeof(int));
	}

	MY_EDGE e;
	int u;
	int v;
	int* cnt = NULL;
	SCIP_CALL( SCIPallocBufferArray(scip, &cnt, nparts) );
	for(int i = 0; i < nedge; ++i)
	{
		memset(cnt, 0, nparts * sizeof(int));
		e = MY_GRAPHgetEdge(graph, i);
		u = e.node1;
		v = e.node2;
		for(int j = 0; j < nparts; ++j)
		{
			if(parts[j][u] == 1) 
				cnt[j]++;
			if(parts[j][v] == 1)
				cnt[j]++;
		}
		int tmp_part = -1;
		int tmp_part_n = -1;
		for(int j = 0; j < nparts; ++j)
		{
			if(cnt[j] > tmp_part_n && part_nedge[j] < sizebound)
			{
				tmp_part = j;
				tmp_part_n = cnt[j];
			}
		}

		//put the edge i into the tmp_part
		parts[tmp_part][u] = 1;
		parts[tmp_part][v] = 1;
		_sol[i] = tmp_part;
		++part_nedge[tmp_part];
	}
	
	for(int i = 0; i < nparts; ++i)
	{
		for(int j = 0; j < nnode; ++j)
		{
			if(parts[i][j] != 0)
				++_obj_val;
		}
	}
	*obj_val = _obj_val;

	for(int i = 0; i < nparts; ++i)
	{
		SCIPfreeBufferArray(scip, &(parts[i]));
	}
	SCIPfreeBufferArray(scip, &parts);
	SCIPfreeBufferArray(scip, &part_nedge);
	SCIPfreeBufferArray(scip, &cnt);

	return SCIP_OKAY;
}

static 
SCIP_DECL_HEUREXEC(heurExecInit)
 {
	 assert(scip != NULL);

	 SCIP_SOL* sol;
	 SCIP_VAR* var;
	 SCIP_CONS** cons;
	 SCIP_PROBDATA* probdata;
	 SCIP_Bool stored;

	 MY_GRAPH* graph;

	 int* _sol = NULL;
	 int obj;

	 int nnodes;
	 int nedges;
	 int nparts;
	 int alpha;

	 int** parts = NULL;
	 int* length = NULL;
	 int* parts_n = NULL;
	 int coef;

	 int idx;

	 SCIPdebugMessage("//-------------start init heurisitc\n");

	 probdata = SCIPgetProbData(scip);
	 assert(probdata != NULL);

	 graph = probdata -> orgngraph;
	 assert(graph != NULL);

	 nnodes = graph -> nnodes;
	 nedges = graph -> nedges;
	 nparts = probdata -> nparts;
	 alpha = probdata -> alpha;


	 SCIP_CALL( SCIPallocBufferArray(scip, &_sol, nedges) );
	 SCIP_CALL( SCIPallocBufferArray(scip, &parts, nparts) );
	 for(int i = 0; i < nparts; ++i)
	 {
		 SCIP_CALL( SCIPallocBufferArray(scip, &(parts[i]), nedges) );
		 memset(parts[i], 0, nedges * sizeof(int));
	 }
	 SCIP_CALL( SCIPallocBufferArray(scip, &length, nparts) );
	 memset(length, 0, nparts * sizeof(int));
	 SCIP_CALL( SCIPallocBufferArray(scip, &parts_n, nnodes) );

	 *result = SCIP_DIDNOTFIND;

	 /* use greedy to find a solution */
	 SCIP_CALL( initHeur(scip, graph, nparts, alpha, &_sol, &obj) );

	 for(int i = 0; i < nedges; ++i)
	 {
		 parts[_sol[i]][length[_sol[i]]] = i;
		 ++length[_sol[i]];
	 }

	 /* 
	 * using heuristic solution to generate variables
	 * and then add variable into the corresponding constraints
	 */
	 for(int i = 0; i < nparts; ++i)
	 {
		 memset(parts_n, 0, nnodes * sizeof(int));
		 coef = 0;
		 for(int j = 0; j < length[i]; ++j)
		 {
			 MY_EDGE e = MY_GRAPHgetEdge(graph, parts[i][j]);
			 if(parts_n[e.node1] == 0)
			 {
				 parts_n[e.node1] = 1;
				 ++coef;
			 }
			 if(parts_n[e.node2] == 0)
			 {
				 parts_n[e.node2] = 1;
				 ++coef;
			 }
		 }

		 /* add an new variable */
		 if(coef != 0)
		 {
			 SCIP_CALL( SCIPprobdataADDVarSet(scip, parts[i], length[i], &idx) );
			 assert(idx != -1);
			 
			 //create variable
			 SCIP_CALL( SCIPcreateVar(scip, &var, NULL, 0.0, 1.0, coef, SCIP_VARTYPE_BINARY,
				 TRUE, TRUE, NULL, NULL, NULL, NULL, (SCIP_VARDATA*)(size_t)idx) );

			 SCIP_CALL( SCIPprobdataADDVar(scip, var, idx) );
			 SCIP_CALL( SCIPaddVar(scip, var) );
			 SCIP_CALL( SCIPchgVarUbLazy(scip, var, 1.0) );

			 // add variable into the corresponding constraints
			 for(int j = 0; j < length[i]; ++j)
			 {
				 SCIP_CALL( SCIPaddCoefSetppc(scip, probdata -> constraints[parts[i][j]], var) );
				 //another constraint
			 }

			 // add variable into the num-constraints
			 SCIP_CALL( SCIPaddCoefKnapsack(scip, probdata -> constraints[probdata -> constraintssize - 1], var, 1.0) );
		 }
	 }

	 //release tmp memory
	 for(int i = 0; i < nparts; ++i)
	 {
		 SCIPfreeBufferArray(scip, &(parts[i]));
	 }
	 SCIPfreeBufferArray(scip, &parts);
	 SCIPfreeBufferArray(scip, &parts_n);
	 SCIPfreeBufferArray(scip, &length);
	 SCIPfreeBufferArray(scip, &_sol);

	 //check and set solution
	 SCIP_CALL( SCIPcreateSol(scip, &sol, NULL) );
	 assert(sol != NULL);
	 for(int i = 0; i < probdata -> nSets; ++i)
	 {
		 SCIP_CALL( SCIPsetSolVal(scip, sol, probdata -> vars[i], 1.0) );
	 }
	 SCIP_CALL( SCIPtrySolFree(scip, &sol, TRUE, FALSE, FALSE, FALSE, FALSE, &stored) );
	 assert(stored);

	 *result = SCIP_FOUNDSOL;

	 SCIPdebugMessage("//-------------finish init heurisitc\n");

	 return SCIP_OKAY;
 }


SCIP_RETCODE SCIPincludeHeurInit(
	SCIP* scip
	)
{
	assert(scip != NULL);

	SCIP_HEUR* heur;
	heur = NULL;

	SCIP_CALL( SCIPincludeHeurBasic(scip, &heur, HEUR_NAME, HEUR_DESC, HEUR_DISPCHAR, HEUR_PRIORITY, HEUR_FREQ, HEUR_FREQOFS,
		HEUR_MAXDEPTH, HEUR_TIMING, HEUR_USESSUBSCIP, heurExecInit, NULL) );

	assert(heur != NULL);

	return SCIP_OKAY;
}