/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */

#include "cons_branchinfo.h"

static
SCIP_DECL_CONSINITSOL(consInitSolBranchInfo)
{
	SCIP_CONSHDLRDATA* conshdlrData;

	SCIP_CONS*         rootcons;
	SCIP_CONSDATA*     rootconsdata;

	assert(scip != NULL);
	assert(conshdlr != NULL);
	assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);

	conshdlrData = SCIPconshdlrGetData(conshdlr);
	assert(conshdlrData != NULL);

	/* init stack */
	SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(conshdlrData -> stack), conshdlrData -> maxstacksize) );

	/* create root cons */
	SCIP_CALL( SCIPallocBlockMemory(scip, &rootconsdata) );
	rootconsdata -> nsame = 0;
	rootconsdata -> ndiffer = 0;
	rootconsdata -> same_branch = NULL;
	rootconsdata -> differ_branch = NULL;
	//SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(rootconsdata -> same_branch), rootconsdata -> nsame) );
	//SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(rootconsdata -> differ_branch), rootconsdata -> ndiffer) );
	rootconsdata -> edge1 = -1;
	rootconsdata -> edge2 = -1;
	rootconsdata -> type = BRANCH_CONSTYPE_ROOT;
	rootconsdata -> fathercons = NULL;
	rootconsdata -> stickingatnode = NULL;
	SCIP_CALL( SCIPcreateCons(scip, &rootcons, "root", conshdlr, rootconsdata, FALSE, FALSE, FALSE, FALSE, FALSE,
			     TRUE, FALSE, TRUE, FALSE, FALSE));

	//note that the generated root branch info cons does not add into the scip object

	//add root branch info constraint into stack
	conshdlrData -> stack[0] = rootcons;
	conshdlrData -> ncons = 1;

	return SCIP_OKAY;
}


static
SCIP_DECL_CONSACTIVE(consActiveBranchInfo)
{
	SCIP_CONSHDLRDATA* conshdlrData;
	SCIP_CONSDATA*     consdata;
	SCIP_CONSDATA*     fatherConsdata;
	
	int tmp;

	assert(conshdlr != NULL);
	assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
	assert(cons != NULL);

	conshdlrData = SCIPconshdlrGetData(conshdlr);
	assert(conshdlrData != NULL);
	assert(conshdlrData -> stack != NULL);

	consdata = SCIPconsGetData(cons);
	assert(consdata != NULL);
	assert((consdata -> type == BRANCH_CONSTYPE_ROOT) || (consdata -> fathercons != NULL));

	/* put the constraint on the stack */
	if(conshdlrData -> ncons >= conshdlrData -> maxstacksize) // allocate larger memory
	{
		SCIP_Real newsize = SCIPcalcMemGrowSize(scip, conshdlrData -> maxstacksize);
		SCIP_CALL( SCIPreallocBlockMemoryArray(scip, &(conshdlrData -> stack), conshdlrData -> maxstacksize, newsize) );
		conshdlrData -> maxstacksize = newsize;
	}
	//put the active constraint into the stack
	conshdlrData -> stack[conshdlrData -> ncons] = cons;
	++(conshdlrData -> ncons);

	if(consdata -> type != BRANCH_CONSTYPE_ROOT)
	{
		fatherConsdata = SCIPconsGetData(consdata -> fathercons);
		assert(fatherConsdata != NULL);

		// alloc memory for the SAME and DIFFER info array
		if(consdata -> type == BRANCH_CONSTYPE_SAME)
		{
			consdata -> nsame = fatherConsdata -> nsame + 1;
			consdata -> ndiffer = fatherConsdata -> ndiffer;
			SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(consdata -> same_branch), 2 * (consdata -> nsame)) );
			SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(consdata -> differ_branch), 2 * (consdata -> ndiffer)) );
			if(consdata -> edge1 > consdata -> edge2)
			{
				tmp = consdata -> edge1;
				consdata -> edge1 = consdata -> edge2;
				consdata -> edge2 = tmp;
			}
			consdata -> same_branch[consdata -> nsame - 1][0] = consdata -> edge1;
			consdata -> same_branch[consdata -> nsame - 1][1] = consdata -> edge2;
		}
		if(consdata -> type == BRANCH_CONSTYPE_DIFFER)
		{
			consdata -> nsame = fatherConsdata -> nsame;
			consdata -> ndiffer = fatherConsdata -> ndiffer + 1;
			SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(consdata -> differ_branch), 2 * (consdata -> ndiffer)) );
			SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(consdata -> same_branch), 2 * (consdata -> nsame)) );
			if(consdata -> edge1 > consdata -> edge2)
			{
				tmp = consdata -> edge1;
				consdata -> edge1 = consdata -> edge2;
				consdata -> edge2 = tmp;
			}
			consdata -> differ_branch[consdata -> ndiffer - 1][0] = consdata -> edge1;
			consdata -> differ_branch[consdata -> ndiffer - 1][1] = consdata -> edge2;
		}

		// copy old branch infomation
		for(int i = 0; i < fatherConsdata ->ndiffer; ++i)
		{
			consdata -> differ_branch[i][0] = fatherConsdata -> differ_branch[i][0];
			consdata -> differ_branch[i][1] = fatherConsdata -> differ_branch[i][1];
		}
		for(int i = 0; i < fatherConsdata -> nsame; ++i)
		{
			consdata -> same_branch[i][0] = fatherConsdata -> same_branch[i][0];
			consdata -> same_branch[i][1] = fatherConsdata -> same_branch[i][1];
		}
	}

	return SCIP_OKAY;
}

