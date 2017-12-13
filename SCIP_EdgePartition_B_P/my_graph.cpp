/**@file   my_graph.cpp
 * @brief  data structure of my garph
 * @author william-jn-zhang
 *
 * 
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "blockmemshell\memory.h"
#include "my_graph.h"

#define DEFAULT_EDGE_WEIGHT 0
#define DEFAULT_NODE_WEIGHT 0

void MY_GRAPHcreateGraph(
	MY_GRAPH** graph, 
	int nnodes, 
	int nedges,
	bool is_directed_graph
	)
{
	BMSallocMemory(graph);
	MY_GRAPH* _graph = *graph;
	_graph -> nnodes = nnodes;
	_graph -> nedges = nedges;
	_graph -> is_directed_graph = is_directed_graph;

	_graph -> degrees_IN = NULL;
	_graph -> node_incident_FROM = NULL;
	_graph -> _edge_cnt = 0;

	_graph -> flushed = 0;

	BMSallocMemoryArray(&(_graph -> degrees_OUT), nnodes);
	memset(_graph -> degrees_OUT, 0, nnodes * sizeof(int));

	BMSallocMemoryArray(&(_graph -> edges), nedges);
	BMSallocMemoryArray(&(_graph -> node_incident_TO), nnodes);

	BMSallocMemoryArray(&(_graph -> nodeweights), nnodes);
	memset(_graph -> nodeweights, 0, nnodes * sizeof(GRAPH_NODE_WEIGHT));
	BMSallocMemoryArray(&(_graph -> edgeweights), nedges);
	memset(_graph -> edgeweights, 0, nnodes * sizeof(GRAPH_EDGE_WEIGHT));

	//handling the directed case
	if(is_directed_graph)
	{
		BMSallocMemoryArray(&(_graph -> node_incident_FROM), nnodes);
		BMSallocMemoryArray(&(_graph -> degrees_IN), nnodes);
		memset(_graph -> degrees_IN, 0, nnodes * sizeof(int));
	}
}


void MY_GRAPHaddEdgeWithEdgeNodeWeight(
	MY_GRAPH* graph,
	int node1,
	int node2,
	GRAPH_NODE_WEIGHT node1_weight,
	GRAPH_NODE_WEIGHT node2_weight,
	GRAPH_EDGE_WEIGHT edge_weight
	)
{
	MY_GRAPH* _graph = graph;
	assert(graph != NULL);
	assert((node1 >= 0) && (node1 < (_graph -> nnodes)));
	assert((node2 >= 0) && (node2 < (_graph -> nnodes)));
	int edge_cnt = _graph -> _edge_cnt;
	assert(edge_cnt < (_graph -> nedges));

	//store edge
	(_graph -> edges)[edge_cnt].node1 = node1;
	(_graph -> edges)[edge_cnt].node2 = node2;

	//store weight
	(_graph -> nodeweights)[node1] = node1_weight;
	(_graph -> nodeweights)[node2] = node2_weight;
	(_graph -> edgeweights)[edge_cnt] = edge_weight;

	//increase edge count
	++(_graph -> _edge_cnt);

	//increase the degree of related nodes
	//note that one should consider the directed and undirected case separately!
	if(_graph -> is_directed_graph)// the graph is directed graph
	{
		(_graph -> degrees_OUT)[node1] += 1;
		(_graph -> degrees_IN)[node2] += 1;
	}
	else
	{
		(_graph -> degrees_OUT)[node1] += 1;
		(_graph -> degrees_OUT)[node2] += 1;
	}
}


void MY_GRAPHaddEdgeWithNodeWeight(
	MY_GRAPH* graph,
	int node1,
	int node2,
	GRAPH_NODE_WEIGHT node1_weight,
	GRAPH_NODE_WEIGHT node2_weight
	)
{
	MY_GRAPHaddEdgeWithEdgeNodeWeight(graph, node1, node2, node1_weight, node2_weight, DEFAULT_EDGE_WEIGHT);
}

void MY_GRAPHaddEdgeWithEdgeWeight(
	MY_GRAPH* graph,
	int node1,
	int node2,
	GRAPH_EDGE_WEIGHT edge_weight
	)
{
	MY_GRAPHaddEdgeWithEdgeNodeWeight(graph, node1, node2, DEFAULT_NODE_WEIGHT, DEFAULT_NODE_WEIGHT, edge_weight);
}

void MY_GRAPHaddEdge(
	MY_GRAPH* graph,
	int node1,
	int node2
	)
{
	MY_GRAPHaddEdgeWithEdgeNodeWeight(graph, node1, node2, DEFAULT_NODE_WEIGHT, DEFAULT_NODE_WEIGHT, DEFAULT_EDGE_WEIGHT);
}

static
void flush_dir(
	MY_GRAPH* graph
	)
{
	int edge_i;
	int node_i;
	int node1;
	int node2;
	int node1_p;
	int node2_p;

	for(node_i = 0; node_i < graph -> nnodes; ++node_i)
	{
		BMSallocMemoryArray(&((graph -> node_incident_TO)[node_i]), (graph -> degrees_OUT)[node_i]);
		BMSallocMemoryArray(&((graph -> node_incident_FROM)[node_i]), (graph -> degrees_IN)[node_i]);
	}

	int* buf_to = NULL;
	int* buf_from = NULL;
	BMSallocMemoryArray(&buf_to, graph -> nnodes);
	BMSallocMemoryArray(&buf_from, graph -> nnodes);

	for(edge_i = 0; edge_i < graph -> nedges; ++edge_i)
	{
		node1 = (graph -> edges)[edge_i].node1;
		node2 = (graph -> edges)[edge_i].node2;

		node1_p = buf_to[node1];
		node2_p = buf_from[node2];

		(graph -> node_incident_TO)[node1][node1_p].edge = edge_i;
		(graph -> node_incident_TO)[node1][node1_p].link = node2;
		
		(graph -> node_incident_FROM)[node2][node2_p].edge = edge_i;
		(graph -> node_incident_FROM)[node2][node2_p].link = node1;

		++buf_to[node1];
		++buf_from[node2];
	}

	BMSfreeMemoryArray(&buf_to);
	BMSfreeMemoryArray(&buf_from);
}

static
void flush_undir(
	MY_GRAPH* graph
	)
{
	int edge_i;
	int node_i;
	int node1;
	int node2;
	int node1_p;
	int node2_p;

	for(node_i = 0; node_i < graph -> nnodes; ++node_i)
	{
		BMSallocMemoryArray(&((graph -> node_incident_TO)[node_i]), (graph -> degrees_OUT)[node_i]);
	}

	int* buf = NULL;
	BMSallocMemoryArray(&buf, graph -> nnodes);
	memset(buf, 0, (graph -> nnodes) * sizeof(int));

	for(edge_i = 0; edge_i < graph -> nedges; ++edge_i)
	{
		node1 = (graph -> edges)[edge_i].node1;
		node2 = (graph -> edges)[edge_i].node2;

		node1_p = buf[node1];
		node2_p = buf[node2];

		(graph -> node_incident_TO)[node1][node1_p].edge = edge_i;
		(graph -> node_incident_TO)[node1][node1_p].link = node2;

		(graph -> node_incident_TO)[node2][node2_p].edge = edge_i;
		(graph -> node_incident_TO)[node2][node2_p].link = node1;

		++buf[node1];
		++buf[node2];
	}
	BMSfreeMemoryArray(&buf);
}

void MY_GRAPHflush(
	MY_GRAPH* graph
	)
{
	assert(graph != NULL);
	assert((graph -> nedges) == (graph -> _edge_cnt));
	if(!(graph -> flushed))// this is the first flush of this graph
	{
		if(graph -> is_directed_graph)
		{
			flush_dir(graph);
		}
		else
		{
			flush_undir(graph);
		}
	}
	graph -> flushed = 1;
}

static
LINK_TO* _getFirstEdgeAdjTO(
	MY_GRAPH* graph,
	int node
	)
{
	if((graph -> degrees_OUT)[node] == 0)
		return NULL;
	
	return &((graph -> node_incident_TO)[node][0]);
}

static
LINK_TO* _getLastEdgeAdjTO(
	MY_GRAPH* graph,
	int node
	)
{
	int deg = (graph -> degrees_OUT)[node];
	if(deg == 0)
		return NULL;

	return &((graph -> node_incident_TO)[node][deg - 1]);
}

LINK_TO* MY_GRAPHgetFirstEdgeAdjTO(
	MY_GRAPH* graph,
	int node
	)
{
	assert(graph != NULL);
	assert(graph -> is_directed_graph);
	assert(node < (graph -> nnodes));

	return _getFirstEdgeAdjTO(graph, node);
}

LINK_TO* MY_GRAPHgetLastEdgeAdjTO(
	MY_GRAPH* graph,
	int node
	)
{
	assert(graph != NULL);
	assert(graph -> is_directed_graph);
	assert(node < (graph -> nnodes));

	return _getLastEdgeAdjTO(graph, node);
}

LINK_TO* MY_GRAPHgetFirstEdgeAdj(
	MY_GRAPH* graph,
	int node
	)
{
	assert(graph != NULL);
	assert(!(graph -> is_directed_graph));
	assert(node < (graph -> nnodes));

	return _getFirstEdgeAdjTO(graph, node);
}

LINK_TO* MY_GRAPHgetLastEdgeAdj(
	MY_GRAPH* graph,
	int node
	)
{
	assert(graph != NULL);
	assert(!(graph -> is_directed_graph));
	assert(node < (graph -> nnodes));

	return _getLastEdgeAdjTO(graph, node);
}


LINK_FROM* MY_GRAPHgetFirstEdgeAdjFROM(
	MY_GRAPH* graph, 
	int node
	)
{
	assert(graph != NULL);
	assert(graph -> is_directed_graph);
	assert(node < (graph -> nnodes));

	int deg = (graph -> degrees_IN)[node];
	if(deg == 0)
		return NULL;

	return &((graph -> node_incident_FROM)[node][0]);
}


LINK_FROM* MY_GRAPHgetLastEdgeAdjFROM(
	MY_GRAPH* graph,
	int node
	)
{
	assert(graph != NULL);
	assert(graph -> is_directed_graph);
	assert(node < (graph -> nnodes));

	int deg = (graph -> degrees_IN)[node];
	if(deg == 0)
		return NULL;

	return &((graph -> node_incident_FROM)[node][deg - 1]);
}


int MY_GRAPHgetDegreeOUT(
	MY_GRAPH* graph,
	int node
	)
{
	assert(graph != NULL);
	assert(graph -> is_directed_graph);
	assert(node < (graph -> nnodes));

	return (graph -> degrees_OUT)[node];
}

int MY_GRAPHgetDegree(
	MY_GRAPH* graph,
	int node
	)
{
	assert(graph != NULL);
	assert(!(graph -> is_directed_graph));
	assert(node < (graph -> nnodes));

	return (graph -> degrees_OUT)[node];
}

int MY_GRAPHgetDegreeIN(
	MY_GRAPH* graph,
	int node
	)
{
	assert(graph != NULL);
	assert(graph -> is_directed_graph);
	assert(node < (graph -> nnodes));

	return (graph -> degrees_IN)[node];
}

int MY_GRAPHgetNnodes(
	MY_GRAPH* graph
	)
{
	assert(graph != NULL);
	return graph -> nnodes;
}

int MY_GRAPHgetNedges(
	MY_GRAPH* graph
	)
{
	assert(graph != NULL);
	return graph -> nedges;
}

MY_EDGE MY_GRAPHgetEdge(
	MY_GRAPH* graph,
	int edge
	)
{
	assert(graph != NULL);
	assert(edge < (graph -> nedges));
	return (graph -> edges)[edge];
}