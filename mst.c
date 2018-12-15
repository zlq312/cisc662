#include <mpi.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char EMPTY_FIELD = ' ';
const char HORIZONTAL_EDGE = '-';
const char VERTICAL_EDGE = '|';
const char VERTEX = '+';
const int EDGE_MEMBERS = 3;
const int MAXIMUM_RANDOM = 100;
const int UNSET_ELEMENT = -1;

typedef struct Handle {
	bool create;
	bool help;
	bool maze;
	bool verbose;
	int algorithm;
	int columns;
	int rows;
	char* graphFile;
} Handle;

typedef struct ListElement {
	int vertex;
	int weight;
} ListElement;

typedef struct List {
	int alloced;
	int size;
	ListElement* elements;
} List;

typedef struct AdjacencyList {
	int elements;
	List* lists;
} AdjacencyList;

typedef struct Set {
	int elements;
	int* canonicalElements;
	int* rank;
} Set;

typedef struct BinaryHeapElement {
	int vertex;
	int via;
	int weight;
} BinaryHeapElement;

typedef struct BinaryMinHeap {
	int alloced;
	int size;
	int* positions;
	BinaryHeapElement* elements;
} BinaryMinHeap;

typedef struct FibonacciHeapElement {
	bool marked;
	int childrens;
	int vertex;
	int via;
	int weight;
	struct FibonacciHeapElement* parent;
	struct FibonacciHeapElement* child;
	struct FibonacciHeapElement* left;
	struct FibonacciHeapElement* right;
} FibonacciHeapElement;

typedef struct FibonacciMinHeap {
	int size;
	FibonacciHeapElement* minimum;
	FibonacciHeapElement** positions;
} FibonacciMinHeap;

typedef struct WeightedGraph {
	int edges;
	int vertices;
	int* edgeList;
} WeightedGraph;

void consolidateFibonacciMinHeap(FibonacciMinHeap* heap);
void copyEdge(int* to, int* from);
void createMazeFile(const int rows, const int columns,
		const char outputFileName[]);
void cutFibonacciMinHeap(FibonacciMinHeap* heap, FibonacciHeapElement* element);
void decreaseBinaryMinHeap(BinaryMinHeap* heap, const int vertex, const int via,
		const int weight);
void decreaseFibonacciMinHeap(FibonacciMinHeap* heap, const int vertex,
		const int via, const int weight);
void deleteAdjacencyList(AdjacencyList* list);
void deleteBinaryMinHeap(BinaryMinHeap* heap);
void deleteFibonacciMinHeap(FibonacciMinHeap* heap);
void deleteSet(Set* set);
void deleteWeightedGraph(WeightedGraph* graph);
int findSet(const Set* set, const int vertex);
void heapifyBinaryMinHeap(BinaryMinHeap* heap, int position);
void heapifyDownBinaryMinHeap(BinaryMinHeap* heap, int position);
void insertFibonacciMinHeap(FibonacciMinHeap* heap,
		FibonacciHeapElement* element);
void merge(int* edgeList, const int start, const int end, const int pivot);
void mergeSort(int* edgeList, const int start, const int end);
void mstBoruvka(const WeightedGraph* graph, WeightedGraph* mst);
void mstKruskal(WeightedGraph* graph, WeightedGraph* mst);
void mstPrimBinary(const WeightedGraph* graph, WeightedGraph* mst);
void mstPrimFibonacci(const WeightedGraph* graph, WeightedGraph* mst);
void newAdjacencyList(AdjacencyList* list, const WeightedGraph* graph);
void newBinaryMinHeap(BinaryMinHeap* heap);
void newFibonacciHeapElement(FibonacciHeapElement* element, const int vertex,
		const int via, const int weight, FibonacciHeapElement* left,
		FibonacciHeapElement* right, FibonacciHeapElement* parent,
		FibonacciHeapElement* child);
void newSet(Set* set, const int elements);
void newFibonacciMinHeap(FibonacciMinHeap* heap);
void newWeightedGraph(WeightedGraph* graph, const int vertices, const int edges);
void popBinaryMinHeap(BinaryMinHeap* heap, int* vertex, int* via, int* weight);
void popFibonacciMinHeap(FibonacciMinHeap* heap, int* vertex, int* via,
		int* weight);
void printAdjacencyList(const AdjacencyList* list);
void printBinaryHeap(const BinaryMinHeap* heap);
void printFibonacciHeap(const FibonacciMinHeap* heap,
		FibonacciHeapElement* startElement);
void printMaze(const WeightedGraph* graph, const int rows, const int columns);
void printSet(const Set* set);
void printWeightedGraph(const WeightedGraph* graph);
Handle processParameters(int argc, char* argv[]);
void pushAdjacencyList(AdjacencyList* list, const int from, const int to,
		const int weight);
void pushBinaryMinHeap(BinaryMinHeap* heap, const int vertex, const int via,
		const int weight);
void pushFibonacciMinHeap(FibonacciMinHeap* heap, const int vertex,
		const int via, const int weight);
void readGraphFile(WeightedGraph* graph, const char inputFileName[]);
void scatterEdgeList(int* edgeList, int* edgeListPart, const int elements,
		int* elementsPart);
void sort(WeightedGraph* graph);
void swapBinaryHeapElement(BinaryMinHeap* heap, int position1, int position2);
void unionSet(Set* set, const int parent1, const int parent2);

/*
 * main program
 */
