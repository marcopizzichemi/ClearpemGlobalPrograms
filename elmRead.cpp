// compile with
// g++ -o elmRead elmRead.cpp

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <getopt.h>
#include <iostream>
#include <set>
#include <assert.h>
#include <string.h>
#include <sstream>
#include <fstream>
#include <vector>



struct EventFormat {
  double ts;                     // time of the event, in seconds. if i'm not mistaken is the absolute machine time in seconds 
  u_int8_t random;               // 
  float d;                       // distance between the heads, fixed to the head distance value... 
  float yozRot;                  // rotation of the heads 
  float x1;                      // x coordinate of the event, for detector 1		
  float y1;                      // y coordinate of the event, for detector 1
  float z1;                      // z coordinate of the event, for detector 1
  float e1;                      // energy deposited in detector 1
  u_int8_t n1;                   // 
  float x2;                      // x coordinate of the event, for detector 2
  float y2;                      // y coordinate of the event, for detector 2
  float z2;                      // z coordinate of the event, for detector 2
  float e2;                      // energy deposited in detector 2
  u_int8_t n2;                   // 
  float dt;                      // delta time between event in detector 1 and event in detector 2 
} __attribute__((__packed__));


int main(int argc, char * argv[]) 
{
  long long nEvents = 0;
  
  for(int i = 1; i < argc; i++)  // read input files
  {
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
    EventFormat fe;
    //read current input file
    while(fread((void*)&fe, sizeof(fe), 1, fIn) == 1 /*&& nEvents < 50*/) 
    {      
      if(1 /*&& nEvents > 1 && nEvents < 5*/ )
      {
	std::cout << fe.ts     << " ";
	std::cout << (int) fe.random << " ";
	std::cout << fe.d      << " ";
	std::cout << fe.yozRot << " ";
	std::cout << fe.x1     << " ";
	std::cout << fe.y1     << " ";
	std::cout << fe.z1     << " ";
	std::cout << fe.e1     << " ";
	std::cout << (int) fe.n1     << " ";
	std::cout << fe.x2     << " ";
	std::cout << fe.y2     << " ";
	std::cout << fe.z2     << " ";
	std::cout << fe.e2     << " ";
	std::cout << (int) fe.n2     << " ";
	std::cout << fe.dt           ;
	std::cout << std::endl;
      }
      nEvents++;
    }
    
    
  }
  
  
  return 0;
}