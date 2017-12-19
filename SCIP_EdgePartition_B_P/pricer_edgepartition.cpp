/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */

#include <assert.h>
#include <string.h>

#include "pricer_edgepartition.h"
#include "cons_branchinfo.h"
#include "scip/cons_setppc.h"
#include "scip/cons_knapsack.h"

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

	}

	return SCIP_OKAY;
}

SCIP_RETCODE SCIPincludePricerEdgePartition(
	SCIP* scip
	)
{
	SCIP_PRICERDATA* pricerdata;
	SCIP_PRICER* pricer;
	assert(scip != NULL);

	SCIP_CALL( SCIPallocBlockMemory(scip, &pricerdata) );
	pricerdata -> scip = scip;
	pricerdata -> maxvarsround = 0;
	pricerdata -> oldmaxvarsround = 0;

	pricer = NULL;

	/* include variable pricer */
	SCIP_CALL( SCIPincludePricerBasic(scip, &pricer, PRICER_NAME, PRICER_DESC, PRICER_PRIORITY, PRICER_DELAY,
         pricerRedcostEdgePartition, NULL, pricerdata) );

	SCIP_CALL( SCIPsetPricerInitsol(scip, pricer, pricerInitSolEdgepartition) );
	SCIP_CALL( SCIPsetPricerExitsol(scip, pricer, pricerExitSolEdgepartition) );

	return SCIP_OKAY;
}