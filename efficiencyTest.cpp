// simple program to generate a "flat" eml2 file, for testing purposes

// compile with
// g++ -o efficiencyTest efficiencyTest.cpp `root-config --cflags --glibs`

//syntax

// generaElm N
// N = number of events to generate

// output is a file called flat_file.elm2

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
#include <time.h>

#include <TROOT.h>
#include <TSpectrum.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TH2F.h>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include "TError.h"


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
  
  //---- input coordinates of x and y events, i.e. coordinates of the crystals, found directly in the acquisition data. it's the same for head 0 and head 1 
  double xCoord[64] = 
  {
    -85.75,-83.45,-81.15,-78.85,-76.55,-74.25,-71.95,-69.65,-63.85,-61.55,-59.25,-56.95,-54.65,-52.35,-50.05,-47.75,-41.25,-38.95,-36.65,-34.35,-32.05,-29.75,-27.45,-25.15,-19.35,-17.05,-14.75,-12.45,-10.15,-7.85,-5.55,-3.25,3.25,5.55,7.85,10.15,12.45,14.75,17.05,19.35,25.15,27.45,29.75,32.05,34.35,36.65,38.95,41.25,47.75,50.05,52.35,54.65,56.95,59.25,61.55,63.85,69.65,71.95,74.25,76.55,78.85,81.15,83.45,85.75
  };
  double yCoord[48] = 
  {
    -75.10,-72.8,-70.2,-67.9,-62.1,-59.8,-57.2,-54.9,-49.1,-46.8,-44.2,-41.9,-36.1,-33.8,-31.2,-28.9,-23.1,-20.8,-18.2,-15.9,-10.1,-7.8,-5.2,-2.9,2.9,5.2,7.8,10.1,15.9,18.2,20.8,23.1,28.9,31.2,33.8,36.1,41.9,44.2,46.8,49.1,54.9,57.2,59.8,62.1,67.9,70.2,72.8,75.1
  };
  
  
  TH2F* head0 = new TH2F("head0","head0",64,0,64,48,0,48);
  TH2F* head1 = new TH2F("head1","head1",64,0,64,48,0,48);
  TFile *fOut = new TFile("histograms.root", "RECREATE");
  
  
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
      // localize the events in the heads 
      // --> e1 are in head 0, e2 in head 1
      // and find the crystals ids
      int x1_id,y1_id,x2_id,y2_id;
      for(int i = 0; i < 64 ; i++)
      {
	if( (fe.x1 > (xCoord[i] - 0.01) ) && (fe.x1 < (xCoord[i] + 0.01)) )
	{
	  x1_id = i;
	  // 	  printf("%d ",i);
	}
	if( (fe.x2 > (xCoord[i] - 0.01) ) && (fe.x2 < (xCoord[i] + 0.01)) )
	{
	  x2_id = i;
	  // 	  printf("%d ",i);
	}
      }
      for(int i = 0; i < 48 ; i++)
      {
	if( (fe.y1 > (yCoord[i] - 0.01) ) && (fe.y1 < (yCoord[i] + 0.01)) )
	{  
	  y1_id = i;
	  // 	  printf("%d ",i);
	}
	if( (fe.y2 > (yCoord[i] - 0.01) ) && (fe.y2 < (yCoord[i] + 0.01)) )
	{
	  y2_id = i;
	  // 	  printf("%d ",i);
	}
      }
      
      
      
      
      head0->Fill(x1_id,y1_id);
      head1->Fill(x2_id,y2_id);
      
      
      
      nEvents++;
    }
  }
  
  fOut->cd();
  head0->Write();
  head1->Write();
  fOut->Close();
  return 0;
  
}

