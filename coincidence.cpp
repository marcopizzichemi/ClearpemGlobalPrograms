// g++ -Wl,--no-as-needed -o coincidence coincidence.cpp `root-config --cflags --glibs`

// procedure  
// once the .root files are produced by gate, this should be run on each one of them to produce a elm2 file with all the possible coincidences
// all the root files will be in a folder 


#include "TROOT.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TLegend.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TString.h"
#include "TApplication.h"
#include "TLegend.h"
#include "TTree.h"
#include "TFile.h"
#include "TF2.h"
#include "TSpectrum.h"
#include "TSpectrum2.h"
#include "TTreeFormula.h"
#include "TMath.h"
#include "TChain.h"
#include "TCut.h"
#include "TLine.h"
#include "TError.h"
#include "TEllipse.h"
#include "TGraph2D.h"

#include <iostream>
#include <bitset>       // std::bitset
#include <fstream>
#include <string>
#include <sstream>
#include "TChain.h"
#include <vector>
#include <algorithm>

#include <stdlib.h> 
#include <stdio.h> 
#include <unistd.h>
#include <cmath> 

//declares the struct of events
struct EventFormat 
{
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

int main(int argc, char** argv)
//int coincidence(TString filename)
{
  std::cout << "Reading file " << argv[1] << std::endl;
  
  //TFile *file = new TFile(argv[1], "READ");
  std::string filename = argv[1];
  
  std::string outFileName;
  outFileName = filename.substr(0,filename.length() -5 );
  outFileName += ".elm2";
  
  TFile *file = new TFile(filename.c_str(), "UPDATE");
  TTree *tree = (TTree *)file->Get("Singles");
  
  Float_t distance = 209.059998; //real heads distance, in mm
  Float_t yozRot = 0.0; //head rotation, in radiands
  Float_t absTime = 0.0; //t_start of this take, from the beginning of the "exam"
  
  if(argc > 2)
    distance = atof(argv[2]);
  if(argc > 3)
    yozRot = atof(argv[3]);
  if(argc > 4)
    absTime = atof(argv[4]);
  
  std::cout << "Heads distance [mm] = " << distance << std::endl;
  std::cout << "Rotation [rad] = " << yozRot << std::endl;
  std::cout << "Absolute initial time for this take [s] = " << absTime << std::endl;
  
  u_int8_t randomSet = 0;
  u_int8_t n1Set = 1;
  u_int8_t n2Set = 1;
  
  int entry;
  double time;
  Float_t xPos,yPos,zPos;
  Float_t energy;
  tree->SetBranchAddress("eventID",&entry);
  tree->SetBranchAddress("time",&time);
  
  tree->SetBranchAddress("globalPosX",&xPos);
  tree->SetBranchAddress("globalPosY",&yPos);
  tree->SetBranchAddress("globalPosZ",&zPos);
  tree->SetBranchAddress("energy",&energy);
  
  int ctrue = 0;
  int crandom = 0;
  
  long int nEvents = tree->GetEntries();
  std::cout << std::endl;
  std::cout << "nEvents = " << nEvents << std::endl;
  const int depth = 100;
  double dt = 4e-9;
  double timeWindow = 90e-9;
  
  //output binary file  
  
  ofstream output_file(outFileName.c_str(), std::ios::binary);
  
  //count the coincidences generated
  long int nCoincidence = 0;
  
  for(int i = 0 ; i < nEvents ; i++)
  {
    //std::cout << "Event " << i << std::endl;
    
    
    std::vector<double> t;
    std::vector<Float_t> x,y,z,e;
    std::vector<int> id;
    
    
    
    //get the "first" single
    tree->GetEntry(i);
    t.push_back( time + absTime);
    id.push_back( entry );
    x.push_back( (Float_t) xPos );
    y.push_back( (Float_t) yPos );
    z.push_back( (Float_t) zPos );
    e.push_back( (Float_t) energy );
    
    //     tree->GetEntry(i+1);
    //     t.push_back(time);
    //     e.push_back(entry);
    //     std::cout << "diff " << t[1] - t[0] << std::endl;
    
    //get all the events in a 90ns time window
    int j=0;
    while( (fabs(t[j] - t[0]) < timeWindow) && ((i+j) < nEvents) )
    {
      j++;
      tree->GetEntry(i+j);
      t.push_back( time + absTime);
      id.push_back(entry);
      x.push_back( (Float_t) xPos);
      y.push_back( (Float_t) yPos);
      z.push_back( (Float_t) zPos);
      e.push_back( (Float_t) energy);
      //std::cout << "diff " << t[j] - t[0] << std::endl;
    }
    
    //std::cout << "size " << t.size() << std::endl;
    for(int j = 1 ; j < t.size()  ; j++)
    {
      //first check, the coincidence is NOT in the same detector head
      if(z[0]*z[j] < 0)
      {
	
	
	//prepare the coincidence event to be written 
	nCoincidence++;
	EventFormat fe;
	fe.ts = t[0];
	fe.random = randomSet;
	fe.d = distance;
	fe.yozRot = yozRot;
	
// 	//invert x with y - definitions in clearpem are different
// 	fe.x1 = y[0];
// 	fe.y1 = x[0];
// 	fe.z1 = z[0];
// 	fe.e1 = e[0] * 1000;
// 	fe.n1 = n1Set;
	
// // 	invert x with y - definitions in clearpem are different
// 	fe.x2 = y[j];
// 	fe.y2 = x[j];
// 	fe.z2 = z[j];
// 	fe.e2 = e[j] * 1000;
// 	fe.n2 = n2Set;
// 	fe.dt = fabs(t[j] - t[0]);
	
	//geometry fixed in the simulation, axis don't need to be inverted anymore
	fe.x1 = x[0];
	fe.y1 = y[0];
	fe.z1 = z[0];
	fe.e1 = e[0] * 1000;
	fe.n1 = n1Set;
        fe.x2 = x[j];
	fe.y2 = y[j];
	fe.z2 = z[j];
	fe.e2 = e[j] * 1000;
	fe.n2 = n2Set;
	fe.dt = fabs(t[j] - t[0]);
	
	
	//debug
// 	std::cout << fe.ts
// 	<< " " 
// 	<< std::bitset<8>(fe.random)
// 	<< " "
// 	<< fe.d
// 	<< " "
// 	<< fe.x1
// 	<< " "
// 	<< fe.y1
// 	<< " "
// 	<< fe.z1
// 	<< " "
// 	<< fe.e1
// 	<< " "
// 	<< std::bitset<8>(fe.n1)
// 	<< " "
// 	<< fe.x2
// 	<< " "
// 	<< fe.y2
// 	<< " "
// 	<< fe.z2		
// 	<< " "
// 	<< fe.e2
// 	<< " "
// 	<< std::bitset<8>(fe.n2)
// 	<< " "
// 	<< fe.dt
// 	<< std::endl;
	
	//write data to file
	output_file.write((char*)&fe,sizeof(fe));
	
	if(fabs(t[j] - t[0]) < dt)
	{
	  if(id[j] == id[0])
	  {
	    //std::cout << "true " << t[j] - t[0] << " " << id[0] << " " << id[j] << std::endl;
	    ctrue++;
	  }
	  else
	  {
	    //std::cout << "random " << t[j] - t[0] << " " << id[0] << " " << id[j] << std::endl;
	    crandom++;
	  }
	}
      }
      //delete t;
      //delete e;
      //cout << i << "\t" << entry<< "\t" << std::setprecision(11)  << time << endl;
    }
  }
  
  output_file.close();
  
  
  std::cout << std::endl;
  std::cout << "Number of coincidences written = " << nCoincidence << std::endl;
  std::cout << "Trues = " << ctrue << std::endl;
  std::cout << "Randoms = " << crandom << std::endl;
  std::cout << std::endl;
  std::cout << "Output written to " << outFileName << std::endl;
  
  return 0;
}