/**@file   reader_epp.h
 * @brief  Edge partition problem file reader
 * @author william-jn-zhang
 *
 * 
 */

#ifndef __SCIP_READER_EPP_H__
#define __SCIP_READER_EPP_H__

#include "scip/scip.h"
#include "my_def.h"

extern
SCIP_RETCODE SCIPincludeReaderEPP(
	SCIP* scip //scip data structure
);


/* test method */
//static
extern
SCIP_RETCODE readEPP(
	SCIP* scip,
	const char* filename
);

#endif

