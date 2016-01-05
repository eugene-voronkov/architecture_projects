import java.util.*;
import java.io.*;

public class sim {
	CacheRow[][] cache;
	int b, c, s, p, numberOfSets, blocksPerSet, validityBit, numberOfDirtyBits, sizeTagBits;
	int blockMask, indexMask, tagMask;
	LinkedList<CacheRow>[] listLRU;
	int reads, readMisses, writes, writeMisses, writeBacks;
	int blockSize;
	
	PrintWriter outputStream;
	void setupCache(int b, int c, int s, int p) {
		this.b = b;
		this.c = c;
		this.s = s;
		this.p = p;
		
		reads = readMisses = writes = writeMisses = writeBacks = 0;

		//For testing purposes
        try {
            outputStream = 
            	new PrintWriter(new FileWriter("C:\\Users\\eugene\\Desktop\\p3\\characteroutput.txt"));
        } catch( Exception e) { }
		// [total blocks] = [total cache size] / [block size]
		// [total sets] = [total blocks ] / [blocks per set]
		// (2^c)/(2^b)(2^s) = 2^(c - b - s) = 1<<(c - b - s)
		// numberOfSets = 1<<(c - b - s);
		// ABOVE IS WRONG
		
		// # blocks per set = # index bits = [total bytes] / [byte blocks per block] * [# sets]
		// (2^c)/(2^b)(2^s) = 2^(c - b - s) = 1<<(c - b - s)
		blocksPerSet = 1<<(c - b - s);
		
		// By definition # sets = 2^s = 1<<s
		numberOfSets = 1<<s;
		
		validityBit = 1;
		
		// 1 dirty bit for every 4 bytes so 2^b / 4 = (2^b)/(2^2) = 2^(b-2) = 1<<(b-2)
		// numberOfDirtyBits = 1 << (b - 2);
		// ABOVE WRONG
		// ASSUMES EACH BLOCK OFFSET INDEXES 32 BITS
		// INSTRUCTIONS IMPLY EACH MEMORY ADDRESS AND THEREFORE EACH OFFSET POINTS TO A BYTE OF DATA
		// 1 dirty bit for every byte so [# dirty bits] = 2^b = 1<<b
		numberOfDirtyBits = 1<<b;

		// t = (address size) - (c + b)
		//sizeTagBits = 32 - c - b;
		// ABOVE IS WRONG
		// SHOULD BE
		// 32 - (c - b - s + b) = 32 - (c - s) = 32 + s - c
		sizeTagBits = 32 + s - c;


		// subtracting 1 from any binary number with 1 bit set, sets all the lower bits to 1.
		blockMask = (1<<b) - 1;
		//indexMask = (1<<c) - 1 - blockMask;
		//ABOVE COMMENTED CODE IS WRONG
		indexMask = (1<<(c-s)) - 1 - blockMask;
		tagMask = ~0 - indexMask - blockMask;
		


		// setup Least Recently Used Lists
		// one list for each index
		listLRU = new LinkedList[blocksPerSet];
		for(int j = 0; j < blocksPerSet; ++j) {
			listLRU[j] = new LinkedList<CacheRow>();
		}

		//Setup cache as an array of "lines" broken up into sets
		//cache[setIndex][blockIndex]
		cache = new CacheRow[numberOfSets][blocksPerSet];
		for(int i = 0; i < numberOfSets; ++i) {
			for(int j = 0; j < blocksPerSet; ++j) {
				cache[i][j] = new CacheRow(b, c, s);
			}
		}

		int validBits = 1;
		int tagBits;
		int dirtyBits;
		//This assumes each block references 4 bytes of data
		blockSize = validBits + sizeTagBits + numberOfDirtyBits + (1<<b);
	}


	float calculateAAT() {
		return (float)((double)2+(0.2*s) + 50*((double)readMisses / (double)reads));
	}
	
	

	int getIndex( int address ) {
		return (address & indexMask) >> b;
	}
	
	int getBlock( int address ) {
		return (address & blockMask);
	}

	int getTag( int address ) {		
		//return (address & tagMask) >> (b + c);
		//COMMENTED CODE IS INCORRECT
		//shift away index bits + offset bits = (c - b - s) + b = c - s
		return (address & tagMask) >> (c - s);
	}

	
	
	
	// Returns true if this memory address is currently in cache
	boolean cacheHas( int address ) {
		return getFromCache(address) != null;
	}
	
	
	
