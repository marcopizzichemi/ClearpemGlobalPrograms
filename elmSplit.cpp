#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <sstream>
#include <fstream>
//#include <string>
#include <string.h>


// ****** Compilation command *************************************************
// g++ -o elmSplit elmSplit.cpp 
// ****************************************************************************


//declares the struct of events
struct EventFormat {
  double ts;				// time of the event, in seconds. if i'm not mistaken is the absolute machine time in seconds 
  u_int8_t random;                      // set probably at the level of acq, says if the event is random (when it's value is 1)
  float d;                              // distance between the heads, fixed to the head distance value... 
  float yozRot;		                // rotation of the heads 
  float x1;                             // x coordinate of the event, for detector 1		
  float y1;                             // y coordinate of the event, for detector 1
  float z1;                             // z coordinate of the event, for detector 1
  float e1;                             // energy deposited in detector 1
  u_int8_t n1;                          // some check done probably at the level of acq, it's 1 if the event is ok, otherwise is not ok
  float x2;                             // x coordinate of the event, for detector 2
  float y2;                             // y coordinate of the event, for detector 2
  float z2;                             // z coordinate of the event, for detector 2
  float e2;                             // energy deposited in detector 2
  u_int8_t n2;                          // some check done probably at the level of acq, it's 1 if the event is ok, otherwise is not ok
  float dt;                             // delta time between event in detector 1 and event in detector 2 
} __attribute__((__packed__));          


int main(int argc, char * argv[]) 
{
  if(argc < 4)
  {
    std::cout << "USAGE:\t\t elmSplit <file.elm2> <t1> <t2>" << std::endl;
    std::cout << "file.elm2 \t input .elm2 file" << std::endl;
    std::cout << "t1 \t Time where the cut begin, from the start, in seconds" << std::endl;
    std::cout << "t2 \t Time where the cut ends, from the start, in seconds" << std::endl;    
    std::cout << std::endl;
    std::cout << "WARNING: output will be stored in a file called cut.elm2" << std::endl;
    return 1;
  }
  else
  {
    long long nEvents = 0;      //counter of the events in the input file
    double TimeFromZero = 0;	//delta time of each event from first event
     
    long long nEventsExported = 0; //number of events exported    
    //inizialize time variables
    double t0 = -1;
    double tMax = -INFINITY;
    double tMin = INFINITY;
    
    
    float timeStart = atof(argv[2]);
    float timeStop =  atof(argv[3]);
    
    //open input file
    //FILE * fIn = fopen(argv[1], "r");	
    
    FILE * fIn = NULL;
    if(strcmp(argv[1], "-") == 0)
      fIn = stdin;
    else
      fIn = fopen(argv[1], "rb");

    //check if file is there
    if (fIn == NULL) 
    {
      fprintf(stderr, "File %s does not exist\n", argv[1]);
      return 1;
    }
    printf("Reading %s\n", argv[1]);
       
    //create the string for the output file
    std::stringstream OutStringStream;
    //std::string OutString;
    OutStringStream << "cut_T" << timeStart << "_T" << timeStop << ".elm2";
    //OutString = OutStringStream.str();
    
    //open output binary file
    FILE *outputFile=fopen(OutStringStream.str().c_str(), "wb");
    
    //create the event struct fe
    EventFormat fe;
    
    //readout loop
    while(/*nEvents < 1000+1 &&*/ fread((void*)&fe, sizeof(fe) , 1, fIn) == 1) 
    {
      
      nEvents++; //counts the events (elements of the binary input file)
      //outputs the events counted
      if(nEvents % 10000 == 0) 
      { 
	printf("%lld events\r", nEvents); 
	fflush(stdout);
      }
      
      //looks for t0, tMin e tMax, updating at each loop. 
      if(t0 == -1) t0 = fe.ts; 		//t0 is effectively calculated only at the first cycle
      if(fe.ts < tMin) tMin = fe.ts;
      if(fe.ts > tMax) tMax = fe.ts;
      
      TimeFromZero = fe.ts - t0;    //this will hold the time from 0
      
      if(TimeFromZero > timeStop)
	break;
      
      //write the input structure in the output file only if it is in the time interval selected by the user
      if(TimeFromZero > timeStart && TimeFromZero < timeStop) 
      {
        nEventsExported++;
	fwrite((void*)&fe,sizeof(fe) , 1, outputFile);
      }
    }
    
    if(TimeFromZero < timeStop)
      std::cout << "WARNING: stop time larger than dataset time length" << std::endl;
    //close files
    fclose(fIn);
    fclose(outputFile);
    
    //print some info
    printf("%lld events exported in output file\n", nEventsExported);
    printf("Total time in exported file: %lf sec (%lf to %lf)\n", timeStop - timeStart, timeStart, timeStop);
    
    
    if(TimeFromZero < timeStop)
    {
      std::cout << std::endl;
      std::cout << "**************************************************************" << std::endl;
      std::cout << "WARNING: chosen stop time is larger than dataset time length! " << std::endl;
      std::cout << "The cut file will be shorter than expected                    " << std::endl;
      std::cout << "Chosen stop time" << "\t" << "=" << "\t" << timeStop << std::endl;
      std::cout << "Dataset time length" << "\t" << "=" << "\t" << TimeFromZero << std::endl;
      std::cout << "*******************************************************" << std::endl;
      std::cout << std::endl;
    }
    
    return 0;
  }
}
