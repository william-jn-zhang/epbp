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

#define PRICER_NAME            "pricer_edgeparititon"
#define PRICER_DESC            "pricer for edge partition"
#define PRICER_PRIORITY        5000000
#define PRICER_DELAY           TRUE     /* only call pricer if all problem variables have non-negative reduced costs */

struct SCIP_PricerData
{
	SCIP*                                scip;                   // scip data structure
	int                                  maxvarsround;           // the maximum number of variable created in each round
	int                                  oldmaxvarsround;        // 
};

static
SCIP_DECL_PRICERREDCOST(pricerRedcostEdgePartition)
{

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

	return SCIP_OKAY;
}