/* ApproxIndex.c

   Author: Paolo Ferragina, University of Pisa, Italy
   Home page: http://pages.di.unipi.it/ferragina
   Copyright: MIT License
   Date: July 2018


This program indexes the file "oldFile.dat" in order to support searches for <=2 mismatches over query stringas whose length is a multiple of 4 and are longer than 12 bytes. 

The key idea is to partition the queryString (of length a multiple of 4) in four pieces and then searching
EXACTLY all possible combinations of pairs of these 4 pieces, which are 6 overall. 

This allows to have longer exact matches, that should therefore reduce the false positive rate.

This is a filtering approach which could be improved in time/space whether you admit that the position of a match is "indicative" and then you can search for the real "match" by looking around the returned position. In this case, you can halve the space and the search time by indexing only the three (instead of six) pairs of the 4 pieces which are in positions 01, 02, 03.

You compile the program with 

gcc -lm -O3 ApproxIndex.c -oApproxIndex 

and then you can run it with 

./ApproxIndex XXXXXXXXXXXX

where the sequence of Xs is the query string of 12 chars. This is a trivial interface, you can search for any sequence of byte by properly passing them to queryStr inside the program.

The program returns the positions which match up to k-hamming distance with the searched string.

*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>



// ---- MAIN TYPES AND DATA ----




typedef long PosType;			// Position in file
typedef unsigned long SigType;          // Hash value

typedef struct hnode *Hptr;
typedef struct hnode {           
  Hptr	next;
  SigType sig;            // fingerprint of the qgram
  PosType pos;            // starting position of the qgram
  int firstBlockPos;      // 0,1,2
  int secondBlockPos;     // firstBlockPos+1,...,3
  unsigned char *block;   // content of the qgram
} Hnode;


#define HSIZE 67867979     // Hash table size

Hptr htab[HSIZE];          // Hash Table

unsigned char *oldText;   // Input file to index
int  oldTextLength=0;






// ----- AUXILIARY SIMPLE FUNCTIONS -----

void assert(int c, const char *s)
{	if (!c) {
		fprintf(stderr, "%s\n", s);
		exit(0);
	}
}



// qsort comparison function over PosType 
int int_cmp(const void *a, const void *b) 
{ 
  const PosType *ia = (const PosType *)a; // casting pointer types 
  const PosType *ib = (const PosType *)b;
  return *ia  - *ib; 
}


// Removes duplicate elements, returning the new size of modified array.
int removeDuplicates(PosType *arr, int n)
{
  if (n==0 || n==1)
    return n;

  PosType temp[n];

  // Start traversing elements
  int j = 0;
  for (int i=0; i<n-1; i++)

    if (arr[i] != arr[i+1])
      temp[j++] = arr[i];

  // Store the last element 
  temp[j++] = arr[n-1];

  // Modify original array
  for (int i=0; i<j; i++)
    arr[i] = temp[i];

  return j;
}




// ----- PRINTING BLOCKS -----

void printBlock(unsigned char *text, int len)
{
  for (int y=0; y < len; y++){
    unsigned char c = text[y];
    if (c>=32) fprintf(stderr, "%c", c);
    else fprintf(stderr, ".");
  }
}

// print HEX the block of length len
void printBlockHex(unsigned char *text, int len)
{
  for (int y=0; y < len; y++){
    unsigned char c = text[y];
    fprintf(stderr, "%.2x ", (unsigned char)c);
  }
}


// ----- FUNCTIONS ON HASH TABLE  -----


// returns the hashing of a block[] of size len 
SigType hashTable(int len, unsigned char *block)
{
  SigType hash = 5381;
  int c;

  for(int i=0; i < len; i++){
    c = block[i];
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return (hash % HSIZE);
}


// returns the hashing of a block[] of size len 
SigType hashBlock(int len, unsigned char *block)
{
  SigType hash;
  int i;

  hash = 0;
  for(i = 0; i < len; i++)
    {
      hash += block[i];
      hash += (hash << 10);
      hash ^= (hash >> 6);
    }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);
  return (hash % HSIZE);
}



// check blocks (as hash's element) for equality: 1 = equal, 0 = different 
int checkBlock(Hptr p, unsigned char *block, int len) {

  if (memcmp(block, p->block, len) == 0) return 1;
  else return 0;
}


// Insert at the head of the list a block[] of length len
void insert(PosType i, int len, unsigned char *block, int firstPiece, int secondPiece)
{  
  // hash entry
  int ht = (int) hashTable(len, block);

  // stronger hash for block to store
  SigType hb = hashBlock(len, block);
  Hptr p = (Hptr) malloc(sizeof(Hnode));
  assert(p != 0, "malloc died in hash insert");

  p->next = htab[ht];
  htab[ht] = p;

  // storing infos about the inserted block
  p->sig = hb;
  p->pos = i;
  p->firstBlockPos = firstPiece;
  p->secondBlockPos = secondPiece;
  p->block = block;
}


// Search block of length "len" constructed from the firstPiece+secondPiece blocks
// it returns an array of results ended by -1 (which cannot be a position)
PosType *search(unsigned char *block, int len, int firstPiece, int secondPiece)
{
  int ht = (int) hashTable(len, block);
  SigType hb = hashBlock(len, block);

  Hptr p;

  PosType *results = (PosType *) malloc(sizeof(PosType) * (oldTextLength+1));
  int j=0;

  for (p = htab[ht]; p; p = p->next)
    if ((p->sig == hb) && (checkBlock(p,block,len)) 
	&& (p->firstBlockPos == firstPiece) 
	&& (p->secondBlockPos == secondPiece)) { 
      results[j++] = p->pos; 
    }

  results[j]=-1;
  return results; //list of results
}



// ----- MAIN PROCEDURE -----

int main(int argc, char *argv[])
{
  FILE *old_file;    
  const char *oldFileName = "old_file.dat";
  

  // ARGV[1] = string to be searched (assume ended by \0)
  unsigned char *queryStr = (unsigned char *) malloc(100); // assume max 100 bytes for the query
  for(int i=0; i<50; i++)
    queryStr[i]=0;
  for(int i=0; i<strlen(argv[1]); i++)
    queryStr[i]=argv[1][i];
  
  int queryLen = strlen(argv[1]);
  if (queryLen % 4 != 0){
    printf("Error, query length should be a multiple of 4\n\n");
    exit(1);
  }


  int blockSize = queryLen/4;  //We split the queryString in 4 blocks of equal length

  // fetch the old file in oldText 
  fprintf(stderr,"  fetching file...");
  old_file = fopen(oldFileName, "r");
  if (old_file == NULL) {
    fprintf(stderr,"\n\nError: Unable to open %s\n",oldFileName);
    exit (8);  }
  fseek(old_file, 0, SEEK_END);

  oldTextLength = (PosType) ftell(old_file);
  fseek(old_file, 0, SEEK_SET);

  oldText = (unsigned char *) malloc(oldTextLength+1);
  fread(oldText, 1, oldTextLength, old_file);
  fclose(old_file);
  oldText[oldTextLength] = 0; // ended by \0

  fprintf(stderr,"\n%s\n\n",oldText);
  fprintf(stderr,"... fetched!!\n");


  // Construct the dictionary of blocks of size 2 * blockSize
  fprintf(stderr,"Building hash table...");
    
  int qgramSize = 2 * blockSize;
  for (int i = 0; i < oldTextLength-queryLen+1; i++) {

    fprintf(stderr,"\n\n %d - check:",i);
    printBlock(oldText+i,queryLen);
    fprintf(stderr, "\n");
	
    // Take a qgram as 2 blocks, each of size blockSize characters
    for(int first=0; first < 3; first++){
      for(int second = first+1; second <= 3; second++){
	
	unsigned char *blockTmp = (unsigned char *) malloc (qgramSize+1);  //allocate memory for the block
	blockTmp[qgramSize] = 0;
	for(int l=0; l < blockSize; l++){
	  blockTmp[l] = oldText[i + first * blockSize + l];
	  blockTmp[l+blockSize] = oldText[i + second * blockSize + l];
	}
	
	printBlock(blockTmp,qgramSize);
	fprintf(stderr, "\n");
	insert(i, qgramSize, blockTmp, first, second);
      } // end second
    } // end first

    if (i % 1000000 == 0) fprintf(stderr, ".");

  }



  // ************ QUERY
  fprintf(stderr,"\n\n ***** QUERY *****\n\n");
  PosType *r = (PosType *)malloc((oldTextLength+1) * sizeof(PosType));
  int rSize = 0;
  PosType *r_tmp;

  for(int first=0; first < 3; first++){
    for(int second = first+1; second <= 3; second++){
      
      //allocate memory and create the block to be searched exactly
      unsigned char *blockTmp = (unsigned char *) malloc (qgramSize+1);  
      blockTmp[qgramSize] = 0;
      for(int l=0; l < blockSize; l++){
	blockTmp[l] = queryStr[first * blockSize + l];
	blockTmp[l+blockSize] = queryStr[second * blockSize + l];
      }
      
      printBlock(blockTmp,qgramSize);
      fprintf(stderr, "   searching.... ");
      
      // Compute results and add to the final set
      r_tmp = search(blockTmp,qgramSize,first,second);
      
      for(int j=0; r_tmp[j] != -1; j++){
	  r[rSize++] = r_tmp[j];
	  // fprintf(stderr,"%ld\n",r_tmp[j]);
      }
      
      fprintf(stderr,"%d\n\n",rSize);
      
    } // end second
  } // end first
  
  // remove duplicates
  heapsort(r, rSize, sizeof(PosType), &int_cmp);
  rSize = removeDuplicates(r, rSize);

  // Results available in r[] and their are rSize
  for(int j=0; j < rSize; j++)
    fprintf(stderr,"%ld\n",r[j]);
  exit(0);
}


