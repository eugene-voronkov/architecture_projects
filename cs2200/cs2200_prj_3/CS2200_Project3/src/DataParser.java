import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

import javax.swing.JFileChooser;


public class DataParser {

	
	Scanner buffer;
	String file;
	List<String[]> data = new ArrayList<String[]>();

	
	private void parseData() {
		
		int listIndex = 0;
		while(buffer.hasNextLine()) {
			data.add(new String[2]);

			Scanner parser = new Scanner( buffer.nextLine() );
			
			data.get(listIndex)[0] = parser.next();
			data.get(listIndex)[1] = parser.next();
			
			
			++listIndex;
		}
		
	}
	
	private void getFileName() {
		 JFileChooser chooser = new JFileChooser();
		 int res = chooser.showOpenDialog(null);
		 if (res == JFileChooser.APPROVE_OPTION) {
			 System.out.println("Opening File: " + chooser.getSelectedFile().getName());
			 try {
				file = chooser.getSelectedFile().getCanonicalPath();
			} catch (IOException e) {
				System.err.println("An exception occurred while initializing our book list");
				e.printStackTrace();
			}
		 }
		 
	}

	public void openFile() {
		getFileName();
		try {
			buffer = new Scanner( new File(file) );
		} catch( Exception e ) {
			System.out.println( e );
		}
			
		parseData();

		buffer.close();
		try {
		buffer = new Scanner( new File(file) );
		} catch( Exception e ) {
			System.out.println( e );
		}
	}
	
	
	public void openFile2( String path ) {
		try {
			buffer = new Scanner( new File(path) );
		} catch( Exception e ) {
			System.out.println( e );
		}
			
		parseData();

		buffer.close();
		try {
		buffer = new Scanner( new File(path) );
		} catch( Exception e ) {
			System.out.println( e );
		}
	}
	/**
	 * @param args
	 */

	//b c s p
	//Size Contraint In Bits
	float[] optimizeForSize(int sizeConstraint) {
		//sim cacheSim = new sim();
		//cacheSim.setupCache(b, c, s, p);
		float min[] = new float[5];
		min[4] = Integer.MAX_VALUE;
		
		int terminateSignal = 0;
		for (int c = 0; (1<<c + 2) <= sizeConstraint; ++c) {
			terminateSignal = 0;
			for (int b = 0; b <= c && terminateSignal != -1; ++b) {
				for (int s = 0; s <= (c - b); ++s) {
					float previousAAT = Integer.MAX_VALUE;
					float AAT = Integer.MAX_VALUE - 1;
					for (int p = 0; p < (1<<(c-b)) && (previousAAT - AAT > 0) && terminateSignal != -1; ++p) {
						sim cacheSim = new sim();
						cacheSim.setupCache(b, c, s, p);
						
						//DEBUG STATEMENT BELOW
						//System.out.println( "current min b="+min[0]+" c="+min[1]+" s="+min[2]+"p="+min[2]);
						//System.out.println( "current compareto b="+b+"c="+c+" s="+s+"p="+p);
						
						if (sizeConstraint == cacheSim.blockSize){
							terminateSignal = -1;
						}
						
						else {
							//Run Simulation here
							for(int i = 0; i < data.size(); ++i) {
								if( data.get(i)[0].equals( "r" ) ) {
									cacheSim.read( Integer.valueOf(data.get(i)[1], 16).intValue() );
								}
								else {
									cacheSim.write( Integer.valueOf(data.get(i)[1], 16).intValue() );
								}
							}
							System.out.println( "current min:" + min[4] +"  previousAAT: "+previousAAT+" AAT:"+AAT );
							if( min[4] > cacheSim.calculateAAT() ) {
								System.out.println( "old min b="+min[0]+" c="+min[1]+" s="+min[2]+"p="+min[3]+" aat=" +min[4]);
								min[0] = b;
								min[1] = c;
								min[2] = s;
								min[3] = p;
								min[4] = cacheSim.calculateAAT();
								System.out.println( "new min b="+min[0]+" c="+min[1]+" s="+min[2]+"p="+min[3]+" aat=" +min[4]);
								AAT = previousAAT;
								previousAAT = min[4];
							}
						}
						
					}
				}
			}
		}
		return min;
	}	

/*		
		for(int b = 0; terminateSignal != -1; ++b) {
			//DEBUG STATEMENT BELOW
			System.out.println( "current min b="+min[0]+" c="+min[1]+" s="+min[2]+"p="+min[2]);
			for(int c = b; terminateSignal != -1; ++c) {
				int cTemp = c;
				System.out.println( "current c value: " + c);
				for(int p = cTemp; terminateSignal != -1; ++p) {
					for(int s = p; (1<<s)<p; ++s) {

						sim cacheSim = new sim();
						cacheSim.setupCache(b, c, s, p);
						
						//DEBUG STATEMENT BELOW
						System.out.println( "current min b="+min[0]+" c="+min[1]+" s="+min[2]+"p="+min[2]);
						System.out.println( "current compareto b="+b+"c="+c+" s="+s+"p="+p);
						
						if (sizeConstraint == cacheSim.blockSize){
							terminateSignal = -1;
						}
						else {
							//Run Simulation here
							for(int i = 0; i < data.size(); ++i) {
								if( data.get(i)[0].equals( "r" ) ) {
									cacheSim.read( Integer.valueOf(data.get(i)[1], 16).intValue() );
								}
								else {
									cacheSim.write( Integer.valueOf(data.get(i)[1], 16).intValue() );
								}
							}							
							if( min[4] < cacheSim.calculateAAT() ) {
								System.out.println( "old min b="+min[0]+" c="+min[1]+" s="+min[2]+"p="+min[2]);
								min[0] = b;
								min[1] = c;
								min[2] = s;
								min[3] = p;
								min[4] = cacheSim.calculateAAT();
								System.out.println( "new min b="+min[0]+" c="+min[1]+" s="+min[2]+"p="+min[2]);
							}
						}
						
					}
				}
*/
	/*			
				for(int s = cTemp; terminateSignal != -1; ++s) {
					System.out.println( "current c value2: " + c);
					System.out.println( "current s value: " + s);
					for(int p = s; p<c; ++p) {

						sim cacheSim = new sim();
						cacheSim.setupCache(b, c, s, p);
						
						//DEBUG STATEMENT BELOW
						System.out.println( "current min b="+min[0]+" c="+min[1]+" s="+min[2]+"p="+min[2]);
						System.out.println( "current compareto b="+b+"c="+c+" s="+s+"p="+p);
						
						if (sizeConstraint == cacheSim.blockSize){
							terminateSignal = -1;
						}
						else {
							//Run Simulation here
							for(int i = 0; i < data.size(); ++i) {
								if( data.get(i)[0].equals( "r" ) ) {
									cacheSim.read( Integer.valueOf(data.get(i)[1], 16).intValue() );
								}
								else {
									cacheSim.write( Integer.valueOf(data.get(i)[1], 16).intValue() );
								}
							}							
							if( min[4] < cacheSim.calculateAAT() ) {
								System.out.println( "old min b="+min[0]+" c="+min[1]+" s="+min[2]+"p="+min[2]);
								min[0] = b;
								min[1] = c;
								min[2] = s;
								min[3] = p;
								min[4] = cacheSim.calculateAAT();
								System.out.println( "new min b="+min[0]+" c="+min[1]+" s="+min[2]+"p="+min[2]);
							}
						}
					}

				}

			}
		}
		return min;
	}
*/	
	