int main(int argc, char* argv[]) {
	// MPI variables and initialization
	int rank;
	int size;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Datatype MPI_HANDLE;
	int blockCounts[3] = { 4, 3, 1 };
	MPI_Aint width[3];
	MPI_Type_extent(MPI_C_BOOL, &width[0]);
	MPI_Type_extent(MPI_INT, &width[1]);
	MPI_Type_extent(MPI_UNSIGNED, &width[2]);
	MPI_Aint offsets[3] = { 0, 4 * width[0], 3 * width[1] };
	MPI_Datatype oldTypes[3] = { MPI_C_BOOL, MPI_INT, MPI_UNSIGNED };
	MPI_Type_struct(3, blockCounts, offsets, oldTypes, &MPI_HANDLE);
	MPI_Type_commit(&MPI_HANDLE);

	// control variable
	Handle handle;

	if (rank == 0) {
		// process command line parameters
		handle = processParameters(argc, argv);
	}

	MPI_Bcast(&handle, 1, MPI_HANDLE, 0, MPI_COMM_WORLD);
	if (handle.help) {
		MPI_Finalize();
		exit(EXIT_SUCCESS);
	}

	// graph Variables
	WeightedGraph* graph = &(WeightedGraph ) { .edges = 0, .vertices = 0,
					.edgeList = NULL };
	WeightedGraph* mst = &(WeightedGraph ) { .edges = 0, .vertices = 0,
					.edgeList = NULL };

	if (rank == 0) {
		printf("Starting\n");

		if (handle.create) {
			// create a new maze file
			createMazeFile(handle.rows, handle.columns, handle.graphFile);
		}

		// read the maze file and store it in the graph
		readGraphFile(graph, handle.graphFile);

		if (handle.verbose) {
			// print the edges of the read graph
			printf("Graph:\n");
			printWeightedGraph(graph);
		}

		newWeightedGraph(mst, graph->vertices, graph->vertices - 1);
	}

	double start = MPI_Wtime();
	switch (handle.algorithm) {
	case 0:
		// use Kruskal's algorithm
		mstKruskal(graph, mst);
		break;
	case 1:
		// use Prim's algorithm (fibonacci)
		mstPrimFibonacci(graph, mst);
		break;
	case 2:
		// use Prim's algorithm (binary)
		mstPrimBinary(graph, mst);
		break;
	case 3:
		// use Boruvka's algorithm
		mstBoruvka(graph, mst);
		break;
	default:
		if (rank == 0) {
			fprintf(stderr, "Unknown algorithm: %d\n"
					"-h for help\n", handle.algorithm);
		}
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}

	if (rank == 0) {
		printf("Time elapsed: %f s\n", MPI_Wtime() - start);

		if (handle.verbose) {
			// print the edges of the MST
			printf("MST:\n");
			printWeightedGraph(mst);
		}

		unsigned long weightMST = 0;
		for (int i = 0; i < mst->edges; i++) {
			weightMST += mst->edgeList[i * EDGE_MEMBERS + 2];
		}
		printf("MST weight: %lu\n", weightMST);

		if (handle.maze) {
			// print the maze to the console
			printf("Maze:\n");
			printMaze(mst, handle.rows, handle.columns);
		}

		// cleanup
		deleteWeightedGraph(graph);
		deleteWeightedGraph(mst);

		printf("Finished\n");
	}

	MPI_Finalize();

	return EXIT_SUCCESS;
}

/*
 * rearrange fibonacci heap and update minimum
 */
void consolidateFibonacciMinHeap(FibonacciMinHeap* heap) {
	// initialize degree array
	int degreeSize = 2 * log2(heap->size) + 1;
	FibonacciHeapElement** degree = (FibonacciHeapElement**) malloc(
			degreeSize * sizeof(FibonacciHeapElement*));
	for (int i = 0; i < degreeSize; i++) {
		degree[i] = NULL;
	}

	// add roots to degree array
	FibonacciHeapElement* element = heap->minimum;
	FibonacciHeapElement* nextElement = NULL;
	do {
		if (element == element->right) {
			nextElement = NULL;
		} else {
			nextElement = element->right;
		}
		element->right->left = element->left;
		element->left->right = element->right;
		element->right = element;
		element->left = element;
		int currentDegree = element->childrens;
		while (degree[currentDegree] != NULL) {
			if (element->weight > degree[currentDegree]->weight) {
				FibonacciHeapElement* tmp = element;
				element = degree[currentDegree];
				degree[currentDegree] = tmp;
			}

			// degree[currentDegree] becomes child of element
			if (element->child == NULL) {
				element->child = degree[currentDegree];
				degree[currentDegree]->parent = element;
			} else {
				degree[currentDegree]->parent = element;
				degree[currentDegree]->right = element->child;
				degree[currentDegree]->left = element->child->left;
				degree[currentDegree]->right->left = degree[currentDegree];
				degree[currentDegree]->left->right = degree[currentDegree];
			}
			element->childrens++;
			degree[currentDegree]->marked = false;
			degree[currentDegree] = NULL;
			currentDegree++;
		}
		degree[currentDegree] = element;

		element = nextElement;
	} while (element != NULL);

	// update minimum
	heap->minimum = NULL;
	for (int i = 0; i < degreeSize; i++) {
		if (degree[i] != NULL) {
			if (heap->minimum == NULL) {
				// heap empty
				heap->minimum = degree[i];
				degree[i]->right = degree[i];
				degree[i]->left = degree[i];
			} else {
				degree[i]->right = heap->minimum;
				degree[i]->left = heap->minimum->left;
				heap->minimum->left->right = degree[i];
				heap->minimum->left = degree[i];
				if (degree[i]->weight < heap->minimum->weight) {
					// less weight then current minimum
					heap->minimum = degree[i];
				}
			}
		}
	}

	free(degree);
}

/*
 * copy an edge
 */
void copyEdge(int* to, int* from) {
	memcpy(to, from, EDGE_MEMBERS * sizeof(int));
}

/*
 * save a 2D (rows x columns) grid graph with random edge weights to a file
 */
void createMazeFile(const int rows, const int columns,
		const char outputFileName[]) {
	// open the file
	FILE* outputFile;
	const char* outputMode = "w";
	outputFile = fopen(outputFileName, outputMode);
	if (outputFile == NULL) {
		fprintf(stderr, "Couldn't open output file, exiting!\n");
		exit(EXIT_FAILURE);
	}

	// first line contains number of vertices and edges
	const int vertices = rows * columns;
	const int edges = vertices * 2 - rows - columns;
	fprintf(outputFile, "%d %d\n", vertices, edges);

	// all lines after the first contain the edges, values stored as "from to weight"
	srand(time(NULL));
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < columns; j++) {
			int vertex = i * columns + j;
			int fprintfResult;
			if (j != columns - 1) {
				fprintfResult = fprintf(outputFile, "%d %d %d\n", vertex,
						vertex + 1, rand() % MAXIMUM_RANDOM);
			}
			if (i != rows - 1) {
				fprintfResult = fprintf(outputFile, "%d %d %d\n", vertex,
						vertex + columns, rand() % MAXIMUM_RANDOM);
			}

			// EOF result of *scanf indicates an error
			if (fprintfResult < 0) {
				fprintf(stderr,
						"Something went wrong during writing of maze file, exiting!\n");
				fclose(outputFile);
				exit(EXIT_FAILURE);
			}
		}
	}

	fclose(outputFile);
}

