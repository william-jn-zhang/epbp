/**@file   probdata_partition.h
 * @brief  Problem data for edge partition problem
 * @author william-jn-zhang
 *
 * 
 */

#ifndef __SCIP_PROBDATA_EDGEPARTITION__
#define __SCIP_PROBDATA_EDGEPARTITION__

#include <assert.h>

#include "scip/scip.h"
#include "my_graph.h"
#include "scip/struct_var.h"
#include "scip/struct_cons.h"
#include "my_def.h"

#define MAX_NAME_LEN 1024

struct SCIP_ProbData
{
	MY_GRAPH*  graph;
	SCIP_CONS**     constraints;
	int             constraintssize;

	MY_GRAPH*  orgngraph;
	int nparts;
	int alpha;
	int nnodes;
	int nedges;

	SCIP_VAR** vars; //variables in the master problem, each variable corresponding to a set of edges.
	int maxnSet;     //the max size of the array of sets
	int nSets;       //the number of stored sets in the array
	int** setArray;  //the array that store the sets
	int* setsSize;   //store the size of each set


};

extern
SCIP_RETCODE SCIPprobdataEPPCreate(
	SCIP* scip,             //scip data structure
	const char* probname,   //problem name
	int nnodes,             //number of nodes in the graph
	int nedges,             //number of edges in the graph
	int nparts,              //required number of partition
	int alpha,              //a parameter that control the size of each partition
	int **edges             //array of start and end points of the edges
	);

/**
* the following two method provide an interface of adding a new variable 
* and its corresponding edge set into the problem data
*/
extern
SCIP_RETCODE SCIPprobdataADDVarSet(
	SCIP* scip,      // scip data structure
	int* setArray,   // the corresponding edge set of the new adding variable
	int nsetEdges,   // the number of edges in the setArray
	int* idx         // returning the index of the new adding element
	);

extern
SCIP_RETCODE SCIPprobdataADDVar(
	SCIP* scip,      // scip data structure
	SCIP_VAR* var,   // the new adding variable
	int idx          // the index of the new adding element
	);

extern
SCIP_RETCODE SCIPprobdataGETVarSet(
	SCIP* scip,
	int   idx,
	int** set,
	int*  setlength
	);

extern
SCIP_CONS* SCIPprobdataGETConstraint(
	SCIP* scip, 
	int edge
	);
#endif