	void prefetch( int address ) {
		//System.out.println( "entered prefetcher" );
		address++;
		for(int addressTemp = address; addressTemp < address + p; ++addressTemp) {
			
			//Check if getFromCache has cached
			//if getFromCache doesn't have in memory run "MISS CODE"
			
			CacheRow currentBlock;
			currentBlock = getFromCache(addressTemp);
			if ( currentBlock == null ) {
				//Create CacheRow data for this address
				currentBlock = new CacheRow(b, c, s);
				currentBlock.valid = 1;
				currentBlock.blockIndex = getIndex(addressTemp);
				//System.out.println( "readmiss   address: " + Integer.toString(address, 16) + " index: " + getIndex(address) + " tag: " + Integer.toString(getTag(address), 16) );

				
				
				//Insert into block where valid bit is 0
				int i;
				for(i = 0; i < numberOfSets && i >= 0; ++i) {
					if( cache[i][getIndex(addressTemp)].valid == 0 ) {
						currentBlock.setIndex = i;
						cache[i][getIndex(addressTemp)] = currentBlock;
						i = -5;
					}
				}
				//remove evictedBlock from LRU list if an invalid block wasn't found
				//this is indicated by i being equal to numberOfSets
				if( i == numberOfSets && !listLRU[ getIndex(addressTemp) ].isEmpty() ) {
					//CacheRow evictedBlock = listLRU[ getIndex(address) ].getLast();
					CacheRow evictedBlock = listLRU[ getIndex(addressTemp) ].removeLast();

					//Add created block to cache[][]
					currentBlock.setIndex = evictedBlock.setIndex;
					cache[ evictedBlock.setIndex ][ evictedBlock.blockIndex ] = currentBlock;

					//If  cached memory was modified and trashed, write back necessary
					if( !evictedBlock.dirtyBits.isEmpty() ) {
						//System.out.println( "* instr=w addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss: victim tag="+evictedBlock.tag+" WB");
						//outputStream.println( "* instr=r addr=" +Integer.toString(addressTemp,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss: victim tag="+Integer.toString(evictedBlock.tag,16)+" WB");
						//Each memory address holds 1<<b=2^b bytes of memory according to validation results.
						writeBacks += 1<<b;
					}
					//Else just trash
					else {
						//System.out.println( "* instr=r addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss: victim tag="+evictedBlock.tag+" WB");
						//outputStream.println( "* instr=r addr=" +Integer.toString(addressTemp,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss: victim tag="+Integer.toString(evictedBlock.tag,16));
					}
				}
				// Special case where LRU list is empty
				else if( i == numberOfSets && listLRU[ getIndex(addressTemp) ].isEmpty() ) {
					cache[0][ getIndex(addressTemp) ] = currentBlock;

					//System.out.println( "* instr=r addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss");
					//outputStream.println( "* instr=r addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss");
				}
				else {
					//System.out.println( "* instr=r addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss");
					//outputStream.println( "* instr=r addr=" +Integer.toString(addressTemp,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss");
				}
				
				listLRU[ getIndex(addressTemp) ].addFirst( currentBlock );

				//++reads;
				//++readMisses;
				//System.out.println( "prefetching reads: "+reads+"readMisses: "+readMisses );
			}
			
		}
	}
	
	// returns line containing address if it is currently in cache
	// otherwise returns null
	CacheRow getFromCache( int address ) {
		for(int set = 0; set < numberOfSets; ++set) {
			//debug statement below
/*
			System.out.println( "set: " + set);
			System.out.println( "index: " + getIndex(address) );
			System.out.println( "binary index: " + Integer.toString(getIndex(address), 2) );
			System.out.println( Integer.toString(blocksPerSet, 2) );
*/
			//System.out.println( "looking for tag: " + Integer.toString(getTag(address), 16) + " found address: " + Integer.toString(address, 16) + " index: " + getIndex(address) + " tag: " + Integer.toString(cache[set][ getIndex(address) ].tag, 16) );
			if( cache[set][ getIndex(address) ].tag == getTag(address) ) {
				//System.out.println( "found it" );
				return cache[set][ getIndex(address) ];
			}
		}
		
		return null;	
	}
	
