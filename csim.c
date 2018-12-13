#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

//dkumykova-tgsprowl

typedef struct cacheLine {
	int validBit;
	int times_accessed;
	int sIndex;
	unsigned long tag;
	int blockSize;

} cacheLine;

typedef struct cacheSet {
	cacheLine *lines;
} cacheSet;

typedef struct cache {
	cacheSet *sets;

} cache;
//for line read in from trace file
typedef struct cacheTrace {
	char* instruction;
	unsigned long memAddress;
	int* instructSize;
} cacheTrace;

///keep track
int hits = 0;
int misses = 0;
int evictions = 0;

cache make2Dcache(int nsets, int nlines); //creates and allocates space for new cache
void printInstructions(); //prints out help instructions
void printInstruct(FILE* input, cacheTrace t); //prints the line from trace file
long calcTag(unsigned long mem_address, int blocksize, int setIndexBits); //calculates tag of a given cache line
int calcSetIndex(long tag, int blocksize, int setIndexBits); //calculates set index of a cache line
int matchLine(cache *test, long tag, int setIndex, int nlines); //returns the index of a line if it is within a cache
int findEmptyLine(cache *test, int setIndex, int nlines); //returns the index of an emtpy line or the least recently
//used line
void runSim(cache test, cacheLine parsedLine, long tag, int numLines,
		int loopCounter, int i, int index);

int main(int argc, char* argv[]) {
	//takes as input number of sets, lines per set, number of block bits, number of set index bits,
	//and filename
	int help = 0;
	int verbose = 0;
	int setIndexBits = 0;
	int blockBits = 0;
	int numLines = 0;
	char *fileName;

	extern char *optarg;
	extern int optind;
	int c;

	//parse user input
	while ((c = getopt(argc, argv, "hvs:E:v:b:t:")) != -1) {
		switch (c) {
		case 'h':
			help = 1; //calling the help flag gives the instructions
			printInstructions();
			break;
		case 'v':
			verbose = 1;
			break;
		case 's':
			setIndexBits = atoi(optarg);
			break;
		case 'E':
			numLines = atoi(optarg);
			break;
		case 'b':
			blockBits = atoi(optarg);
			break;
		case 't':
			fileName = optarg;
			break;

		}
	}

	//test print
	printf("help = %d\n", help);
	printf("verbose = %d\n", verbose);
	printf("setIndexBits = %d\n", setIndexBits);
	printf("linesPer = %d\n", numLines);
	printf("blockBits = %d\n", blockBits);
	printf("filename = %s\n", fileName);

	//define a cache trace to use to read in trace file lines
	cacheTrace T1;
	T1.instruction = (char*) malloc(sizeof(char) * 30);
	T1.memAddress = 0L;
	T1.instructSize = (int*) malloc(sizeof(int));

	//dimensions for cache
	int numSets = 2 << setIndexBits;
	int blockSize = 2 << blockBits;

	//create cache with given dimensions
	cache test;
	test = make2Dcache(numSets, numLines);

	//open and parse given trace file
	FILE *fp;
	fp = fopen(fileName, "r");
	if (fp == NULL) {
		perror("Error opening file");
		return (-1);
	}
	int loopCounter = 0; //keeps track of number of loops function goes through to act as clock for number of times
						 //each line is accessed

	//loops through file until finished, performs operation on each line; line is printed
	while ((fscanf(fp, " %c %lx,%d\n", T1.instruction, &T1.memAddress,
			T1.instructSize)) != EOF) {
		loopCounter++;
		printf("\nmemAddress: %ld\n", T1.memAddress);

		long tag = calcTag(T1.memAddress, blockSize, setIndexBits);
		long index = calcSetIndex(T1.memAddress, blockBits, setIndexBits);
		printf("Tag: %ld\n", tag);
		printf("Index: %ld\n", index);
		printInstruct(fp, T1);

		//create line from instruction read
		cacheLine parsedLine;
		parsedLine.validBit = 0;
		parsedLine.blockSize = blockSize;
		parsedLine.sIndex = index;
		parsedLine.tag = tag;
		parsedLine.times_accessed = 0; //assign number of times the line is accessed to number of times the loop has parsed that line

		int i = 0; //index for lines array

		if (strcmp(T1.instruction, "L") == 0
				|| (strcmp(T1.instruction, "M")) == 0) { //load, finds address of what looking for; including M makes sure M is done twice when called
			runSim(test, parsedLine, tag, numLines, loopCounter, i, index);

		}
		if ((strcmp(T1.instruction, "S")) == 0
				|| (strcmp(T1.instruction, "M")) == 0) { //store, similar to load
			runSim(test, parsedLine, tag, numLines, loopCounter, i, index);
		}
	} //otherwise it's I, which we do nothing with

	fclose(fp);

	printSummary(hits, misses, evictions);
	return 0;
}