	public static void main(String[] args) {
		int cHolder = -1;
		int bHolder = -1;
		int sHolder = -1;
		int pHolder = -1;
		String inputFile = null;
		
		inputFile = args[0];
		
		for(int c = 1; c < args.length; c += 2) {
			if( args[c].equals( "-c" ) ) {
				cHolder = Integer.valueOf(args[c+1]);
			}
		}

		for(int b = 1; b < args.length; b += 2) {
			if (args[b].equals( "-b" )) {
				bHolder = Integer.valueOf(args[b+1]);
			}
		}
		for(int s = 1; s < args.length; s += 2) {
			if (args[s].equals( "-s" )) {
				sHolder = Integer.valueOf(args[s+1]);
			}
		}
		for(int p = 1; p < args.length; p += 2) {
			if (args[p].equals( "-p" )) {
				pHolder = Integer.valueOf(args[p+1]);
			}
		}

		if( cHolder >= 0 && bHolder >= 0 && sHolder >= 0 && pHolder >= 0 && inputFile != null) {
			DataParser testParser = new DataParser();		
			testParser.openFile2(args[0]);
			
			sim cacheSim = new sim();
			cacheSim.setupCache(bHolder, cHolder, sHolder, pHolder);
			
			for(int i = 0; i < testParser.data.size(); ++i) {							
				if( testParser.data.get(i)[0].equals( "r" ) ) {
					cacheSim.read( Integer.valueOf(testParser.data.get(i)[1], 16).intValue() );
				}
				else {
					cacheSim.write( Integer.valueOf(testParser.data.get(i)[1], 16).intValue() );
				}
			}			

			System.out.println( "Cache Accesses: " + (cacheSim.reads + cacheSim.writes));
			System.out.println( "reads: " + cacheSim.reads );
			System.out.println( "read misses: " + cacheSim.readMisses );
			System.out.println( "writes: " + cacheSim.writes );
			System.out.println( "write misses: " + cacheSim.writeMisses );
			System.out.println( "Write Backs(bytes): " + cacheSim.writeBacks );
			//Mem Traffic
			System.out.println( "Mem Traffic: " + (cacheSim.readMisses + cacheSim.writeMisses)*8 );
			System.out.println( "Total Misses: " + (cacheSim.writeMisses + cacheSim.readMisses) );
			System.out.println( "Miss Ratio: " + (float)cacheSim.readMisses / (float)cacheSim.reads );
			System.out.println( "AAT: " + cacheSim.calculateAAT() );
			System.out.println( "Tag Size: " + cacheSim.sizeTagBits );
			System.out.println( "Data Bits / Block: " + (1<<cacheSim.b) * 8);
			System.out.println( "Number Of Blocks: " + (1<<(cacheSim.c - cacheSim.b)));

			//total cache size = block size * number of blocks
			System.out.println( "Total Cache Size: " + cacheSim.blockSize * (1<<(cacheSim.c - cacheSim.b)) );
			
			
			//Cache Dump
			/*
			for( int set = 0; set < cacheSim.numberOfSets; ++set ) {
				System.out.println( "Set " + set );
				for( int index = 0; index < cacheSim.blocksPerSet; ++index) {
					System.out.print( Integer.toString(cacheSim.cache[set][index].tag, 16) + "-->" );
				}
			}
			*/
		}

		
	}
	
	
	
}
