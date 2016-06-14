// g++ -o ../build/elmInfo elmInfo.cpp

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <getopt.h>
// #include <boost/lexical_cast.hpp>
#include <iostream>
#include <set>
#include <assert.h>
#include <string.h>
#include <iomanip>      // std::setw

// #include <TCanvas.h>
// #include <TH2F.h>
// #include <TFile.h>
// #include <TH1F.h>

static float eMin = 400;
static float eMax = 650;
static float dMax = 4E-9;
static int   maxHits = 4;

static float tRandMin = 20e-9;
static float tRandMax = 90e-9;

static const int nCols = 6;

struct EventFormat {
		double ts;
		u_int8_t random;
		float d;
		float yozRot;		
		float x1;
		float y1;
		float z1;
		float e1;
	        u_int8_t n1;
		float x2;
		float y2;
		float z2;
		float e2;
	        u_int8_t n2;
		float dt;
	} __attribute__((__packed__));


int main(int argc, char * argv[]) 
{
  
  for(int i = 1; i < argc; i++) 
  {
    //FILE * fIn = fopen(argv[i], "r");
    FILE * fIn = NULL;
    if(strcmp(argv[i], "-") == 0)
      fIn = stdin;
    else
      fIn = fopen(argv[i], "rb");
    if (fIn == NULL) {
      fprintf(stderr, "File %s does not exist\n", argv[i]);
      return 1;
    }
    printf("Reading %s\n", argv[i]);
    
    double totalTime = 0;
    double t0 = -1;
    long long int nEvents = 0;
    
    EventFormat fe;
    while(fread((void*)&fe, sizeof(fe), 1, fIn) == 1) 
    {
      nEvents++;
      if(t0 == -1)
	t0 = fe.ts;
      totalTime = fe.ts - t0;
      
      if( (nEvents % 10000) == 0 )
      {
        std::cout << "\r";
        std::cout << "Events " << std::setw(10) << nEvents;
      }
      
    }
    
    std::cout << std::endl;
    std::cout << "------------------------" << std::endl;
    std::cout << "File \t" << argv[i] << std::endl;
    std::cout << "nEvents =\t" << nEvents << std::endl;
    std::cout << "totalTime =\t" << totalTime << std::endl;
    std::cout << "------------------------" << std::endl;
    std::cout << std::endl;
  }
  
  
  return 0;
}