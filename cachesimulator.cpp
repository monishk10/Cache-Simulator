/*
 * Cache Simulator
 * Author: Monish Kapadia
 */
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <stdlib.h>
#include <cmath>
#include <bitset>

using namespace std;
//access state:
#define NA 0 // no action
#define RH 1 // read hit
#define RM 2 // read miss
#define WH 3 // Write hit
#define WM 4 // write miss

struct config{
	int L1blocksize;
	int L1setsize;
	int L1size;
	int L2blocksize;
	int L2setsize;
	int L2size;
};

// Extracting particular bits from the passed address
bitset<32> bit_extract(bitset<32> addr, int start, int end){
	bitset<32> mask = (pow(2, (end - start)) - 1);
	return ((addr >> start) & mask);
}

// Creating class cache for basic setup
class Cache{
public:
	unsigned long cacheSize, noOfWays, blockSize, noOfBlocks;
	unsigned long offsetBits, indexBits, tagBits;
	unsigned long offset, index, tag;
	bool hit;
	int checkIndex;
	vector < vector<unsigned long> > tagArray;
	vector < vector<bool> > validArray;

	void setupCache(unsigned long cacheSize, unsigned long noOfWays, unsigned long blockSize){
		this->cacheSize = cacheSize * 1024;
		this->blockSize = blockSize;
		this->noOfWays = (noOfWays == 0)?(this->cacheSize/this->blockSize):(noOfWays);
		this->noOfBlocks = (this->cacheSize)/(this->blockSize * this->noOfWays);
		this->offsetBits = log2(this->blockSize);
		this->indexBits = log2(this->noOfBlocks);
		this->tagBits = 32 - this->indexBits - this->offsetBits;
		this->tagArray = vector< vector<unsigned long> >(this->noOfBlocks, vector<unsigned long>(this->noOfWays));
		this->validArray = vector< vector<bool> >(this->noOfBlocks, vector<bool>(this->noOfWays));
	}

	void extractValues(bitset<32> addr){
		offset = bit_extract(addr, 0, offsetBits).to_ulong();
		index = bit_extract(addr, offsetBits, (offsetBits + indexBits)).to_ulong();
		tag = bit_extract(addr, (offsetBits + indexBits), (offsetBits + indexBits + tagBits)).to_ulong();
	}

	// check if the tag exists in the cache
	void check(){
		checkIndex=-1;
		hit = false;
		for(unsigned long i=0; i < noOfWays; i++){
			if(tag == tagArray[index][i] && validArray[index][i]){
				checkIndex = i;
				hit = true;
				break;
			}
		}
	}

	//shift the LRU cache to the last and evict if necessary
	void allocateUpdate(){
		if(hit){
			for(int i=checkIndex;i>0;i--){
				tagArray[index][i] = tagArray[index][i-1];
				validArray[index][i] = validArray[index][i-1];
			}
		} else {
			if(tag != tagArray[index][0]){
				for(int i=noOfWays-1; i>0;i--){
					tagArray[index][i] = tagArray[index][i-1];
					validArray[index][i] = validArray[index][i-1];
				}
			}
		}
		tagArray[index][0] = tag;
		validArray[index][0] = 1;
	}
};


int main(int argc, char* argv[]){
   	config cacheconfig;
    ifstream cache_params;
    string dummyLine;
    cache_params.open(argv[1]);
    while(!cache_params.eof())  // read config file
    {
		cache_params>>dummyLine;
		cache_params>>cacheconfig.L1blocksize;
		cache_params>>cacheconfig.L1setsize;
		cache_params>>cacheconfig.L1size;
		cache_params>>dummyLine;
		cache_params>>cacheconfig.L2blocksize;
		cache_params>>cacheconfig.L2setsize;
		cache_params>>cacheconfig.L2size;
    }

    Cache L1, L2;
    L1.setupCache(cacheconfig.L1size, cacheconfig.L1setsize, cacheconfig.L1blocksize);
    L2.setupCache(cacheconfig.L2size, cacheconfig.L2setsize, cacheconfig.L2blocksize);

    int L1AcceState =0; // L1 access state variable, can be one of NA, RH, RM, WH, WM;
    int L2AcceState =0; // L2 access state variable, can be one of NA, RH, RM, WH, WM;

    ifstream traces;
    ofstream tracesout;
    string outname;
    outname = string(argv[2]) + ".out";

    traces.open(argv[2]);
    tracesout.open(outname.c_str());


    string line;
    string accesstype;  // the Read/Write access type from the memory trace;
    string xaddr;       // the address from the memory trace store in hex;
    unsigned int addr;  // the address from the memory trace store in unsigned int;
    bitset<32> accessaddr; // the address from the memory trace store in the bitset;


    if (traces.is_open()&&tracesout.is_open()){
    	while (getline (traces,line)){   // read mem access file and access Cache
    		istringstream iss(line);
            if (!(iss >> accesstype >> xaddr)) {break;}
            stringstream saddr(xaddr);
            saddr >> std::hex >> addr;
            accessaddr = bitset<32> (addr);

            L1.extractValues(accessaddr);
            L2.extractValues(accessaddr);
            L1.check();
            L2.check();

            if (accesstype.compare("R")==0){
            	if(L1.hit){
					L1AcceState = RH;
					L2AcceState = NA;
					L1.allocateUpdate();
				} else if(L2.hit && !L1.hit) {
					L1AcceState = RM;
					L2AcceState = RH;
					L1.allocateUpdate();
					L2.allocateUpdate();
				} else {
					L1AcceState = RM;
					L2AcceState = RM;
					L1.allocateUpdate();
					L2.allocateUpdate();
				}
            } else {
            	if(L1.hit){
            		L1AcceState = WH;
            		L2AcceState = WH;
            		L1.allocateUpdate();
            		L2.allocateUpdate();
            	} else if(L2.hit) {
            		L1AcceState = WM;
            		L2AcceState = WH;
            		L2.allocateUpdate();
            	} else {
            		L1AcceState = WM;
            		L2AcceState = WM;
            	}
            }
            tracesout<< L1AcceState << " " << L2AcceState << endl;  // Output hit/miss results for L1 and L2 to the output file;
        }
        traces.close();
        tracesout.close();

    }
    else cout<< "Unable to open trace or traceout file ";

    return 0;
}
