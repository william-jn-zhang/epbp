/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */

#ifndef __SCIP_PRICER_EDGEPARTITION_H__
#define __SCIP_PRICER_EDGEPARTITION_H__

#include "probdata_edgepartition.h"
#include "my_def.h"

#define PRICER_NAME            "pricer_edgeparititon"
#define PRICER_DESC            "pricer for edge partition"
#define PRICER_PRIORITY        5000000
#define PRICER_DELAY           TRUE     /* only call pricer if all problem variables have non-negative reduced costs */


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

extern
SCIP_RETCODE SCIPincludePricerEdgePartition(
	SCIP* scip
	);

#endif