/*
 * cut an element from a fibonacci heap
 */
void cutFibonacciMinHeap(FibonacciMinHeap* heap, FibonacciHeapElement* element) {
	FibonacciHeapElement* parent = element->parent;
	if (parent != NULL) {
		parent->childrens--;
	}
	if (element->right == element) {
		// only one element in the child list
		parent->child = NULL;
	} else {
		element->right->left = element->left;
		element->left->right = element->right;
		if (parent->child == element) {
			// update parents child pointer
			parent->child = element->right;
		}
	}

	// insert as new root element
	insertFibonacciMinHeap(heap, element);
	element->parent = NULL;

	if (parent->parent != NULL) {
		// not a root element
		if (parent->marked) {
			// recursively cut marked parent
			cutFibonacciMinHeap(heap, parent);
			parent->marked = false;
		} else {
			parent->marked = true;
		}
	}
}

/*
 * only decrease the weight to a given vertex
 */
void decreaseBinaryMinHeap(BinaryMinHeap* heap, const int vertex, const int via,
		const int weight) {
	if (heap->positions[vertex] != UNSET_ELEMENT
			&& heap->elements[heap->positions[vertex]].weight > weight) {
		heap->elements[heap->positions[vertex]].via = via;
		heap->elements[heap->positions[vertex]].weight = weight;
		heapifyBinaryMinHeap(heap, heap->positions[vertex]);
	}
}

/*
 * only decrease the weight to a given vertex
 */
void decreaseFibonacciMinHeap(FibonacciMinHeap* heap, const int vertex,
		const int via, const int weight) {
	FibonacciHeapElement* element = heap->positions[vertex];
	if (element != NULL && element->weight > weight) {
		element->via = via;
		element->weight = weight;
		if (element->parent == NULL) {
			if (element->weight < heap->minimum->weight) {
				heap->minimum = element;
			}
		} else if (weight < element->parent->weight) {
			// if heap property is violated cut off the element
			cutFibonacciMinHeap(heap, element);
		}
	}
}

/*
 * cleanup adjacency list data
 */
void deleteAdjacencyList(AdjacencyList* list) {
	for (int i = 0; i < list->elements; i++) {
		free(list->lists[i].elements);
	}
	free(list->lists);
}

/*
 * cleanup binary heap data
 */
void deleteBinaryMinHeap(BinaryMinHeap* heap) {
	free(heap->elements);
	free(heap->positions);
}

/*
 * cleanup fibonacci heap data
 */
void deleteFibonacciMinHeap(FibonacciMinHeap* heap) {
	free(heap->positions);
}

/*
 * cleanup set data
 */
void deleteSet(Set* set) {
	free(set->canonicalElements);
	free(set->rank);
}

/*
 * cleanup graph data
 */
void deleteWeightedGraph(WeightedGraph* graph) {
	free(graph->edgeList);
}

/*
 * return the canonical element of a vertex with path compression
 */
int findSet(const Set* set, const int vertex) {
	if (set->canonicalElements[vertex] == UNSET_ELEMENT) {
		return vertex;
	} else {
		set->canonicalElements[vertex] = findSet(set,
				set->canonicalElements[vertex]);
		return set->canonicalElements[vertex];
	}
}

/*
 * check and restore heap property from given position upwards
 */
void heapifyBinaryMinHeap(BinaryMinHeap* heap, int position) {
	while (position >= 0) {
		int positionParent = (position - 1) / 2;
		if (heap->elements[position].weight
				< heap->elements[positionParent].weight) {
			swapBinaryHeapElement(heap, position, positionParent);
			position = positionParent;
		} else {
			break;
		}
	}
}

/*
 * check and restore heap property from given position upwards
 */
void heapifyDownBinaryMinHeap(BinaryMinHeap* heap, int position) {
	while (position < heap->size) {
		int positionLeft = (position + 1) * 2 - 1;
		int positionRight = (position + 1) * 2;
		int positionSmallest = position;
		if (positionLeft <= heap->size
				&& heap->elements[positionLeft].weight
						< heap->elements[positionSmallest].weight) {
			positionSmallest = positionLeft;
		}
		if (positionRight <= heap->size
				&& heap->elements[positionRight].weight
						< heap->elements[positionSmallest].weight) {
			positionSmallest = positionRight;
		}
		if (heap->elements[position].weight
				> heap->elements[positionSmallest].weight) {
			swapBinaryHeapElement(heap, position, positionSmallest);
			position = positionSmallest;
		} else {
			break;
		}
	}
}

/*
 * merge element into fibonacci heap left to the minimum
 */
void insertFibonacciMinHeap(FibonacciMinHeap* heap,
		FibonacciHeapElement* element) {
	if (heap->minimum == NULL) {
		heap->minimum = element;
	} else {
		FibonacciHeapElement* endHeap = heap->minimum->left;
		heap->minimum->left = element;
		element->left = endHeap;
		endHeap->right = element;
		element->right = heap->minimum;

		// set new minimum
		if (heap->minimum->weight > element->weight) {
			heap->minimum = element;
		}
	}
}

/*
 * merge sorted lists, start and end are inclusive
 */
