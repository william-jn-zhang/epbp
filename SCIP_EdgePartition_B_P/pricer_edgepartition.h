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


extern
SCIP_RETCODE SCIPincludePricerEdgePartition(
	SCIP* scip
	);

#endif