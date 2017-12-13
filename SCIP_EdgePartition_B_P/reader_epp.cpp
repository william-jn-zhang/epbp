/**@file   reader_epp.cpp
 * @brief  Edge partition problem file reader
 * @author william-jn-zhang
 *
 * 
 */

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "scip/cons_setppc.h"

#include "reader_epp.h"
#include "probdata_edgepartition.h"


#define READER_NAME         "epp_reader"
#define READER_DESC	        "file reader for edge partition problem"
#define READER_EXTENSION    "epp"

#define MAX_EPP_FILENAME_LEN    1024

/** get next number from string s */
static inline
int getNextNumber(
   char**                s,                   /**< pointer to the pointer of the current position in the string */
   int* ret
   )
{
  long tmp;
  /* skip whitespaces */
  while ( isspace(**s) )
    ++(*s);
  if(**s == 0)
	  return 0;// end of the buffer
  /* read number */
  tmp = atol(*s);
  *ret = (int)tmp;
  /* skip whitespaces */
  while ( (**s != 0) && (!isspace(**s)) )
    ++(*s);
  return 1;//not end of the buffer
}

//static
SCIP_RETCODE readEPP(
	SCIP* scip,
	const char* filename
)
{
	SCIP_FILE* fp;
	char* probname = NULL;

	char* buf = NULL;
	char fnbuf[MAX_EPP_FILENAME_LEN];
	int nnodes;
	int nedges;
	int nparts;
	int alpha;
	int** edges = NULL;

	int i;
	int j;
	
	assert(scip != NULL);
	assert(filename != NULL);

	if(NULL == (fp = SCIPfopen(filename, "r")))
	{
		SCIPerrorMessage("cannot open file <%s> for reading\n", filename);
		  perror(filename);
		  return SCIP_NOFILE;
	}

	/*
	if(SCIPfgets(fnbuf, (int)sizeof(fnbuf), fp) == NULL)
		return SCIP_READERROR;
	*/

	/*generate problem name from filename*/
	i = 1;
	while ( (filename[i] != '/') && (filename[i] != '\0') )
	{
		i++;
	}
	if ( filename[i] != '/' )
	{
		j = i;
		i = -1;
	}
	else
	{
		j = i+1;
		while ( filename[i] == '/' && filename[j] != '\0' )
		{
			j = i+1;
			while ( filename[j] != '\0' )
			{
			j++;
			if ( filename[j] == '/' )
			{
				i = j;
				break;
			}
			}
		}
	}

	if( j-i-4 <= 0 )
		return SCIP_READERROR;

	SCIP_CALL( SCIPallocBufferArray(scip, &probname, (j-i-4)) );
	strncpy(probname, &filename[i+1], (j-i-5)); /*lint !e732 !e776*/
	probname[j-i-5]= '\0';

	//skip the comment line
	do
	{
		SCIPfgets(fnbuf, (int)sizeof(fnbuf), fp);
	}
	while(!SCIPfeof(fp) && (fnbuf[0] == '%'));
	
	//get the first line [nnodes nedges]
	sscanf(fnbuf, "%d %d", &nnodes, &nedges);
   
	//allocate the main buffer
	int bufsize = 2 * nnodes + 3;
	SCIP_CALL( SCIPallocBufferArray(scip, &buf, bufsize) );
   
	//get #nparts and #alpha parameters
	SCIPfgets(buf, bufsize, fp);
	sscanf(buf, "%%nparts %d", &nparts);
	SCIPfgets(buf, bufsize, fp);
	sscanf(buf, "%%alpha %d", &alpha);

	/*according to the file format of dimacs, node id is begining from 1*/
   
	//allocate memory for edges
	SCIP_CALL( SCIPallocBufferArray(scip, &edges, nedges) );
	for(i = 0; i < nedges; ++i)
	{
		SCIP_CALL( SCIPallocBufferArray(scip, &(edges[i]), 2) );
	}
	i = 0;
	int nodei = 1;
	int nodej = 0;
	while(!SCIPfeof(fp))
	{
		SCIPfgets(buf, bufsize, fp);
		if(buf[0] == '%')
			continue;
		char* p = buf;
		while(getNextNumber(&p, &nodej))
		{
			if(nodei < nodej){//indirected graph
				edges[i][0] = nodei;
				edges[i][1] = nodej;
				++i;
			}
		}
		++nodei;
	}
	if(i != nedges)
	{
		SCIPerrorMessage("incorrect number of edges: expected %d many, but got %d many\n", nedges, i);
		return SCIP_ERROR;
	}

	//create problem data
	SCIP_CALL( SCIPprobdataEPPCreate(scip, probname, nnodes, nedges, nparts, alpha, edges) );

	//clean up
	for(i = 0; i < nedges; ++i)
	{
		SCIPfreeBufferArray(scip, &(edges[i]));
	}
	SCIPfreeBufferArray(scip, &edges);
	SCIPfreeBufferArray(scip, &probname);
	SCIPfreeBufferArray(scip, &buf);
	SCIPfclose(fp);


	return SCIP_OKAY;
}

static
SCIP_DECL_READERREAD(readerReadEpp){
	assert(reader != NULL);
	assert(strcmp(SCIPreaderGetName(reader), READER_NAME) == 0);
	assert(scip != NULL);
	assert(result != NULL);

	SCIP_CALL( readEPP(scip, filename) );

	*result = SCIP_SUCCESS;

	return SCIP_OKAY;
}

SCIP_RETCODE SCIPincludeReaderEPP(
	SCIP* scip
	)
{
	SCIP_READERDATA* readerdata;
	SCIP_READER* reader;

	readerdata = NULL;

	SCIP_CALL( SCIPincludeReaderBasic(scip, &reader, READER_NAME, READER_DESC, READER_EXTENSION, readerdata) );
	assert(reader != NULL);
	SCIP_CALL( SCIPsetReaderRead(scip, reader, readerReadEpp) );

	return SCIP_OKAY;
}