void merge(int* edgeList, const int start, const int end, const int pivot) {
	int length = end - start + 1;
	int* working = (int*) malloc(length * EDGE_MEMBERS * sizeof(int));

	// copy first part
	memcpy(working, &edgeList[start * EDGE_MEMBERS],
			(pivot - start + 1) * EDGE_MEMBERS * sizeof(int));

	// copy second part reverse to simpify merge
	int workingEnd = end + pivot - start + 1;
	for (int i = pivot + 1; i <= end; i++) {
		copyEdge(&working[(workingEnd - i) * EDGE_MEMBERS],
				&edgeList[i * EDGE_MEMBERS]);
	}

	int left = 0;
	int right = end - start;
	for (int k = start; k <= end; k++) {
		if (working[right * EDGE_MEMBERS + 2]
				< working[left * EDGE_MEMBERS + 2]) {
			copyEdge(&edgeList[k * EDGE_MEMBERS],
					&working[right * EDGE_MEMBERS]);
			right--;
		} else {
			copyEdge(&edgeList[k * EDGE_MEMBERS],
					&working[left * EDGE_MEMBERS]);
			left++;
		}
	}

	// clean up
	free(working);

// old merge
//	int length = end - start + 1;
//	// make a temporary copy of the list for merging
//	int* working = (int*) malloc(length * EDGE_MEMBERS * sizeof(int));
//	memcpy(working, &edgeList[start * EDGE_MEMBERS],
//			length * EDGE_MEMBERS * sizeof(int));
//
//	// merge the two parts together
//	int merge1 = 0;
//	int merge2 = pivot - start + 1;
//	for (int i = 0; i < length; i++) {
//		if (merge2 <= end - start) {
//			if (merge1 <= pivot - start) {
//				if (working[merge1 * EDGE_MEMBERS + 2]
//						> working[merge2 * EDGE_MEMBERS + 2]) {
//					copyEdge(&edgeList[(i + start) * EDGE_MEMBERS],
//							&working[merge2 * EDGE_MEMBERS]);
//					merge2++;
//				} else {
//					copyEdge(&edgeList[(i + start) * EDGE_MEMBERS],
//							&working[merge1 * EDGE_MEMBERS]);
//					merge1++;
//				}
//			} else {
//				copyEdge(&edgeList[(i + start) * EDGE_MEMBERS],
//						&working[merge2 * EDGE_MEMBERS]);
//				merge2++;
//			}
//		} else {
//			copyEdge(&edgeList[(i + start) * EDGE_MEMBERS],
//					&working[merge1 * EDGE_MEMBERS]);
//			merge1++;
//		}
//	}
//
//	// clean up
//	free(working);
}

/*
 * sort the edge list using merge sort, start and end are inclusive
 */
void mergeSort(int* edgeList, const int start, const int end) {
	if (start != end) {
		// recursively divide the list in two parts and sort them
		int pivot = (start + end) / 2;
		mergeSort(edgeList, start, pivot);
		mergeSort(edgeList, pivot + 1, end);

		merge(edgeList, start, end, pivot);
	}
}

/*
 * find a MST of the graph using Boruvka's algorithm
 */