	CacheRow read( int address ) {
		CacheRow currentBlock;
		currentBlock = getFromCache(address);
		//HIT
		if( currentBlock != null ) {
			//System.out.println( "* instr=r addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 hit");
			outputStream.println( "* instr=r addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 hit");
			updateLRU(address);
			++reads;
		}
		//MISS
		else {
			//Create CacheRow data for this address
			currentBlock = new CacheRow(b, c, s);
			currentBlock.valid = 1;
			currentBlock.tag = getTag(address);
			currentBlock.blockIndex = getIndex(address);
			//System.out.println( "readmiss   address: " + Integer.toString(address, 16) + " index: " + getIndex(address) + " tag: " + Integer.toString(getTag(address), 16) );

			
			
			//Insert into block where valid bit is 0
			int i;
			for(i = 0; i < numberOfSets && i >= 0; ++i) {
				if( cache[i][getIndex(address)].valid == 0 ) {
					currentBlock.setIndex = i;
					cache[i][getIndex(address)] = currentBlock;
					i = -5;
				}
			}
			//remove evictedBlock from LRU list if an invalid block wasn't found
			//this is indicated by i being equal to numberOfSets
			if( i == numberOfSets && !listLRU[ getIndex(address) ].isEmpty() ) {
				//CacheRow evictedBlock = listLRU[ getIndex(address) ].getLast();
				CacheRow evictedBlock = listLRU[ getIndex(address) ].removeLast();

				//Add created block to cache[][]
				currentBlock.setIndex = evictedBlock.setIndex;
				cache[ evictedBlock.setIndex ][ evictedBlock.blockIndex ] = currentBlock;

				//If  cached memory was modified and trashed, write back necessary
				if( !evictedBlock.dirtyBits.isEmpty() ) {
					//System.out.println( "* instr=w addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss: victim tag="+evictedBlock.tag+" WB");
					outputStream.println( "* instr=r addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss: victim tag="+Integer.toString(evictedBlock.tag,16)+" WB");
					//Each memory address holds 1<<b=2^b bytes of memory according to validation results.
					writeBacks += 1<<b;
				}
				//Else just trash
				else {
					//System.out.println( "* instr=r addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss: victim tag="+evictedBlock.tag+" WB");
					outputStream.println( "* instr=r addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss: victim tag="+Integer.toString(evictedBlock.tag,16));
				}
			}
			// Special case where LRU list is empty
			else if( i == numberOfSets && listLRU[ getIndex(address) ].isEmpty() ) {
				cache[0][ getIndex(address) ] = currentBlock;

				//System.out.println( "* instr=r addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss");
				outputStream.println( "* instr=r addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss");
			}
			else {
				//System.out.println( "* instr=r addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss");
				outputStream.println( "* instr=r addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss");
			}
			
			listLRU[ getIndex(address) ].addFirst( currentBlock );

			++reads;
			++readMisses;

			prefetch(address);
		}
		
		
		//DEBUG FUNCTION
		if( getIndex(address) == 32 )
			debug();
		return currentBlock;
	}

	public void updateLRU( int address ) {
		CacheRow current = getFromCache(address);
		//remove all instances of current from LRU list
		//this way you can just look at the last element to know which line to replace
		listLRU[ getIndex(address) ].remove( current );
		listLRU[ getIndex(address) ].addFirst( current );		

		
	}
	
	
	
