/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */


#include "scip/scip.h"
#include "scip/scipshell.h"
#include "scip/scipdefplugins.h"

#include "reader_epp.h"
#include "probdata_edgepartition.h"
#include "heur.h"

#define SCIP_DEBUG

int main(int argc, char** argv)
{
	SCIP* scip = NULL;
	SCIP_CALL( SCIPcreate(&scip) );
	assert(scip != NULL);
	SCIP_CALL( SCIPincludeReaderEPP(scip) );

	SCIP_CALL( SCIPincludeHeurInit(scip) );

	SCIP_CALL( SCIPincludeDefaultPlugins(scip) );

	SCIP_CALL( SCIPprocessShellArguments(scip, argc, argv, "edge_partition") );

	SCIP_CALL( SCIPfree(&scip) );

	BMScheckEmptyMemory();

	return 0;
}