void mstBoruvka(const WeightedGraph* graph, WeightedGraph* mst) {
	int rank;
	int size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Status status;

	bool parallel = size != 1;

	// send number of edges and vertices
	int edges;
	int vertices;
	if (rank == 0) {
		edges = graph->edges;
		vertices = graph->vertices;
		MPI_Bcast(&edges, 1, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(&vertices, 1, MPI_INT, 0, MPI_COMM_WORLD);
	} else {
		MPI_Bcast(&edges, 1, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(&vertices, 1, MPI_INT, 0, MPI_COMM_WORLD);
	}

	// scatter the edges to search in them
	int edgesPart = (edges + size - 1) / size;
	int* edgeListPart = (int*) malloc(edgesPart * EDGE_MEMBERS * sizeof(int));
	if (parallel) {
		scatterEdgeList(graph->edgeList, edgeListPart, edges, &edgesPart);
	} else {
		edgeListPart = graph->edgeList;
	}

	// create needed data structures
	Set* set = &(Set ) { .elements = 0, .canonicalElements = NULL, .rank =
			NULL };
	newSet(set, vertices);

	int edgesMST = 0;
	int* closestEdge = (int*) malloc(vertices * EDGE_MEMBERS * sizeof(int));
	int* closestEdgeRecieved;
	if (parallel) {
		closestEdgeRecieved = (int*) malloc(
				vertices * EDGE_MEMBERS * sizeof(int));
	}

	for (int i = 1; i < vertices && edgesMST < vertices - 1; i *= 2) {
		// reset all closestEdge
		for (int j = 0; j < vertices; j++) {
			closestEdge[j * EDGE_MEMBERS + 2] = INT_MAX;
		}

		// find closestEdge
		for (int j = 0; j < edgesPart; j++) {
			int* currentEdge = &edgeListPart[j * EDGE_MEMBERS];
			int canonicalElements[2] = { findSet(set, currentEdge[0]), findSet(
					set, currentEdge[1]) };

			// eventually update closestEdge
			if (canonicalElements[0] != canonicalElements[1]) {
				for (int k = 0; k < 2; k++) {
					bool closestEdgeNotSet = closestEdge[canonicalElements[k]
							* EDGE_MEMBERS + 2] == INT_MAX;
					bool weightSmaller = currentEdge[2]
							< closestEdge[canonicalElements[k] * EDGE_MEMBERS
									+ 2];
					if (closestEdgeNotSet || weightSmaller) {
						copyEdge(
								&closestEdge[canonicalElements[k] * EDGE_MEMBERS],
								currentEdge);
					}
				}
			}
		}

		if (parallel) {
			int from;
			int to;
			for (int step = 1; step < size; step *= 2) {
				if (rank % (2 * step) == 0) {
					from = rank + step;
					if (from < size) {
						MPI_Recv(closestEdgeRecieved, vertices * EDGE_MEMBERS,
						MPI_INT, from, 0, MPI_COMM_WORLD, &status);

						// combine all closestEdge parts
						for (int i = 0; i < vertices; i++) {
							int currentVertex = i * EDGE_MEMBERS;
							if (closestEdgeRecieved[currentVertex + 2]
									< closestEdge[currentVertex + 2]) {
								copyEdge(&closestEdge[currentVertex],
										&closestEdgeRecieved[currentVertex]);
							}
						}
					}
				} else if (rank % step == 0) {
					to = rank - step;
					MPI_Send(closestEdge, vertices * EDGE_MEMBERS, MPI_INT, to,
							0,
							MPI_COMM_WORLD);
				}
			}
			// publish all closestEdge parts
			MPI_Bcast(closestEdge, vertices * EDGE_MEMBERS, MPI_INT, 0,
			MPI_COMM_WORLD);
		}

		// add new edges to MST
		for (int j = 0; j < vertices; j++) {
			if (closestEdge[j * EDGE_MEMBERS + 2] != INT_MAX) {
				int from = closestEdge[j * EDGE_MEMBERS];
				int to = closestEdge[j * EDGE_MEMBERS + 1];

				// prevent adding the same edge twice
				if (findSet(set, from) != findSet(set, to)) {
					if (rank == 0) {
						copyEdge(&mst->edgeList[edgesMST * EDGE_MEMBERS],
								&closestEdge[j * EDGE_MEMBERS]);
					}
					edgesMST++;
					unionSet(set, from, to);
				}
			}
		}
	}

	// clean up
	deleteSet(set);
	free(closestEdge);
	if (parallel) {
		free(closestEdgeRecieved);
		free(edgeListPart);
	}
}

/*
 * find a MST of the graph using Kruskal's algorithm
 */
void mstKruskal(WeightedGraph* graph, WeightedGraph* mst) {
	// create needed data structures
	Set* set = &(Set ) { .elements = 0, .canonicalElements = NULL, .rank =
			NULL };
	newSet(set, graph->vertices);

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// sort the edges of the graph
	sort(graph);

	if (rank == 0) {
		// add edges to the MST
		int currentEdge = 0;
		for (int edgesMST = 0;
				edgesMST < graph->vertices - 1 || currentEdge < graph->edges;) {
			// check for loops if edge would be inserted
			int canonicalElementFrom = findSet(set,
					graph->edgeList[currentEdge * EDGE_MEMBERS]);
			int canonicalElementTo = findSet(set,
					graph->edgeList[currentEdge * EDGE_MEMBERS + 1]);
			if (canonicalElementFrom != canonicalElementTo) {
				// add edge to MST
				copyEdge(&mst->edgeList[edgesMST * EDGE_MEMBERS],
						&graph->edgeList[currentEdge * EDGE_MEMBERS]);
				unionSet(set, canonicalElementFrom, canonicalElementTo);
				edgesMST++;
			}
			currentEdge++;
		}
	}

	// clean up
	deleteSet(set);
}

/*
 * find a MST of the graph using Prim's algorithm with a binary heap
 */
void mstPrimBinary(const WeightedGraph* graph, WeightedGraph* mst) {
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank == 0) {
		// create needed data structures
		AdjacencyList* list = &(AdjacencyList ) { .elements = 0, .lists = NULL };
		newAdjacencyList(list, graph);
		for (int i = 0; i < graph->edges; i++) {
			pushAdjacencyList(list, graph->edgeList[i * EDGE_MEMBERS],
					graph->edgeList[i * EDGE_MEMBERS + 1],
					graph->edgeList[i * EDGE_MEMBERS + 2]);
		}

		BinaryMinHeap* heap = &(BinaryMinHeap ) { .alloced = 0, .size = 0,
						.positions = NULL, .elements = NULL };
		newBinaryMinHeap(heap);
		heap->positions = (int*) realloc(heap->positions,
				graph->vertices * sizeof(int));
		for (int i = 0; i < graph->vertices; i++) {
			pushBinaryMinHeap(heap, i, INT_MAX, INT_MAX);
		}

		int vertex;
		int via;
		int weight;

		// start at first vertex
		decreaseBinaryMinHeap(heap, 0, 0, 0);
		popBinaryMinHeap(heap, &vertex, &via, &weight);
		for (int i = 0; i < list->lists[vertex].size; i++) {
			decreaseBinaryMinHeap(heap, list->lists[vertex].elements[i].vertex,
					vertex, list->lists[vertex].elements[i].weight);
		}

		for (int i = 0; heap->size > 0; i++) {
			// add edge from heap to MST
			popBinaryMinHeap(heap, &vertex, &via, &weight);
			mst->edgeList[i * EDGE_MEMBERS] = vertex;
			mst->edgeList[i * EDGE_MEMBERS + 1] = via;
			mst->edgeList[i * EDGE_MEMBERS + 2] = weight;

			// update heap
			for (int j = 0; j < list->lists[vertex].size; j++) {
				decreaseBinaryMinHeap(heap,
						list->lists[vertex].elements[j].vertex, vertex,
						list->lists[vertex].elements[j].weight);
			}
		}

		// clean up
		deleteAdjacencyList(list);
		deleteBinaryMinHeap(heap);
	}
}

/*
 * find a MST of the graph using Prim's algorithm with a fibonacci heap
 */
void mstPrimFibonacci(const WeightedGraph* graph, WeightedGraph* mst) {
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank == 0) {
		// create needed data structures
		AdjacencyList* list = &(AdjacencyList ) { .elements = 0, .lists = NULL };
		newAdjacencyList(list, graph);
		for (int i = 0; i < graph->edges; i++) {
			pushAdjacencyList(list, graph->edgeList[i * EDGE_MEMBERS],
					graph->edgeList[i * EDGE_MEMBERS + 1],
					graph->edgeList[i * EDGE_MEMBERS + 2]);
		}

		FibonacciMinHeap* heap = &(FibonacciMinHeap ) { .size = 0, .minimum =
				NULL, .positions = NULL };
		newFibonacciMinHeap(heap);
		heap->positions = (FibonacciHeapElement**) realloc(heap->positions,
				graph->vertices * sizeof(FibonacciHeapElement*));
		for (int i = 0; i < graph->vertices; i++) {
			pushFibonacciMinHeap(heap, i, INT_MAX, INT_MAX);
		}

		int vertex;
		int via;
		int weight;

		// start at first vertex
		decreaseFibonacciMinHeap(heap, 0, 0, 0);

		popFibonacciMinHeap(heap, &vertex, &via, &weight);
		for (int i = 0; i < list->lists[vertex].size; i++) {
			decreaseFibonacciMinHeap(heap,
					list->lists[vertex].elements[i].vertex, vertex,
					list->lists[vertex].elements[i].weight);
		}

		for (int i = 0; heap->size > 0; i++) {
			// add edge from heap to MST
			popFibonacciMinHeap(heap, &vertex, &via, &weight);
			mst->edgeList[i * EDGE_MEMBERS] = vertex;
			mst->edgeList[i * EDGE_MEMBERS + 1] = via;
			mst->edgeList[i * EDGE_MEMBERS + 2] = weight;

			// update heap
			for (int j = 0; j < list->lists[vertex].size; j++) {
				decreaseFibonacciMinHeap(heap,
						list->lists[vertex].elements[j].vertex, vertex,
						list->lists[vertex].elements[j].weight);
			}
		}

		// clean up
		deleteAdjacencyList(list);
		deleteFibonacciMinHeap(heap);
	}
}

