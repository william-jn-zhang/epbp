/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */

#ifndef __BRANCH_EDGE_H__
#define __BRANCH_EDGE_H__

#include "scip/scip.h"
#include "probdata_edgepartition.h"
#include "scip/cons_setppc.h"


extern
SCIP_RETCODE SCIPincludeEdgeBranchRule(
	SCIP* scip
	);

#endif