/**Runs the cache simulator
 * @param test, cache being searched and loaded into
 * @param parsedLine, line being searched for, given by trace file instruction
 * @apram tag, tag for given line
 * @param numLines, number of lines in each set
 * @param loopCounter, counts number of iterations through this loop
 * @param i, iterator to go through lines
 * @param index, set index for given line
 *
 */
void runSim(cache test, cacheLine parsedLine, long tag, int numLines,
		int loopCounter, int i, int index) {
	loopCounter++;
	int lineIndexInCache = matchLine(&test, tag, index, numLines); //tries to find given line in the cache by returning index

	if (lineIndexInCache == -1) { //miss
		int lineToReplaceIndex = findEmptyLine(&test, index, numLines); //returns index of empty line

		//place the value at index lineToReplaceIndex to the parsedLine values given from the read in address
		test.sets[index].lines[lineToReplaceIndex].blockSize =
				parsedLine.blockSize;
		test.sets[index].lines[lineToReplaceIndex].sIndex = parsedLine.sIndex;
		test.sets[index].lines[lineToReplaceIndex].tag = parsedLine.tag;
		test.sets[index].lines[lineToReplaceIndex].times_accessed = loopCounter;
		test.sets[index].lines[lineToReplaceIndex].validBit = 1;

		printf("Miss.\n");
		misses++;
		if (i == numLines) { //iterate through all lines in set
			i = 0;
		} else {
			i++; //go to next line within array of lines for parsing
		}
	} else { //hit
		printf("Hit.\n");
		test.sets[index].lines[lineIndexInCache].times_accessed = loopCounter; //update times_accessed to equals number of loops
		hits++;

	}
}

/**Returns the index of an empty line, or if the cache is full, the line
 * to be evicted when putting a new line into the cache.
 * @param test, cache in which we are looking for a line to replace
 * @param setIndex, details which specific set we are looking for a line
 * @param nlines, number of lines in that set
 * @return the index of the line in the set to be replaced
 */
int findEmptyLine(cache *test, int setIndex, int nlines) {
	int last_used = test->sets[setIndex].lines[0].times_accessed; //keep track of line accessed least amount

	int index = 0; //return value, initialized

	int i; //iterator
	for (i = 0; i < nlines; i++) {
		if (test->sets[setIndex].lines[i].validBit == 0) { //this is an empty line
			index = i; //assign and return index of empty line immediately
			return index;
		} else { //this means the set is full, because no empty line has been found

			if (last_used > test->sets[setIndex].lines[i].times_accessed) {
				last_used = test->sets[setIndex].lines[i].times_accessed;
				index = i;
			}
		}
	}
//handle eviction case, because iterated through lines and no empty one;
//clear line by setting valid bit to 0
	test->sets[setIndex].lines[index].validBit = 0;
	evictions++;
	printf("Eviction.\n");

	return index; //return index of the empty line within the cache
}
/**Returns the index of the line if it is located within the cache; if not,
 * returns -1
 *@param test, cache in which we are looking for the given line
 *@param tag, tag bits for given line
 *@param setIndex, set index for given line
 *@param nline, number of lines in cache
 *@return -1 as index if line is not within the cache; otherwise returns index of line in cache
 */
