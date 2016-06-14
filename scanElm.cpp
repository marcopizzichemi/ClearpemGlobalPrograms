// g++ -o ../build/scanElm scanElm.cpp `root-config --cflags --glibs`

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <getopt.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <assert.h>
#include <string.h>
#include <iomanip>      // std::setw

#include <TCanvas.h>
#include <TH2F.h>
#include <TFile.h>
#include <TH1F.h>
#include <TF1.h>
// static const int nCols = 6;

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
  if(argc < 2)
  {
    std::cout << "USAGE:\t\t scanElm file.elm2 [eMin] [eMax] [dMax] [maxHits] [RandMin] [RandMax] " << std::endl;
    std::cout << "file.elm2 \t input .elm2 file" << std::endl;
    std::cout << "eMin \t\t OPTIONAL: lower energy bound in Kev. Default = 400 Kev" << std::endl;
    std::cout << "eMax \t\t OPTIONAL: upper energy bound in Kev. Default = 650 Kev" << std::endl;
    std::cout << "dMax \t\t OPTIONAL: width of coincidence window in sec. Default = 4e-09 s" << std::endl;
    std::cout << "maxHits \t OPTIONAL: maximum number of hits allowed per event. Default = 4" << std::endl;
    std::cout << "RandMin \t OPTIONAL: lower bound of the delayed concidence windows in sec. Default = 20e-9 s" << std::endl;
    std::cout << "RandMax \t OPTIONAL: upper bound of the delayed concidence windows in sec. Default = 90e-9 s" << std::endl;
    
    std::cout << std::endl;
    std::cout << "WARNING: the program assumes a frames.txt file in this folder, with time frames in sec" << std::endl;
    return 1;
  }
  
  
  float eMin = 400;
  float eMax = 650;
  float dMax = 4E-9;
  float tRandMin = 20e-9;
  float tRandMax = 90e-9;
  int   maxHits = 4;
  
  //check if a frames.txt file is provided in this directory
  std::ifstream fFrames;
  fFrames.open ("frames.txt", std::ifstream::in);
  if (!fFrames.is_open()) 
  {
    std::cout << "ERROR: no frames.txt file found! " << std::endl;
    std::cout << "You need to provide a frames.txt file in this folder" << std::endl;
    return 1;
  }
  //read the frames.txt file if provided
  std::vector<double> frameStart,frameStop; //start and stop pf each frame
  while(!fFrames.eof()) 
  {
    double x,y;
    fFrames >> x >> y;
    if(!fFrames.eof())
    {
      frameStart.push_back(x);
      frameStop.push_back(y);
    }
  }
  fFrames.close();
  //   //debug
  //   std::cout << std::endl;
  //   for(int i = 0; i < frame.size() ; i++)
  //   {
  //     std::cout << frame[i] << "\t";
  //   }
  //   std::cout << std::endl;
  
  std::vector<long int> prompts;
  
  std::vector<double> randoms;
  std::vector<double> dtfwhm;
  std::vector<double> time;
  //create total events
  
  
  
  double t0 = -1;
  double TimeFromZero = 0;	//delta time of each event from first event
  long long nRandoms = 0;
  long long nPrompts = 0;
  long long nBad = 0;
  long long nEvents = 0;
  long long nTrues = 0;
  
  
  //read the elm2 file
  
  //   for(int i = 1; i < argc; i++) 
  //   {
  //FILE * fIn = fopen(argv[i], "r");
  
  FILE * fIn = NULL;
  if(strcmp(argv[1], "-") == 0)
    fIn = stdin;
  else
    fIn = fopen(argv[1], "rb");
  if (fIn == NULL) {
    fprintf(stderr, "File %s does not exist\n", argv[1]);
    return 1;
  }
  printf("Reading %s\n", argv[1]);
  
  //optional inputs
  if(argc > 2)
  {
    eMin = atof(argv[2]);
    if(argc > 3)
    {
      eMax = atof(argv[3]);
      if(argc > 4)
      {
	dMax = atof(argv[4]);
	if(argc > 5)
	{
	  maxHits = atoi(argv[5]);
	  if(argc > 6)
	  {
	    tRandMin = atof(argv[6]);
	    if(argc > 7)
	    {
	      tRandMax = atof(argv[7]);
	    }
	  }
	}
      }
    }
  }
  
  //feedback to user
  std::cout << "--------------------------------------" << std::endl;
  std::cout << "|             PARAMETERS              |" << std::endl;
  std::cout << "--------------------------------------" << std::endl;
  std::cout << "Energy window [keV] = \t\t\t" << eMin << " | " << eMax << std::endl;
  std::cout << "Concidence window [s] = \t\t" << dMax << std::endl;
  std::cout << "Delayed concidence window [s] = \t" << tRandMin << " | " << tRandMax << std::endl;
  std::cout << "Max Hits = \t\t\t\t" << maxHits << std::endl;
  std::cout << "--------------------------------------" << std::endl;
  //   std::cout << eMin << " " << eMax << " " << dMax <<  " " << maxHits << " " << tRandMin << " " << tRandMax  << std::endl;
  
  EventFormat fe;
  int frameCounter = 0;
  double timeStop = frameStop[frameStop.size()-1];
  bool passLow = false;
  bool passHigh = false;
  
  TFile *fRoot = new TFile("deltaTime.root", "RECREATE");
  
  //histograms
  TH1F** deltaTime;
  deltaTime = new TH1F*[frameStop.size()];
  
  
  while(fread((void*)&fe, sizeof(fe), 1, fIn) == 1) 
  {
    nEvents++;
    if(frameCounter == frameStart.size())
      break;
    //looks for t0
    if(t0 == -1) t0 = fe.ts; 		//t0 is effectively calculated only at the first cycle
    TimeFromZero = fe.ts - t0;        //this will hold the time from 0
    
    if(TimeFromZero >= frameStart.at(frameCounter)) //check time
    {
      
      if(!passLow) //if low th of this slide was not passed, mark it passed and set vars to 0
      {
	passLow = true;
	nPrompts = 0;
	nRandoms = 0;
	std::stringstream sname;
	sname << "Time Histograms Slice " << frameCounter;
	deltaTime[frameCounter] = new TH1F(sname.str().c_str(),sname.str().c_str(),1000,-90e-9,90e-9);
      }
      if(TimeFromZero <= frameStop.at(frameCounter) && passLow) //accumulate data 
      {
// 	std::cout << TimeFromZero<< " " << nPrompts << " " << nRandoms  << std::endl;
	float u = fe.x1 - fe.x2;
	float v = fe.y1 - fe.y2;
	float w = fe.z1 - fe.z2;
	if(fabs(w) < fe.d/2.0) {
	  nBad++;
	  continue;
	}
	
	
	// dt fix
	if (fe.z1 > 0)
	  fe.dt *= -1;
	
	//always fill the delta timehistogram, for events in this time slice
	deltaTime[frameCounter]->Fill(fe.dt);
	
	//conditions
	bool eOK = (fe.e1 >= eMin) && (fe.e1 <= eMax) && (fe.e2 >= eMin) && (fe.e2 <= eMax);
	bool tOK = fabs(fe.dt) < dMax;
	bool p = fe.random == 0;
	bool hitOK = (fe.n1 < maxHits) && (fe.n2 < maxHits);
	bool randomTimeWindow = (fabs(fe.dt) > tRandMin) && (fabs(fe.dt) < tRandMax);
	
	if(eOK && tOK && p && hitOK) 
	{  //if event is in the energy window and delta t < 4ns, then it's a true (prompt)
	  
	  nPrompts++;
	  // 			  (p ? h : hr)->Fill(mx, my);
	}
	
	if(eOK && randomTimeWindow && p && hitOK) 
	{  //if the event is in the energy window and in the delayed delta time, then it's a random
	  nRandoms++;
	  // 			  (p ? h : hr)->Fill(mx, my);
	}
	
      }
      else // the slice finished
      {
	
// 	std::cout << TimeFromZero << " " << frameCounter << " " << nPrompts << " " << nRandoms   << std::endl;
	
	//store data
	time.push_back(frameStart.at(frameCounter)); //add the start time
	prompts.push_back(nPrompts);
	randoms.push_back(nRandoms * (dMax / (tRandMax - tRandMin)));  //randoms are rescaled (delayed window)
	passLow = false;
	//change frame
	frameCounter++;
	
      }
    }
    
    if( (nEvents % 10000) == 0 )
    {
      std::cout << "\r";
      std::cout << "Time " << std::setw(14) << TimeFromZero << " sec" ;      
    }
  }
  fclose(fIn);
  std::cout << std::endl;
  
  std::cout << "Total number of events analized = " << nEvents << std::endl;
  std::cout << "Stop time                       = " << TimeFromZero << std::endl;
  
  if(TimeFromZero < timeStop)
    std::cout << "WARNING: stop time larger than dataset time length" << std::endl;
  
  fRoot->cd();
  for(int i = 0 ; i < frameStop.size() ; i++)
  {
    TF1 *gauss = new TF1("gauss","gaus");
    deltaTime[i]->Fit("gauss","Q","",-15e-9,15e-9);
    dtfwhm.push_back(gauss->GetParameter(2)*2.355);
    deltaTime[i]->Write();
    delete gauss;
  }
  fRoot->Close();
  
  std::ofstream ofs;
  ofs.open ("results.txt", std::ofstream::out);
  ofs << "T0\tPrompts\tTrues\t\tRandoms\tdtFWHM\t\tRandomPerc" << std::endl;
  for(int i = 0; i < time.size(); i++)
  {
    ofs << time[i] <<  "\t" << prompts[i] << "\t" << prompts[i]-randoms[i] << "\t" << randoms[i]  << "\t" << dtfwhm[i] << "\t" << 100.0*randoms[i]/prompts[i] << std::endl;
  }
  ofs.close();
  std::cout << "Results saved in file results.txt" << std::endl;
  
  
  
  return 0;
  
}