/*
 * create adjacency list
 */
void newAdjacencyList(AdjacencyList* list, const WeightedGraph* graph) {
	int startSize = 4;
	list->elements = graph->vertices;
	list->lists = (List*) malloc(list->elements * sizeof(List));
	for (int i = 0; i < list->elements; i++) {
		list->lists[i].alloced = startSize;
		list->lists[i].size = 0;
		list->lists[i].elements = (ListElement*) malloc(
				startSize * sizeof(ListElement));
	}
}

/*
 * create binary min heap
 */
void newBinaryMinHeap(BinaryMinHeap* heap) {
	int startSize = 4;
	heap->alloced = startSize;
	heap->size = 0;
	heap->positions = (int*) malloc(sizeof(int));
	heap->elements = (BinaryHeapElement*) malloc(
			startSize * sizeof(BinaryHeapElement));
}

/*
 * create fibonacci min heap element
 */
void newFibonacciHeapElement(FibonacciHeapElement* element, const int vertex,
		const int via, const int weight, FibonacciHeapElement* left,
		FibonacciHeapElement* right, FibonacciHeapElement* parent,
		FibonacciHeapElement* child) {
	element->childrens = 0;
	element->marked = false;
	element->vertex = vertex;
	element->via = via;
	element->weight = weight;
	element->parent = parent;
	element->child = child;
	element->left = left;
	element->right = right;
}

/*
 * create fibonacci min heap
 */
void newFibonacciMinHeap(FibonacciMinHeap* heap) {
	heap->size = 0;
	heap->positions = (FibonacciHeapElement**) malloc(
			sizeof(FibonacciHeapElement*));
}

/*
 * initialize and allocate memory for the members of the graph
 */
void newSet(Set* set, const int elements) {
	set->elements = elements;
	set->canonicalElements = (int*) malloc(elements * sizeof(int));
	memset(set->canonicalElements, UNSET_ELEMENT, elements * sizeof(int));
	set->rank = (int*) calloc(elements, sizeof(int));
}

/*
 * initialize and allocate memory for the members of the graph
 */
void newWeightedGraph(WeightedGraph* graph, const int vertices, const int edges) {
	graph->edges = edges;
	graph->vertices = vertices;
	graph->edgeList = (int*) calloc(edges * EDGE_MEMBERS, sizeof(int));
}

/*
 * remove the minimum of the heap
 */
void popBinaryMinHeap(BinaryMinHeap* heap, int* vertex, int* via, int* weight) {
	*vertex = heap->elements[0].vertex;
	*via = heap->elements[0].via;
	*weight = heap->elements[0].weight;
	heap->positions[heap->elements[0].vertex] = UNSET_ELEMENT;
	heap->elements[0] = heap->elements[heap->size - 1];
	heap->positions[heap->elements[0].vertex] = 0;
	heap->size--;
	heapifyDownBinaryMinHeap(heap, 0);
}

/*
 * remove the minimum of the heap
 */
void popFibonacciMinHeap(FibonacciMinHeap* heap, int* vertex, int* via,
		int* weight) {
	FibonacciHeapElement* minimum = heap->minimum;
	if (minimum != NULL) {
		// store the minimum
		*vertex = minimum->vertex;
		*via = minimum->via;
		*weight = minimum->weight;

		// add all childs of minimum to the parent list
		for (int i = 0; i < minimum->childrens; i++) {
			FibonacciHeapElement* child = minimum->child;
			if (child == child->right) {
				minimum->child = NULL;
			} else {
				minimum->child = child->right;
				child->right->left = child->left;
				child->left->right = child->right;
			}
			child->parent = NULL;
			child->right = minimum;
			child->left = minimum->left;
			minimum->left->right = child;
			minimum->left = child;
		}

		// remove minimum
		if (minimum == minimum->right) {
			heap->minimum = NULL;
		} else {
			minimum->right->left = minimum->left;
			minimum->left->right = minimum->right;
			heap->minimum = minimum->right;
		}
		heap->size--;
		heap->positions[minimum->vertex] = NULL;
		free(minimum);
		if (heap->size > 0) {
			consolidateFibonacciMinHeap(heap);
		}
	}
}

/*
 * prints the adjacency list
 */
void printAdjacencyList(const AdjacencyList* list) {
	for (int i = 0; i < list->elements; i++) {
		printf("%d:", i);
		for (int j = 0; j < list->lists[i].size; j++) {
			printf(" %d(%d)", list->lists[i].elements[j].vertex,
					list->lists[i].elements[j].weight);
		}
		printf("\n");
	}
}

/*
 * print binary min heap
 */
void printBinaryHeap(const BinaryMinHeap* heap) {
	for (int i = 0; i < heap->size; i++) {
		printf("[%d]%d: %d(%d) ", heap->positions[heap->elements[i].vertex],
				heap->elements[i].vertex, heap->elements[i].via,
				heap->elements[i].weight);
		if (log2(i + 2) == (int) log2(i + 2)) {
			// line break after each stage
			printf("\n");
		}
	}
	printf("\n");
}

/*
 * print fibonacci min heap
 */
void printFibonacciHeap(const FibonacciMinHeap* heap,
		FibonacciHeapElement* startElement) {
	if (heap->size > 0) {
		FibonacciHeapElement* currentElement = startElement;
		printf("[%d]:", startElement->vertex);
		do {
			printf(" (%d,%d)%d|%d|%d", currentElement->marked,
					currentElement->childrens, currentElement->vertex,
					currentElement->via, currentElement->weight);
			currentElement = currentElement->right;
		} while (currentElement != startElement);
		printf("\n");
		do {
			if (currentElement->child != NULL) {
				printf("{%d}", currentElement->vertex);
				printFibonacciHeap(heap, currentElement->child);
				printf("\n");
			}
			currentElement = currentElement->right;
		} while (currentElement != startElement);
	} else {
		printf("heap is empty!\n");
	}
}

