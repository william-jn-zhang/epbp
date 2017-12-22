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
#include "pricer_edgepartition.h"
#include "cons_branchinfo.h"
#include "branch_edge.h"
#include "my_def.h"

#include "scip/heur_actconsdiving.h"
#include "scip/heur_coefdiving.h"
#include "scip/heur_crossover.h"
#include "scip/heur_dins.h"
#include "scip/heur_feaspump.h"
#include "scip/heur_fixandinfer.h"
#include "scip/heur_fracdiving.h"
#include "scip/heur_guideddiving.h"
#include "scip/heur_intdiving.h"
#include "scip/heur_intshifting.h"
#include "scip/heur_linesearchdiving.h"
#include "scip/heur_localbranching.h"
#include "scip/heur_mutation.h"
#include "scip/heur_objpscostdiving.h"
#include "scip/heur_octane.h"
#include "scip/heur_oneopt.h"
#include "scip/heur_pscostdiving.h"
#include "scip/heur_rens.h"
#include "scip/heur_rins.h"
#include "scip/heur_rootsoldiving.h"
#include "scip/heur_rounding.h"
#include "scip/heur_shifting.h"
#include "scip/heur_simplerounding.h"
#include "scip/heur_trivial.h"
#include "scip/heur_trysol.h"
#include "scip/heur_twoopt.h"
#include "scip/heur_undercover.h"
#include "scip/heur_veclendiving.h"
#include "scip/heur_zirounding.h"

#define SCIP_DEBUG

int main(int argc, char** argv)
{
	SCIP* scip = NULL;
	SCIP_CALL( SCIPcreate(&scip) );
	assert(scip != NULL);
	SCIP_CALL( SCIPincludeReaderEPP(scip) );

	SCIP_CALL( SCIPincludeHeurInit(scip) );

	SCIP_CALL( SCIPincludeConshdlrBranchInfo(scip) );

	SCIP_CALL( SCIPincludePricerEdgePartition(scip) );

	SCIP_CALL( SCIPincludeEdgeBranchRule(scip) );

	SCIP_CALL( SCIPincludeDefaultPlugins(scip) );

#ifdef USEHEURS
	SCIP_CALL( SCIPincludeHeurActconsdiving(scip) );
	SCIP_CALL( SCIPincludeHeurCoefdiving(scip) );
	SCIP_CALL( SCIPincludeHeurCrossover(scip) );
	SCIP_CALL( SCIPincludeHeurDins(scip) );
	SCIP_CALL( SCIPincludeHeurFixandinfer(scip) );
	SCIP_CALL( SCIPincludeHeurFracdiving(scip) );
	SCIP_CALL( SCIPincludeHeurGuideddiving(scip) );
	SCIP_CALL( SCIPincludeHeurIntdiving(scip) );
	SCIP_CALL( SCIPincludeHeurIntshifting(scip) );
	SCIP_CALL( SCIPincludeHeurLinesearchdiving(scip) );
	SCIP_CALL( SCIPincludeHeurLocalbranching(scip) );
	SCIP_CALL( SCIPincludeHeurMutation(scip) );
	SCIP_CALL( SCIPincludeHeurObjpscostdiving(scip) );
	SCIP_CALL( SCIPincludeHeurOctane(scip) );
	SCIP_CALL( SCIPincludeHeurOneopt(scip) );
	SCIP_CALL( SCIPincludeHeurPscostdiving(scip) );
	SCIP_CALL( SCIPincludeHeurRens(scip) );
	SCIP_CALL( SCIPincludeHeurRins(scip) );
	SCIP_CALL( SCIPincludeHeurRootsoldiving(scip) );
	SCIP_CALL( SCIPincludeHeurRounding(scip) );
	SCIP_CALL( SCIPincludeHeurShifting(scip) );
	SCIP_CALL( SCIPincludeHeurSimplerounding(scip) );
	SCIP_CALL( SCIPincludeHeurTrivial(scip) );
	SCIP_CALL( SCIPincludeHeurTrySol(scip) );
	SCIP_CALL( SCIPincludeHeurTwoopt(scip) );
	SCIP_CALL( SCIPincludeHeurUndercover(scip) );
	SCIP_CALL( SCIPincludeHeurVeclendiving(scip) );
	SCIP_CALL( SCIPincludeHeurZirounding(scip) );
#endif


	SCIP_CALL( SCIPprocessShellArguments(scip, argc, argv, "edge_partition") );

	SCIP_CALL( SCIPfree(&scip) );

	BMScheckEmptyMemory();

	return 0;
}