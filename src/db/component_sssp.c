#include <fcntl.h>
#include <math.h>
#include <limits.h>
#include "graph.h"
#include "cli.h"	


/* Place the code for your Dijkstra implementation in this file */

//Find the number of vertices in the graph
int getNumberOfVertices(){
	//Declare variables
	off_t off;
	ssize_t len;
	char s[BUFSIZE], *buf;
	int fd, length, numberOfVertices;
	length = sizeof(vertex_t);
	numberOfVertices = 0;
	buf = malloc(length);

	//Open vertex file/get file descriptor
	memset(s, 0, BUFSIZE);
	sprintf(s, "%s/%d/%d/v", grdbdir, gno, cno);
	fd = open(s, O_RDONLY);

	//Read in vertices from the file until no vertices remain 
	for (off = 0;;off += length){
		lseek(fd, off, SEEK_SET);
		len = read(fd, buf, length);
		if (len <= 0)
			break;
		numberOfVertices += 1;
	}

	//Close file
	close(fd);

	return numberOfVertices;
}


int
component_sssp(
        component_t c,
        vertexid_t v1,
        vertexid_t v2,
        int *n,
        int *total_weight,
        vertexid_t **path)
{

	//Get the weight of edge (u, v) out of the database
	int getEdgeWeight(vertexid_t u, vertexid_t v) {
		char s[BUFSIZE];
		int edgeFile, edgeWeight, offset;
		struct edge _edge;
		attribute_t attribute;

		edgeWeight = -1;

		//open edge file
		memset(s, 0, BUFSIZE);
		sprintf(s, "%s/%d/%d/e", grdbdir, gno, cno);
		edgeFile = open(s, O_RDONLY);

		//get edge
		edge_init(&_edge); //create empty edge
		edge_set_vertices(&_edge, u, v); //assign vertices to edge
		edge_read(&_edge, c->se, edgeFile); //read edge from db matching edge just created

		//read edge weight
		if (_edge.tuple != NULL){ //only continue if edge was found in database
			attribute = _edge.tuple->s->attrlist;
			offset = tuple_get_offset(_edge.tuple, attribute->name);
			if (offset >= 0){ 
				edgeWeight = tuple_get_int(_edge.tuple->buf + offset);
			}
		}

		close(edgeFile);

		return(edgeWeight);
	}

	void init_GC(int numberOfVertices, int G[numberOfVertices][numberOfVertices], int C[numberOfVertices][numberOfVertices]){
		int result;
		for (int i = 0; i < numberOfVertices; i += 1){
			for (int j = 0; j < numberOfVertices; j += 1){
				result = getEdgeWeight(i + 1, j + 1);
				if (result >= 0){
					G[i][j] = 1;
					C[i][j] = result;
				} else {
					G[i][j] = 0;
					C[i][j] = INT_MAX;
				}
			}
		}
	}

	void init_SD(int numberOfVertices, int S[numberOfVertices], int D[numberOfVertices], vertexid_t P[numberOfVertices], vertexid_t v1){
		for (int i = 0; i < numberOfVertices; i += 1){
			if (i == (v1)){
				S[i] = 1;
				D[i] = 0;
			} else {
				S[i] = 0;
				D[i] = INT_MAX;
			}
			P[i] = -1;
		}
	}

	void updatePath(vertexid_t **path, vertexid_t new, int *path_size){ 
	//Add vertex to path array
		vertexid_t *newPathArray = realloc(*path, (*path_size + 1) * sizeof(vertexid_t));
		if (newPathArray){
			newPathArray[*path_size] = new + 1;
			*path = newPathArray;
			*path_size += 1;
		}
	}

	void getPathFromVertex(int numberOfVertices, vertexid_t P[numberOfVertices]){
	//Go backwards from v2 to the source vertex to get the shortest path

		vertexid_t current, temp[numberOfVertices];
		int m = numberOfVertices - 1;
		current = v2;

		//Initialize temp
		for (int i = 0; i < numberOfVertices; i += 1){
			temp[i] = -1;
		}

		while (P[current] != -1){
			temp[m] = current;
			current = P[current];
			m -= 1;
		}

		for (int i = m + 1; i < numberOfVertices; i += 1){
			updatePath(path, temp[i], n);
		}
	}

	//Checks if all vertices in the graph have been added to S
	int is_full(int numberOfVertices, int S[numberOfVertices]){
		int isFull = 1;
		for (int i = 0; i < numberOfVertices; i += 1){
			if (S[i] == 0)
				isFull = 0;
		}

		return isFull;
	}

	//Check if a vertex v is in S
	int in_S(int numberOfVertices, int S[numberOfVertices], vertexid_t v){
		return S[v];
	}

	//Get the cheapest edge connected to S that's not already in S
	vertexid_t chooseMinimumPath(int numberOfVertices, 
							   int S[numberOfVertices], 
							   int G[numberOfVertices][numberOfVertices], 
							   int C[numberOfVertices][numberOfVertices])
	{
		int minimumWeight = INT_MAX;
		vertexid_t newVertex, parentVertex;

		for (vertexid_t i = 0; i < numberOfVertices; i += 1){
			if (S[i] == 1){   //If vertex i in S
				//Check costs of going to adjacent vertices not in S 
				for (vertexid_t j = 0; j < numberOfVertices; j += 1){
					if (!in_S(numberOfVertices, S, j) && C[i][j] < minimumWeight){
						minimumWeight = C[i][j];
						parentVertex = i;
						newVertex = j;
					}
				}
			}
		}

		S[newVertex] = 1;

		return newVertex;
	}

	void relaxGraphEdges(int numberOfVertices, vertexid_t w, 
					 int G[numberOfVertices][numberOfVertices], 
					 int C[numberOfVertices][numberOfVertices], 
					 int D[numberOfVertices], 
					 vertexid_t P[numberOfVertices])
	{
		for (int i = 0; i < numberOfVertices; i += 1){
			if (G[w][i] && D[i] > (D[w] + C[w][i])){
				D[i] = D[w] + C[w][i];
				P[i] = w;
			}
		}

	
	}

	/*Execute Dijkstra on the attribute you found for the specifiedcomponent*/
	v1 -= 1;
	v2 -= 1;
	int numberOfVertices = getNumberOfVertices();
	vertexid_t w, P[numberOfVertices];        //P = parent list
	int S[numberOfVertices], D[numberOfVertices]; //S = min path found list, D = Current distance list
	int G[numberOfVertices][numberOfVertices], C[numberOfVertices][numberOfVertices]; //G = adjacency matrix, C = cost matrix
	vertexid_t *new_path = malloc(sizeof(vertexid_t));


	//Initialize path
	if (new_path){
		*path = new_path;
		*path[0] = v1 + 1;
	}
	*n = 1;

	//Initialize matrices and lists
	init_SD(numberOfVertices, S, D, P, v1);
	init_GC(numberOfVertices, G, C);

	//Relax edges connected to v1
	relaxGraphEdges(numberOfVertices, v1, G, C, D, P);

	//Add cheapest edges and relax until all shortest paths found
	while(!is_full(numberOfVertices, S)){
		w = chooseMinimumPath(numberOfVertices, S, G, C);
		relaxGraphEdges(numberOfVertices, w, G, C, D, P);
	}

	//Print out weight of minimum paths to each vertex
	for (int i = 0; i < numberOfVertices; i += 1){
		printf("Min distance to vertex %i is %i\n", i + 1, D[i]);
	}

	getPathFromVertex(numberOfVertices, P);
	*total_weight = D[v2];	

	return 0;
}