	CacheRow write( int address ) {
		
		CacheRow currentBlock;
		currentBlock = getFromCache(address);
		//HIT
		if( currentBlock != null ) {
			//System.out.println( "* instr=w addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 hit");
			outputStream.println( "* instr=w addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 hit");
			currentBlock.dirtyBits.set( getBlock(address) );
			updateLRU(address);
			++writes;
		}
		//MISS
		else {
			//Create CacheRow data for this address
			currentBlock = new CacheRow(b, c, s);
			currentBlock.valid = 1;
			currentBlock.tag = getTag(address);
			currentBlock.blockIndex = getIndex(address);
			//System.out.println( "writemiss  tag set to: " + currentBlock.tag );

			//Insert into block where valid bit is 0
			int i;
			for(i = 0; i < numberOfSets && i >= 0; ++i) {
				if( cache[i][getIndex(address)].valid == 0 ) {
					currentBlock.setIndex = i;
					cache[i][getIndex(address)] = currentBlock;
					i = -5;
				}
			}
			//remove evictedBlock from LRU list if an invalid block wasn't found
			//this is indicated by i being equal to numberOfSets
			if( i == numberOfSets && !listLRU[ getIndex(address) ].isEmpty() ) {
				CacheRow evictedBlock = listLRU[ getIndex(address) ].getLast();
				listLRU[ getIndex(address) ].removeLast();

				//Add created block to cache[][]
				currentBlock.setIndex = evictedBlock.setIndex;
				cache[ evictedBlock.setIndex ][ evictedBlock.blockIndex ] = currentBlock;

				//If  cached memory was modified and trashed, write back necessary
				if( !evictedBlock.dirtyBits.isEmpty() ) {
					//System.out.println( "* instr=w addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss: victim tag="+evictedBlock.tag+" WB");
					outputStream.println( "* instr=w addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss: victim tag="+Integer.toString(evictedBlock.tag,16)+" WB");
					//Each address holds 1<<b=2^b bytes of memory according to validation results.
					writeBacks += 1<<b;
				}
				//Else just trash
				else {
					//System.out.println( "* instr=w addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss: victim tag="+evictedBlock.tag+" WB");
					outputStream.println( "* instr=w addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss: victim tag="+Integer.toString(evictedBlock.tag,16));
				}
			}
			// Special case where LRU list is empty
			else if( i == numberOfSets && listLRU[ getIndex(address) ].isEmpty() ) {
				cache[0][ getIndex(address) ] = currentBlock;

				//System.out.println( "* instr=w addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss");
				outputStream.println( "* instr=w addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss");

			}
			//THIS ELSE IS ONLY FOR PRINTING OUTPUT
			else {
				//System.out.println( "* instr=w addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss");
				outputStream.println( "* instr=w addr=" +Integer.toString(address,16)+" index="+currentBlock.blockIndex+" tag="+Integer.toString(currentBlock.tag, 16)+" sb=0 miss");
			}
			listLRU[ getIndex(address) ].addFirst( currentBlock );
			//Set dirty bit
			currentBlock.dirtyBits.set( getBlock(address) );
			
			++writes;
			++writeMisses;
			
			prefetch(address);
		}
		
		//DEBUG FUNCTION
		if( getIndex(address) == 32 )
			debug();
		
		return currentBlock;		
	}
	
	
	
	public void analyzeTrace( List data ) {
		
	}

	
	
	/* Used BitSet to hold dirty bits since number of dirty bits can be larger than 32 bits */
	public class CacheRow {
		int valid;
		BitSet dirtyBits;
		int tag;
		
		int setIndex;
		int blockIndex;
		public CacheRow(int b, int c, int s) {
			//int numberOfWords = (1 << b) / 4;
			// ABOVE IS WRONG
			// ASSUMES WORD SIZE OF 4 bytes
			// INSTRUCTIONS IMPLY WORD SIZE OF 1 BYTE
			//int numberOfWords = (1 << b);
			dirtyBits = new BitSet( numberOfDirtyBits );
			valid = 0;
			//dirtyBits uses a BitSet because number of dirty bits needed grows exponentially.
			//tag doesn't have this problem since it's guaranteed to be 32 bits or smaller for this architecture.
			//therefore an integer is used.
			tag = 0;
		}		
	}
	
	
	
	
	public void debug() {
/*
		System.out.println( "Index Block 32 Dump" );
		for(int i = 0; i < numberOfSets; ++i) {
			System.out.println( "set: " + cache[i][32].setIndex + " tag: " + Integer.toString(cache[i][32].tag, 16) );
		}
		
		System.out.print( "LRU Dump For Index 32: " );
		for(int i = 0; i < listLRU[32].size(); ++i) {
			CacheRow temp = listLRU[32].get(i);			
			System.out.println( "(set: " + temp.setIndex + " tag: " + Integer.toString(temp.tag,16) + ") -> " ); 
		}
*/
	}
	
	
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub

		BitSet test = new BitSet(5);
		
		test.set(3);
		System.out.println( test );
	}

	
	
}
