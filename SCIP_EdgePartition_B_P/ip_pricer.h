/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */

#ifndef __IP_PRICER_H__
#define __IP_PRICER_H__

#include "scip\scip.h"
#include "pricer_edgepartition.h"



extern
SCIP_RETCODE createIpPricerProblem(
	SCIP**                subscip,     // the created subscip instance
	SCIP_PRICERDATA*      pricerdata,  // pricer information, contain the dual val of master problem
	SCIP_PROBDATA*        master_probdata, // the probdata of master problem
	SCIP_CONSDATA*        branchInfo_consdata // the consData of branch_info constraint
);

extern
SCIP_RETCODE changIpPricerObjCoef(
	SCIP*                 subscip,     // the created subscip instance
	SCIP_PRICERDATA*      pricerdata,   // pricer information, contain the dual val of master problem
	SCIP_CONSDATA*        branchInfo_consdata // the consData of branch_info constraint
);

#endif