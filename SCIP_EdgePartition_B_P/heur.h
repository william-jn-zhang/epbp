/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */
#ifndef __HEUR_H__
#define __HEUR_H__

#include <assert.h>

#include "my_graph.h"
#include "scip/scip.h"
#include "my_def.h"


/**
* the heuristic that use to give an initial solution of the problem
* PARAM:
* scip: the scip environment
* graph: input graph
* nparts: the number of partition that the graph needs to be divided
* RETURN:
* sol: a pointer of a solution array, 
* obj_val: a pointer of a variable of object value
*
* NOTICE:
* the solution array should alloc memory before put into this method
*/
//static
extern
SCIP_RETCODE initHeur(SCIP* scip, MY_GRAPH* graph, int nparts, int alpha, int** sol, int* obj_val);


extern
SCIP_RETCODE SCIPincludeHeurInit(
	SCIP* scip
	);
#endif