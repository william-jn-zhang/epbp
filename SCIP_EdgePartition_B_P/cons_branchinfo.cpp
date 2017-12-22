/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */

#include "cons_branchinfo.h"
#include "probdata_edgepartition.h"

int RepFind(
	int*                    rep,           // the union find set array
	int                     ele            // the element that need to find the representation
	)
{
	if (ele == rep[ele]) return ele;
	return rep[ele] = RepFind(rep, rep[ele]);
}

void RepUnion(
	int*                    rep,          // the union find set array
	int                     ele1,
	int                     ele2
	)
{
	int u = RepFind(rep, ele1), v = RepFind(rep, ele2);
	if (u == v) return;
	rep[u] = v;
}

SCIP_Bool RepSame(
	int*                    rep,
	int                     ele1,
	int                     ele2
	)
{
	return RepFind(rep, ele1) == RepFind(rep, ele2);
}

/*
int father[100000];

void init() {
	for (int i = 1; i < 100000; i++)
		father[i] = i;
}

int Find(int x) {
	if (x == father[x]) return x;
	return father[x] = Find(father[x]);
}

void unionSet(int x, int y) {
	int u = Find(x), v = Find(y);
	if (u == v) return;
	father[u] = v;
}

bool Same(int x, int y) {
	return Find(x) == Find(y);
}
*/