/*
 * print the graph as a maze to console
 */
void printMaze(const WeightedGraph* graph, const int rows, const int columns) {
	// initialize the maze with spaces
	int rowsMaze = rows * 2 - 1;
	int columnsMaze = columns * 2 - 1;
	char maze[rowsMaze][columnsMaze];
	memset(maze, EMPTY_FIELD, rowsMaze * columnsMaze * sizeof(char));

	// each vertex is represented as a plus sign
	for (int i = 0; i < rowsMaze; i++) {
		for (int j = 0; j < columnsMaze; j++) {
			if (i % 2 == 0 && j % 2 == 0) {
				maze[i][j] = VERTEX;
			}
		}
	}

	// each edge is represented as dash or pipe sign
	for (int i = 0; i < graph->edges; i++) {
		int from;
		int to;
		if (graph->edgeList[i * EDGE_MEMBERS]
				< graph->edgeList[i * EDGE_MEMBERS + 1]) {
			from = graph->edgeList[i * EDGE_MEMBERS];
			to = graph->edgeList[i * EDGE_MEMBERS + 1];
		} else {
			to = graph->edgeList[i * EDGE_MEMBERS];
			from = graph->edgeList[i * EDGE_MEMBERS + 1];
		}
		int row = from / columns + to / columns;
		if (row % 2) {
			// edges in even rows are displayed as pipes
			maze[row][(to % columns) * 2] = VERTICAL_EDGE;
		} else {
			// edges in uneven rows are displayed as dashes
			maze[row][(to % columns - 1) * 2 + 1] = HORIZONTAL_EDGE;
		}
	}

	// print the char array to the console
	for (int i = 0; i < rowsMaze; i++) {
		for (int j = 0; j < columnsMaze; j++) {
			printf("%c", maze[i][j]);
		}
		printf("\n");
	}
}

/*
 * print the components of the set
 */
void printSet(const Set* set) {
	for (int i = 0; i < set->elements; i++) {
		printf("%d: %d(%d)\n", i, set->canonicalElements[i], set->rank[i]);
	}
}

/*
 * print all edges of the graph in "from to weight" format
 */
void printWeightedGraph(const WeightedGraph* graph) {
	for (int i = 0; i < graph->edges; i++) {
		for (int j = 0; j < EDGE_MEMBERS; j++) {
			printf("%d\t", graph->edgeList[i * EDGE_MEMBERS + j]);
		}
		printf("\n");
	}
}

/*
 * process the command line parameters and return a Handle struct with them
 */
Handle processParameters(int argc, char* argv[]) {
	Handle handle = { .algorithm = 0, .columns = 3, .help = false,
			.maze = false, .create = false, .rows = 2, .verbose = false,
			.graphFile = "maze.csv" };

	for (int currentArgument = 1; currentArgument < argc; currentArgument++) {
		switch (argv[currentArgument][1]) {
		case 'a':
			// choose algorithm
			handle.algorithm = atoi(&argv[currentArgument + 1][0]);
			currentArgument++;
			break;
		case 'c':
			// set number of columns
			handle.columns = atoi(&argv[currentArgument + 1][0]);
			currentArgument++;
			break;
		case 'f':
			// set graph file location
			handle.graphFile = &argv[currentArgument + 1][0];
			currentArgument++;
			break;
		case 'h':
			// print help message
			printf(
					"Parameters:\n"
							"\t-a <int>\tchoose algorithm: 0 Kruskal (default), 1 Prim (Fibonacci), 2 Prim (Binary), 3 Boruvka\n"
							"\t-c <int>\tset number of columns (default: 3)\n"
							"\t-h\t\tprint this help message\n"
							"\t-m\t\tprint the resulting maze to console at the end (correct number of rows and columns needed!)\n"
							"\t-n\t\tcreate a new maze file\n"
							"\t-r <int>\tset number of rows (default: 2)\n"
							"\t-v\t\tprint more information\n"
							"\nThis program is distributed under the terms of the LGPLv3 license\n");
			handle.help = true;
			break;
		case 'm':
			// print the resulting maze to console at the end
			handle.maze = true;
			break;
		case 'n':
			// create a new maze file
			handle.create = true;
			break;
		case 'r':
			// set number of rows
			handle.rows = atoi(&argv[currentArgument + 1][0]);
			currentArgument++;
			break;
		case 'v':
			// print more information
			handle.verbose = true;
			break;
		default:
			fprintf(stderr, "Wrong parameter: %s\n"
					"-h for help\n", argv[currentArgument]);
			exit(EXIT_FAILURE);
		}
	}

	return handle;
}

/*
 * add edge to adjacency list
 */
void pushAdjacencyList(AdjacencyList* list, const int from, const int to,
		const int weight) {
	List* lists[2] = { &list->lists[from], &list->lists[to] };
	for (int i = 0; i < 2; i++) {
		if (lists[i]->size == lists[i]->alloced) {
			lists[i]->elements = (ListElement*) realloc(lists[i]->elements,
					2 * lists[i]->alloced * sizeof(ListElement));
			lists[i]->alloced *= 2;
		}

		// add element at the end
		if (i == 0) {
			lists[i]->elements[lists[i]->size] = (ListElement ) { .vertex = to,
							.weight = weight };
		} else {
			lists[i]->elements[lists[i]->size] = (ListElement ) {
							.vertex = from, .weight = weight };
		}
		lists[i]->size++;
	}
}

/*
 * push a new element to the end of a binary heap, then bubble up
 */
void pushBinaryMinHeap(BinaryMinHeap* heap, const int vertex, const int via,
		const int weight) {
	if (heap->size == heap->alloced) {
		// double the size if heap is full
		heap->elements = (BinaryHeapElement*) realloc(heap->elements,
				2 * heap->alloced * sizeof(BinaryHeapElement));
		heap->alloced *= 2;
	}

	heap->elements[heap->size] = (BinaryHeapElement ) { .vertex = vertex, .via =
					via, .weight = weight };
	heap->positions[vertex] = heap->size;

	heapifyBinaryMinHeap(heap, heap->size);

	heap->size++;
}