int matchLine(cache *test, long tag, int setIndex, int nlines) {
//different logic
	cacheSet set = test->sets[setIndex];
	int index = -1;

	for (int i = 0; i < nlines; i++) {
		if (set.lines[i].tag == tag && set.lines[i].validBit == 1) { //tag matches and bit is valid
			index = i;
		}

	}
	return index; //returns index of line within cache

}

/**Calculates the set index of a line
 * @param mem_address, the given address in memory for the line being evaluated
 * @param blockBits, number of block bits in line (read in from argv)
 * @param setIndexBits, number of set index bits (read in from argv)
 * @return the set index
 */
int calcSetIndex(long mem_address, int blockBits, int setIndexBits) {
	printf("CALC SI addr %ld  b %d s %d", mem_address, blockBits, setIndexBits);

	long tag_and_s_index = mem_address >> blockBits; //shift line over right # block bits to get rid of the block bits at the end
	long stuff = tag_and_s_index >> setIndexBits; //account for signed mem address
	long index = tag_and_s_index ^ (stuff << setIndexBits); //gets rid of garbage bits

	return index;
}

/**Calculates the tag of a line
 * @param mem_address, the given address in memory for the line being evaluated
 * @param blocksize, size of block offset
 * @param setIndexBits, number of set index bits
 * @return the tag
 */
long calcTag(unsigned long mem_address, int blocksize, int setIndexBits) {

	long tag = mem_address - (mem_address % blocksize);

	return tag;
}

/**Creats a 2D array --> cache, with all variable within the lines set
 * to 0 and initialized. Based off of make2dchar function from Game of Life
 * @param nsets, number of sets in cache
 * @param nlines, number of lines per set
 * @return built and initialized to 0 cache
 */
cache make2Dcache(int nsets, int nlines) {

	cache testCache; // Array of pointers to rows
	int sIndex;
	int lIndex;
//allocate array of pointers to rows
	testCache.sets = (cacheSet*) malloc(nsets * sizeof(cacheSet)); //space for sets
	for (sIndex = 0; sIndex < nsets; sIndex++) {
		testCache.sets[sIndex].lines = (cacheLine*) malloc(
				nlines * sizeof(cacheLine)); //space for lines

		//initialize everything in the lines to 0, clean cache
		for (lIndex = 0; lIndex < nlines; lIndex++) {
			testCache.sets[sIndex].lines[lIndex].times_accessed = 0;
			testCache.sets[sIndex].lines[lIndex].validBit = 0;
			testCache.sets[sIndex].lines[lIndex].tag = 0;
			testCache.sets[sIndex].lines[lIndex].blockSize = 0;
			testCache.sets[sIndex].lines[lIndex].sIndex = sIndex;
		}
	}

	return testCache;
}

/**Prints out help statements if the -h flag is called
 * @param argv, the input arguments given my user
 * @return void
 */
void printInstructions() {
	printf(
			"How to call: ./csim [-hv] -s <num> -E <num> -b <num> -t <filename>\n");
	printf("Arguments must be in order. Options are as follows: \n");
	printf(" -h   Call this to view help options.\n");
	printf(" -v   Verbose flag, optional\n");
	printf(" -s <num>  Number of set index bits for cache\n");
	printf(" -E <num>  Number of lines per set\n");
	printf(" -b <num>  Number of block offset bits\n");
	printf(" -t <filename> Trace file\n\n");
	printf(" Example: ./csim -h -v -s 2 -E 4 -b 3 -t traces/dave.trace\n");

	exit(0);
}

/**Reads each line from the input file and places into a cacheTrace struct, then prints out each line; Reads
 * in middle value, the hex address, as hex.
 * @param input, the input trace file
 * @return void
 */
void printInstruct(FILE* input, cacheTrace t) {

	printf("%c, %lx, %d ", *t.instruction, t.memAddress, *t.instructSize);
	if (EOF) {
	}
}