static
SCIP_DECL_CONSINITSOL(consInitSolBranchInfo)
{
	SCIP_CONSHDLRDATA* conshdlrData;

	SCIP_CONS*         rootcons;
	SCIP_CONSDATA*     rootconsdata;

	SCIP_PROBDATA*     probdata;

	assert(scip != NULL);
	assert(conshdlr != NULL);
	assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);

	conshdlrData = SCIPconshdlrGetData(conshdlr);
	assert(conshdlrData != NULL);

	probdata = SCIPgetProbData(scip);
	assert(probdata != NULL);

	/* init stack */
	SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(conshdlrData -> stack), conshdlrData -> maxstacksize) );

	/* create root cons */
	rootconsdata = NULL;
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

	// initialize rep
	rootconsdata -> nedges = probdata -> nedges;
	
	SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(rootconsdata -> rep), rootconsdata -> nedges) );

	for(int i = 0; i < rootconsdata -> nedges; ++i)
	{
		rootconsdata -> rep[i] = i;
	}

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

		//copy union find set infomation
		consdata -> nedges = fatherConsdata -> nedges;
		SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(consdata -> rep), consdata -> nedges) );
		for(int i = 0; i < consdata -> nedges; ++i)
		{
			consdata -> rep[i] = fatherConsdata -> rep[i];
		}

		// add SAME info to the union find set
		if(consdata -> type == BRANCH_CONSTYPE_SAME)
		{
			RepUnion(consdata -> rep, consdata -> edge1, consdata -> edge2);
		}

		if(consdata -> propagatedvars < SCIPgetNTotalVars(scip))
		{
			SCIP_CALL( SCIPrepropagateNode(scip, consdata -> stickingatnode) );
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

	SCIPfreeBlockMemoryArray(scip, &((*consdata) -> rep), (*consdata) -> nedges);

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

static
SCIP_DECL_CONSPROP(consPropBranchInfo)
{
	SCIP_CONSHDLRDATA* conshdlrData;
	SCIP_CONS*         cons;
	SCIP_CONSDATA*     consdata;
	SCIP_VAR*          var;
	SCIP_PROBDATA*     probdata;
	int*               set;
	int                setlength;
	int                nsets;
	int                i;
	int                propcount;

	assert(conshdlr != NULL);
	conshdlrData = SCIPconshdlrGetData(conshdlr);
	assert(conshdlrData != NULL);
	assert(conshdlrData -> stack != NULL);

	probdata = SCIPgetProbData(scip);
	assert(probdata != NULL);

	nsets = probdata ->nSets;
	*result = SCIP_DIDNOTFIND;
	propcount = 0;

	cons = conshdlrData -> stack[conshdlrData -> ncons - 1];
	consdata = SCIPconsGetData(cons);

	if(consdata -> type == BRANCH_CONSTYPE_DIFFER)
	{
		for(int i = 0; i < nsets; ++i)
		{
			if(!SCIPisFeasZero(scip, SCIPvarGetUbLocal(probdata -> vars[i])))
			{
				// if there exists a variable that contains both edge1 and edge2
				if(SCIPprobdataIsEdgeInSet(scip, i, consdata -> edge1) && SCIPprobdataIsEdgeInSet(scip, i, consdata -> edge2))
				{
					var = probdata -> vars[i];
					SCIP_CALL(SCIPchgVarUb(scip, var, 0.0));
					++propcount;
				}
			}
		}
	}

	if(consdata -> type == BRANCH_CONSTYPE_SAME)
	{
		for(int i = 0; i < nsets; ++i)
		{
			if(!SCIPisFeasZero(scip, SCIPvarGetUbLocal(probdata -> vars[i])))
			{
				// if there exists a variable that contains both edge1 and edge2
				if((SCIPprobdataIsEdgeInSet(scip, i, consdata -> edge1) || SCIPprobdataIsEdgeInSet(scip, i, consdata -> edge2))
					&& !(SCIPprobdataIsEdgeInSet(scip, i, consdata -> edge1) && SCIPprobdataIsEdgeInSet(scip, i, consdata -> edge2)))
				{
					var = probdata -> vars[i];
					SCIP_CALL(SCIPchgVarUb(scip, var, 0.0));
					++propcount;
				}
			}
		}
	}

	consdata -> propagatedvars = SCIPgetNTotalVars(scip);

	return SCIP_OKAY;
}

SCIP_RETCODE SCIPincludeConshdlrBranchInfo(
	SCIP* scip
	)
{
	SCIP_CONSHDLRDATA* conshdlrData;
	SCIP_CONSHDLR* conshdlr;

	conshdlrData = NULL;

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
	SCIP_CALL( SCIPsetConshdlrProp(scip, conshdlr, consPropBranchInfo, CONSHDLR_PROPFREQ, CONSHDLR_DELAYPROP,
         CONSHDLR_PROP_TIMING) );

	return SCIP_OKAY;
}

SCIP_CONS* SCIPconsGetActiveBranchInfoCons(
	SCIP* scip
	)
{
	SCIP_CONSHDLR* conshdlr;
	SCIP_CONSHDLRDATA* conshdlrData;

	assert(scip != NULL);
	conshdlr = SCIPfindConshdlr(scip, CONSHDLR_NAME);
	if(conshdlr == NULL)
	{
		return NULL;
	}
	conshdlrData = SCIPconshdlrGetData(conshdlr);
	assert(conshdlrData != NULL);
	assert(conshdlrData -> stack != NULL);
	assert(conshdlrData -> ncons > 0);

	return conshdlrData -> stack[conshdlrData -> ncons - 1];
}

SCIP_RETCODE SCIPconsCreateConsBranchInfo(
	SCIP*                   scip,          // scip data structure
	SCIP_CONS**              cons,          // the created constraint
	const char*             consname,      // name of constraint
	SCIP_CONS*              fathercons,    // the father of the current created constraint
	BRANCH_CONSTYPE         type,          // the type of the current branching
	int                     edge1,         // the first edge corresponding to this branch step 
	int                     edge2,         // the first edge corresponding to this branch step 
	SCIP_NODE*              stickingnode   // the B&B-tree node at which the constraint will be sticking
	)
{
	SCIP_CONSHDLR* conshdlr;
	SCIP_CONSDATA* consdata;
	int tmp;

	assert(scip != NULL);
	assert(fathercons != NULL);
	assert(type == BRANCH_CONSTYPE_DIFFER || type == BRANCH_CONSTYPE_SAME);
	assert(stickingnode != NULL);

	conshdlr = SCIPfindConshdlr(scip, CONSHDLR_NAME);
	if(conshdlr == NULL)
	{
		return SCIP_PLUGINNOTFOUND;
	}

	consdata = NULL;
	SCIP_CALL(SCIPallocBlockMemory(scip, &consdata));

	if(edge1 > edge2)
	{
		tmp = edge1;
		edge1 = edge2;
		edge2 = tmp;
	}
	consdata -> edge1 = edge1;
	consdata -> edge2 = edge2;
	consdata -> type = type;
	consdata -> fathercons = fathercons;
	consdata -> stickingatnode = stickingnode;

	consdata -> differ_branch = NULL;
	consdata -> same_branch = NULL;
	consdata -> ndiffer = 0; 
	consdata -> nsame = 0;

	SCIP_CALL( SCIPcreateCons(scip, cons, consname, conshdlr, consdata, FALSE, FALSE, FALSE, FALSE, TRUE,
         TRUE, FALSE, TRUE, FALSE, TRUE) );

	return SCIP_OKAY;
}