/*
 * add a new element
 */
void pushFibonacciMinHeap(FibonacciMinHeap* heap, const int vertex,
		const int via, const int weight) {
	FibonacciHeapElement* element = (FibonacciHeapElement*) malloc(
			sizeof(FibonacciHeapElement));
	newFibonacciHeapElement(element, vertex, via, weight, element, element,
	NULL, NULL);
	heap->positions[element->vertex] = element;

	// insert as root element
	insertFibonacciMinHeap(heap, element);
	heap->size++;
}

/*
 * read a previously generated maze file and store it in the graph
 */
void readGraphFile(WeightedGraph* graph, const char inputFileName[]) {
	// open the file
	FILE* inputFile;
	const char* inputMode = "r";
	inputFile = fopen(inputFileName, inputMode);
	if (inputFile == NULL) {
		fprintf(stderr, "Couldn't open input file, exiting!\n");
		exit(EXIT_FAILURE);
	}

	int fscanfResult;

	// first line contains number of vertices and edges
	int vertices = 0;
	int edges = 0;
	fscanfResult = fscanf(inputFile, "%d %d", &vertices, &edges);
	newWeightedGraph(graph, vertices, edges);

	// all lines after the first contain the edges
	// values stored as "from to weight"
	int from;
	int to;
	int weight;
	for (int i = 0; i < edges; i++) {
		fscanfResult = fscanf(inputFile, "%d %d %d", &from, &to, &weight);
		graph->edgeList[i * EDGE_MEMBERS] = from;
		graph->edgeList[i * EDGE_MEMBERS + 1] = to;
		graph->edgeList[i * EDGE_MEMBERS + 2] = weight;
		// EOF result of *scanf indicates an error
		if (fscanfResult == EOF) {
			fprintf(stderr,
					"Something went wrong during reading of graph file, exiting!\n");
			fclose(inputFile);
			exit(EXIT_FAILURE);
		}
	}

	fclose(inputFile);
}

/*
 * scatter the edge list of a graph
 */
void scatterEdgeList(int* edgeList, int* edgeListPart, const int elements,
		int* elementsPart) {
	int rank;
	int size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	MPI_Scatter(edgeList, *elementsPart * EDGE_MEMBERS, MPI_INT, edgeListPart,
			*elementsPart * EDGE_MEMBERS,
			MPI_INT, 0, MPI_COMM_WORLD);

	if (rank == size - 1 && elements % *elementsPart != 0) {
		// number of elements and processes isn't divisible without remainder
		*elementsPart = elements % *elementsPart;
	}

	if (elements / 2 + 1 < size && elements != size) {
		if (rank == 0) {
			fprintf(stderr, "Unsupported size/process combination, exiting!\n");
		}
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}
}

/*
 * sort the edges of the graph in parallel with mergesort in parallel
 */
void sort(WeightedGraph* graph) {
	int rank;
	int size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Status status;

	bool parallel = size != 1;

	// send number of elements
	int elements;
	if (rank == 0) {
		elements = graph->edges;
		MPI_Bcast(&elements, 1, MPI_INT, 0, MPI_COMM_WORLD);
	} else {
		MPI_Bcast(&elements, 1, MPI_INT, 0, MPI_COMM_WORLD);
	}

	// scatter the edges to sort
	int elementsPart = (elements + size - 1) / size;
	int* edgeListPart = (int*) malloc(
			elementsPart * EDGE_MEMBERS * sizeof(int));
	if (parallel) {
		scatterEdgeList(graph->edgeList, edgeListPart, elements, &elementsPart);
	} else {
		edgeListPart = graph->edgeList;
	}

	// sort the part
	mergeSort(edgeListPart, 0, elementsPart - 1);

	if (parallel) {
		// merge all parts
		int from;
		int to;
		int elementsRecieved;
		for (int step = 1; step < size; step *= 2) {
			if (rank % (2 * step) == 0) {
				from = rank + step;
				if (from < size) {
					MPI_Recv(&elementsRecieved, 1, MPI_INT, from, 0,
					MPI_COMM_WORLD, &status);
					edgeListPart = realloc(edgeListPart,
							(elementsPart + elementsRecieved) * EDGE_MEMBERS
									* sizeof(int));
					MPI_Recv(&edgeListPart[elementsPart * EDGE_MEMBERS],
							elementsRecieved * EDGE_MEMBERS,
							MPI_INT, from, 0, MPI_COMM_WORLD, &status);
					merge(edgeListPart, 0, elementsPart + elementsRecieved - 1,
							elementsPart - 1);
					elementsPart += elementsRecieved;
				}
			} else if (rank % step == 0) {
				to = rank - step;
				MPI_Send(&elementsPart, 1, MPI_INT, to, 0, MPI_COMM_WORLD);
				MPI_Send(edgeListPart, elementsPart * EDGE_MEMBERS, MPI_INT, to,
						0,
						MPI_COMM_WORLD);
			}
		}

		// edgeListPart is the new edgeList of the graph, cleanup other memory
		if (rank == 0) {
			free(graph->edgeList);
			graph->edgeList = edgeListPart;
		} else {
			free(edgeListPart);
		}
	} else {
		graph->edgeList = edgeListPart;
	}
}

/*
 * helper function to swap binary heap elements
 */
void swapBinaryHeapElement(BinaryMinHeap* heap, const int position1,
		const int position2) {
	heap->positions[heap->elements[position1].vertex] = position2;
	heap->positions[heap->elements[position2].vertex] = position1;

	BinaryHeapElement swap = heap->elements[position1];
	heap->elements[position1] = heap->elements[position2];
	heap->elements[position2] = swap;
}

/*
 * merge the set of parent1 and parent2 with union by rank
 */
void unionSet(Set* set, const int parent1, const int parent2) {
	int root1 = findSet(set, parent1);
	int root2 = findSet(set, parent2);

	if (root1 == root2) {
		return;
	} else if (set->rank[root1] < set->rank[root2]) {
		set->canonicalElements[root1] = root2;
	} else if (set->rank[root1] > set->rank[root2]) {
		set->canonicalElements[root2] = root1;
	} else {
		set->canonicalElements[root1] = root2;
		set->rank[root2] = set->rank[root1] + 1;
	}
}
