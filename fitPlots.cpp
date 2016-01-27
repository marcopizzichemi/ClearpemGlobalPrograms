// compile with
// g++ -o fitPlots fitPlots.cpp `root-config --cflags --glibs` -Wl,--no-as-needed -lHist -lCore -lMathCore

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
#include <vector>
#include <algorithm>
#include <sstream>
#include <string>

#include <TCanvas.h>
#include <TH2F.h>
#include <TF1.h>
#include <TFile.h>
#include <TH1F.h>
#include <TGraphErrors.h>


static float eMin = 100;                // large energy window, the same as the firmware, so i can have all the spectra
static float eMax = 700;
static float dMax = 4E-9;               // not really important, we ar enot concerned with coincidences here
static int   maxHits = 1;               // set to 1, we don't want to sum energies from different crystals

static float tRandMin = 20e-9; 
static float tRandMax = 90e-9;


// struct EventFormat {
//   double ts;
//   u_int8_t random;
//   float d;
//   float yozRot;
//   float x1;
//   float y1;
//   float z1;
//   float e1;
//   u_int8_t n1;
//   float x2;
//   float y2;
//   float z2;
//   float e2;
//   u_int8_t n2;
//   float dt;
// } __attribute__((__packed__));


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


int main(int argc, char * argv[]) 
{
  TFile *file = new TFile(argv[1]);
  int failed = 0;
  TH2F *peaks[2];
  peaks[0] = new TH2F("peaks0","peaks0",64,0,64,48,0,48);
  peaks[1] = new TH2F("peaks1","peaks1",64,0,64,48,0,48);
  
  
  for(int head = 0; head < 2; head++) for(int xCry = 0; xCry < 64; xCry++) for(int yCry = 0; yCry < 48 ; yCry++) 
  {
//     char name[256];
//     sprintf(name, "spectrum_head%d_n%d_m%d", head , xCry , yCry);
    file->cd();
    std::stringstream headDir,xDir,yDir,histogram;
    
    headDir << "head" << head; 
    xDir    << "x_" << xCry;
    yDir    << "y_" << yCry;
    histogram << "spectrum_head" << head << "_x" << xCry << "_y" << yCry;
    
    file->cd(headDir.str().c_str());
    gDirectory->cd(xDir.str().c_str());
    gDirectory->cd(yDir.str().c_str());
    
    TH1F* histo = (TH1F*)gDirectory->Get(histogram.str().c_str());
    histo->Fit("gaus","Q","",480,600);
    
    TF1 *fit = histo->GetFunction("gaus");
    
    if(fit != NULL)
      peaks[head]->Fill(xCry,yCry,fit->GetParameter(1));
    else
      failed++;
    
  }
  
  TFile *fileOut = new TFile("temp.root","RECREATE");
  fileOut->cd();
  peaks[0]->Write();
  peaks[1]->Write();
  fileOut->Close();
  
  
  
  
  
  return 0;
}