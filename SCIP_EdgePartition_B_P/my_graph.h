/**@file   my_graph.h
 * @brief  data structure for graph
 * @author william-jn-zhang
 *
 * node index is started from 0 
 * edge index is started from 0
 *
 * NOTICE:
 * you should ensure that the indexes of elements is started from 0 !!
 */

#ifndef __MY_GRAPH_H__
#define __MY_GRAPH_H__

#include "my_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef double GRAPH_NODE_WEIGHT;
typedef double GRAPH_EDGE_WEIGHT;


/*
* the node link to which node by which edge
*/
struct _LINK
{
	int edge;
	int link;
};

typedef _LINK LINK_TO;
typedef _LINK LINK_FROM;

/* 
* store the incident nodes of this edge 
* default: node1 < node2
*/
struct MY_edge
{
	/* 
	store the incident nodes of this edge 
	default: node1 < node2
	*/
	int node1;
	int node2;
};

typedef MY_edge MY_EDGE;

struct MY_graph
{
	bool						is_directed_graph; //indicate whether this graph is directed graph
													//0: undirected
													//1: directed
	bool						flushed;// to memory whether this graph has been flushed?

	int							nnodes;
	int							nedges;

	GRAPH_NODE_WEIGHT*			nodeweights;
	GRAPH_EDGE_WEIGHT*			edgeweights;

	int*						degrees_OUT; //store the out degree when graph is directed
											 //when graph is undirected, store the whole degrees
	int*						degrees_IN; // only used when the graph is directed

	/* store information for each node */
	LINK_TO**					node_incident_TO;
	LINK_FROM**					node_incident_FROM; // only used when the graph is directed

	/* store information for each edge */
	MY_edge*					edges;

	int							_edge_cnt;
};

typedef MY_graph MY_GRAPH;


/*
* init the graph data structure
*/
extern
void MY_GRAPHcreateGraph(
	MY_GRAPH** graph, 
	int nnodes, 
	int nedges,
	bool is_directed_graph
	);


/*
* and an edge <node1, node2> into the graph,
*
*
* NOTICE:
* 1. this method does not check the duplication of edges.
*    for undirected graph (node1, node2) and (node2, node1) is the same edge
*
* 2. you should ensure that the indexes of elements (edges and nodes) is started from 0 !!
*/
extern
void MY_GRAPHaddEdgeWithEdgeNodeWeight(
	MY_GRAPH* graph,
	int node1,
	int node2,
	GRAPH_NODE_WEIGHT node1_weight,
	GRAPH_NODE_WEIGHT node2_weight,
	GRAPH_EDGE_WEIGHT edge_weight
	);


/*
* and an edge <node1, node2> into the graph,
*
* this method does not check the duplication of edges.
*
* NOTICE:
* you should ensure that the indexes of elements (edges and nodes) is started from 0 !!
*/
extern
void MY_GRAPHaddEdgeWithNodeWeight(
	MY_GRAPH* graph,
	int node1,
	int node2,
	GRAPH_NODE_WEIGHT node1_weight,
	GRAPH_NODE_WEIGHT node2_weight
	);

/*
* and an edge <node1, node2> into the graph,
*
* this method does not check the duplication of edges.
*
* NOTICE:
* you should ensure that the indexes of elements (edges and nodes) is started from 0 !!
*/
extern
void MY_GRAPHaddEdgeWithEdgeWeight(
	MY_GRAPH* graph,
	int node1,
	int node2,
	GRAPH_EDGE_WEIGHT edge_weight
	);

/*
* and an edge <node1, node2> into the graph,
*
* this method does not check the duplication of edges.
*
* NOTICE:
* you should ensure that the indexes of elements (edges and nodes) is started from 0 !!
*/
extern
void MY_GRAPHaddEdge(
	MY_GRAPH* graph,
	int node1,
	int node2
	);

/*
* call this method when finished adding all the edges
*/
extern
void MY_GRAPHflush(
	MY_GRAPH* graph
	);

/*
* get the first edge of @param{node}'s indegree
* this method only suitable for directed graph
* if this node is degree 0, then return a NULL pointer
*/
extern
LINK_FROM* MY_GRAPHgetFirstEdgeAdjFROM(
	MY_GRAPH* graph, 
	int node
	);

/*
* get the last edge of @param{node}'s indegree
* this method only suitable for directed graph
* if this node is degree 0, then return a NULL pointer
*/
extern
LINK_FROM* MY_GRAPHgetLastEdgeAdjFROM(
	MY_GRAPH* graph,
	int node
	);

/*
* get the first edge of @param{node}'s outdegree
* this method only suitable for directed graph
* if this node is degree 0, then return a NULL pointer
*/
extern
LINK_TO* MY_GRAPHgetFirstEdgeAdjTO(
	MY_GRAPH* graph,
	int node
	);

/*
* get the last edge of @param{node}'s outdegree
* this method only suitable for directed graph
* if this node is degree 0, then return a NULL pointer
*/
extern
LINK_TO* MY_GRAPHgetLastEdgeAdjTO(
	MY_GRAPH* graph,
	int node
	);

/*
* get the first edge that incident to @param{node}
* this method only suitable for undirected graph
* if this node is degree 0, then return a NULL pointer
*/
extern
LINK_TO* MY_GRAPHgetFirstEdgeAdj(
	MY_GRAPH* graph,
	int node
	);

/*
* get the last edge that incident to @param{node}
* this method only suitable for undirected graph
* if this node is degree 0, then return a NULL pointer
*/
extern
LINK_TO* MY_GRAPHgetLastEdgeAdj(
	MY_GRAPH* graph,
	int node
	);

/*
* this method only suitable for directed graph
*/
extern
int MY_GRAPHgetDegreeIN(
	MY_GRAPH* graph,
	int node
	);

/*
* this method only suitable for directed graph
*/
extern
int MY_GRAPHgetDegreeOUT(
	MY_GRAPH* graph,
	int node
	);

/*
* this method only suitable for undirected graph
*/
extern
int MY_GRAPHgetDegree(
	MY_GRAPH* graph,
	int node
	);

/*
* get the number of nodes
*/
extern
int MY_GRAPHgetNnodes(
	MY_GRAPH* graph
	);

/*
* get the number of edges
*/
extern
int MY_GRAPHgetNedges(
	MY_GRAPH* graph
	);

/*
* get edge
*/
extern
MY_EDGE MY_GRAPHgetEdge(
	MY_GRAPH* graph,
	int edge
	);

#ifdef __cplusplus
}
#endif

#endif