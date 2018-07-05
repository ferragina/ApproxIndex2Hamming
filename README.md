# ApproxIndex2Hamming

   Author: Paolo Ferragina, University of Pisa, Italy
   
   Home page: http://pages.di.unipi.it/ferragina
   
   Copyright: MIT License
   
   Date: July 2018


This program indexes the file "oldFile.dat" in order to support searches for <=2 mismatches over query strings whose length is a multiple of 4 and are longer than 12 bytes. 

The key idea is to partition the queryString (of length a multiple of 4) in four pieces and then search EXACTLY all possible combinations of pairs of these 4 pieces, which are 6 overall. 

This allows to have longer exact matches, that should therefore reduce the false positive rate.

This is a filtering approach which could be improved in time/space if you admit that the position returned by the program is "indicative of a match", so that a subsequent search phase can search for the real "match" by looking around the returned position. In this case, you can e.g. halve the space and the search time by indexing only the three (instead of six) pairs of the 4 pieces which are in positions 01, 02, 03.

Another optimization is that I'm loading all qgrams to be matched in one hash table, whereas you could build 6 independent hash tables, that would therefore speedup the searches.

You compile the program with: gcc -lm -O3 ApproxIndex.c -oApproxIndex 

and then you can run it with: ./ApproxIndex XXXXXXXXXXXX 
where the sequence of Xs is the query string of at least 12 chars and having multiple-4 length. This is a trivial interface, you can search for any sequence of byte by properly passing them to queryStr inside the program.

The program returns the positions which match up to k-hamming distance with the searched string.