static
SCIP_DECL_CONSDEACTIVE(consDeactiveBranchInfo)
{
	SCIP_CONSHDLRDATA* conshdlrData;
	//SCIP_CONSDATA*     consdata;

	assert(scip != NULL);
	assert(conshdlr != NULL);
	assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
	assert(cons != NULL);

	conshdlrData = SCIPconshdlrGetData(conshdlr);
	assert(conshdlrData != NULL);
	assert(conshdlrData -> stack != NULL);
	assert(conshdlrData -> ncons > 0);
	assert(cons == conshdlrData -> stack[conshdlrData -> ncons - 1]);

	--(conshdlrData -> ncons);

	return SCIP_OKAY;
}

/* free constraint data */
static
SCIP_DECL_CONSDELETE(consDeleteBranchInfo)
{
	assert(scip != NULL);
	assert(conshdlr != NULL);
	assert(cons != NULL);
	assert((consdata != NULL) && (*consdata != NULL));
	assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);

	assert((*consdata) -> same_branch != NULL);
	assert((*consdata) -> differ_branch != NULL);

	SCIPfreeBlockMemoryArray(scip, &((*consdata) -> same_branch), 2 * ((*consdata) -> nsame));
	SCIPfreeBlockMemoryArray(scip, &((*consdata) -> differ_branch), 2 * ((*consdata) -> ndiffer));

	SCIPfreeBlockMemory(scip, consdata);

	return SCIP_OKAY;
}

static
SCIP_DECL_CONSFREE(consFreeBranchInfoHdlr)
{
	SCIP_CONSHDLRDATA* conshdlrData;

	assert(scip != NULL);
	assert(conshdlr != NULL);
	assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);

	conshdlrData = SCIPconshdlrGetData(conshdlr);
	assert(conshdlrData != NULL);

	assert(conshdlrData -> stack == NULL);

	SCIPfreeBlockMemory(scip, &conshdlrData);

	return SCIP_OKAY;
}

static
SCIP_DECL_CONSEXITSOL(consExitsolBranchInfo)
{
	SCIP_CONSHDLRDATA* conshdlrData;

	assert(scip != NULL);
	assert(conshdlr != NULL);
	assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);

	conshdlrData = SCIPconshdlrGetData(conshdlr);
	assert(conshdlrData != NULL);
	assert(conshdlrData -> ncons == 1);
	SCIP_CALL( SCIPreleaseCons(scip, &(conshdlrData -> stack[0])) );
	conshdlrData -> stack[0] = NULL;

	SCIPfreeBlockMemoryArray(scip, &conshdlrData -> stack, conshdlrData -> maxstacksize);

	return SCIP_OKAY;
}

SCIP_RETCODE SCIPincludeConshdlrBranchInfo(
	SCIP* scip
	)
{
	SCIP_CONSHDLRDATA* conshdlrData;
	SCIP_CONSHDLR* conshdlr;

	SCIP_CALL( SCIPallocBlockMemory(scip, &conshdlrData) );
	conshdlrData -> stack = NULL;
	conshdlrData -> maxstacksize = 100;
	conshdlrData -> ncons = 0;

	conshdlr = NULL;
	/* include constraint handler */
    /*SCIP_CALL( SCIPincludeConshdlrBasic(scip, &conshdlr, CONSHDLR_NAME, CONSHDLR_DESC,
         CONSHDLR_ENFOPRIORITY, CONSHDLR_CHECKPRIORITY, CONSHDLR_EAGERFREQ, CONSHDLR_NEEDSCONS,
         consEnfolpStoreGraph, consEnfopsStoreGraph, consCheckStoreGraph, consLockStoreGraph,
         conshdlrData) );*/
	SCIP_CALL( SCIPincludeConshdlrBasic(scip, &conshdlr, CONSHDLR_NAME, CONSHDLR_DESC,
         CONSHDLR_ENFOPRIORITY, CONSHDLR_CHECKPRIORITY, CONSHDLR_EAGERFREQ, CONSHDLR_NEEDSCONS,
         NULL, NULL, NULL, NULL, /////////////////////////////////these method need to be added
         conshdlrData) );
	assert(conshdlr != NULL);

	SCIP_CALL( SCIPsetConshdlrInitsol(scip, conshdlr, consInitSolBranchInfo) );
	SCIP_CALL( SCIPsetConshdlrActive(scip, conshdlr, consActiveBranchInfo) );
	SCIP_CALL( SCIPsetConshdlrDeactive(scip, conshdlr, consDeactiveBranchInfo) );
	SCIP_CALL( SCIPsetConshdlrDelete(scip, conshdlr, consDeleteBranchInfo) );
	SCIP_CALL( SCIPsetConshdlrFree(scip, conshdlr, consFreeBranchInfoHdlr) );
	SCIP_CALL( SCIPsetConshdlrExitsol(scip, conshdlr, consExitsolBranchInfo) );

	return SCIP_OKAY;
}