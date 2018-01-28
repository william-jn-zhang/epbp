/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */

#include "scip/scip.h"
#include "scip/scipdefplugins.h"

#include "hash_debugger.h"

#include "heur.h"
#include "my_graph.h"
#include "reader_epp.h"
#include "probdata_edgepartition.h"
#include <stdio.h>

#define MAX_LEN 1024

int main1()
{
	SCIP* scip = NULL;
	SCIP_CALL( SCIPcreate(&scip) );
	assert(scip != NULL);

	SCIP_CALL( SCIPincludeDefaultPlugins(scip) );
	char filename[MAX_LEN];
	scanf("%s", &filename);
	readEPP(scip, filename);

	SCIP_PROBDATA* probdata = NULL;
	probdata = SCIPgetProbData(scip);

	MY_GRAPH *graph = probdata -> orgngraph;

	int nedges = MY_GRAPHgetNedges(graph);
	int nnodes = MY_GRAPHgetNnodes(graph);
	int nparts = probdata -> nparts;
	int alpha = probdata -> alpha;

	int* sol = NULL;
	SCIP_CALL( SCIPallocBufferArray(scip, &sol, nedges) );

	int obj = 0;

	initHeur(scip, graph, nparts, alpha, &sol, &obj);

	for(int i = 0; i < nedges; ++i)
	{
		printf("%d ", sol[i]);
	}
	printf("\n");

	SCIPfreeBufferArray(scip, &sol);

	return 0;
}
