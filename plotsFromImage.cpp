// program to analyze listmode data from an acq and build spectra for each crystal
// the program is based on mousescan structure 
// it runs on all the events, find the crystals, fill them with the energy and timing data written in the elm2 file
// at the same time avoiding repetitions 

// compile with
// g++ -o ../build/plotsFromImage plotsFromImage.cpp `root-config --cflags --glibs` -lSpectrum

// run with
// plotsFromImage data.elm2 [totalTime] [singleFrameTime]

#define printGif false  // set to true if you want to print the .gif canvases during execution. then you have to recompile

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <getopt.h>
#include <iostream>
#include <set>
#include <assert.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>      // std::setw

#include <TROOT.h>
#include <TSpectrum.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TH2F.h>
#include <TFile.h>
#include <TH1F.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include "TError.h"

#define LOWER_ENERGY_ACCEPTED_STRICT 507
#define HIGHER_ENERGY_ACCEPTED_STRICT 515
#define LOWER_ENERGY_ACCEPTED_RELAXED 501
#define HIGHER_ENERGY_ACCEPTED_RELAXED 521
#define LOW_STATISTICS_CUTOFF 2000

static float eMin = 400;                // energy window for the scatter evaluation
static float eMax = 650;                // energy window for the scatter evaluation
static float dMax = 4E-9;               // time window 
static int   maxHits = 1;               // set to 1, here we don't want to sum energies from different crystals
static float tRandMin = 20e-9;          // time window to evaluate randoms
static float tRandMax = 90e-9;          // time window to evaluate randoms

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
  if(argc < 2) // tell the user how to use the program
  {
    std::cout << "USAGE:\t\t plotsFromImage <file.elm2> [totalTime] [singleFrameTime]" << std::endl;
    std::cout << "file.elm2 \t\t input .elm2 file" << std::endl;
    std::cout << "totalTime \t\t total acq time in sec (OPTIONAL, by default is 30min)" << std::endl;
    std::cout << "singleFrameTime \t time length of single frama in sec (OPTIONAL, by default is 5min)" << std::endl;
    std::cout << "halfLife \t Half life of the isotope in SECONDS (OPTIONAL, by default it's 18F-FDG -> 109min)" << std::endl;
    std::cout << std::endl;
    std::cout << "Spectra will be stored in a file spectra_file.root" << std::endl;
    std::cout << std::endl;
    return 1;
  }
  else
  {
    //----parse command line args
    std::string filename,filenameNoExtension;
    // sort between input from stdin and from file
    if(strcmp(argv[1], "-") == 0) 
    {
      filename = "stdin";
      filenameNoExtension = "stdin";
    }
    else
    {
      filename = argv[1];
      filenameNoExtension = filename.substr(0,filename.length() -5 );
    }
    // get time frames if input, otherwise stick to default
    int totalTime = 1800; // by default, 30 min 
    int singleFrameTime = 300; // by default, 5 min
    double tau = (109.8*60.0/log(2));  // FDG half life in sec
    if(argc > 2)
    {
      totalTime = atoi(argv[2]);
      singleFrameTime = atoi(argv[3]);
    }   
    if(argc > 4)
      tau = (atof(argv[4])*60.0/log(2));
    
    //---- inizialize some variable
    gErrorIgnoreLevel = kError;
    long long nRandoms = 0;
    long long nPrompts = 0;
    long long nBad = 0;
    long long nEvents = 0;
    long long nTrues = 0;
    float xSize = 171.5 + 2;
    float ySize =  150.2 + 2;
    int pxSize = 1;
    int nx = (xSize)/pxSize;
    int lx = nx * pxSize;
    int ny = (ySize)/pxSize;
    int ly = ny * pxSize;
    
    //---- input coordinates of x and y events, i.e. coordinates of the crystals, found directly in the acquisition data. it's the same for head 1 and head 2 
    double xCoord[64] = 
    {
      -85.75,-83.45,-81.15,-78.85,-76.55,-74.25,-71.95,-69.65,-63.85,-61.55,-59.25,-56.95,-54.65,-52.35,-50.05,-47.75,-41.25,-38.95,-36.65,-34.35,-32.05,-29.75,-27.45,-25.15,-19.35,-17.05,-14.75,-12.45,-10.15,-7.85,-5.55,-3.25,3.25,5.55,7.85,10.15,12.45,14.75,17.05,19.35,25.15,27.45,29.75,32.05,34.35,36.65,38.95,41.25,47.75,50.05,52.35,54.65,56.95,59.25,61.55,63.85,69.65,71.95,74.25,76.55,78.85,81.15,83.45,85.75
    };
    double yCoord[48] = 
    {
      -75.10,-72.8,-70.2,-67.9,-62.1,-59.8,-57.2,-54.9,-49.1,-46.8,-44.2,-41.9,-36.1,-33.8,-31.2,-28.9,-23.1,-20.8,-18.2,-15.9,-10.1,-7.8,-5.2,-2.9,2.9,5.2,7.8,10.1,15.9,18.2,20.8,23.1,28.9,31.2,33.8,36.1,41.9,44.2,46.8,49.1,54.9,57.2,59.8,62.1,67.9,70.2,72.8,75.1
    };
    
    //----prepare output root file
    std::string spectraFileName = "spectra_" + filenameNoExtension + ".root";
    TFile *f = new TFile(spectraFileName.c_str(), "RECREATE");
    
    //----prepare histos and arrays
    TH1F *spectra[2][64][48];                                                                // array of histograms for the single crystal energy spectrum
    TH2F *spectraTime[2][64][48];                                                            // array of histograms for the single crystal energy vs. time scatter plot
    TH1F *deltaTime[2][64][48];                                                              // array of histograms for the single crystal coincidence time spectrum
    std::vector<double> corrX_peakVsTime[2][64][48],corrY_peakVsTime[2][64][48];             // arrays of vectors for the TGraphErrors (peak position vs time)
    std::vector<double> corrXerr_peakVsTime[2][64][48],corrYerr_peakVsTime[2][64][48];       // arrays of vectors for the TGraphErrors (peak position vs time)
    long int promptMap[2][64][48];                                                           // array of the num of prompts per channel
    long int randomMap[2][64][48];                                                           // array of the num of random per channel
    
    //group of histos, filled with different time frames to compensate for fdg decay
    //---- compute the time intervals to get "stable" statistics
    //tau = 60000000; // fix for no different time frame
    double presentTime = 0;
    double presentFrame = singleFrameTime;
    std::vector<double> xTime;
    std::vector<long long int> xTimeI;
    int xTimeCounter = 0;
    double nextFrame;
    xTime.push_back(presentTime);        // time starts from 0, then first frame is decided by the user
    xTime.push_back(singleFrameTime);    // so the second time point is the first frame 
    
    while(xTime[xTime.size()-1] < totalTime)
    {
      nextFrame = -tau * log( 2.0*exp(- xTime[xTime.size()-1]/tau) -exp(-xTime[xTime.size()-2]/tau) );
      xTime.push_back(xTime[xTimeCounter-1] + nextFrame); 
    }  
    for(int i = 0 ; i < xTime.size() ; i++)
    {
      xTimeI.push_back(round(xTime[i]));
    }
    //little fix for log and exp exploding
    xTime[xTime.size()-1] = totalTime;
    xTimeI[xTimeI.size()-1] = totalTime;
//     for(int i = 0 ; i < xTime.size() ; i++)
//     {
//       std::cout << xTime[i] << " " << xTimeI[i] << std::endl;
//     }
    
    TH1F **spectraSplitTimeWeighted[2][64][48];

    for(int head = 0; head < 2; head++) for(int xCry = 0; xCry < 64; xCry++) for(int yCry = 0; yCry < 48; yCry++) 
    {
      spectraSplitTimeWeighted[head][xCry][yCry]= new TH1F*[xTimeI.size()-1];
      for(int k = 0; k < xTimeI.size()-1; k++)
      {
	std::stringstream histogramSplit;
        histogramSplit << "Time_weighted_spectrum_head" << head << "_x" << xCry << "_y" << yCry << "- "<< xTimeI[k] << " to " << xTimeI[k+1] << " sec";
	spectraSplitTimeWeighted[head][xCry][yCry][k] = new TH1F(histogramSplit.str().c_str(),histogramSplit.str().c_str(),100,0,1000);
      }
    }
    
    //----initialize arrays and histos
    for(int head = 0; head < 2; head++) for(int xCry = 0; xCry < 64; xCry++) for(int yCry = 0; yCry < 48; yCry++) 
    {
      char name[256],nameDelta[256],nameTime[256],nameTimeOriginal[256];
      sprintf(name, "spectrum_head%d_x%d_y%d", head , xCry , yCry);
      spectra[head][xCry][yCry] = new TH1F(name, name, 550, 0 , 1100);
      spectra[head][xCry][yCry]->GetXaxis()->SetTitle("KeV");
      spectra[head][xCry][yCry]->GetYaxis()->SetTitle("N");
      sprintf(nameDelta, "deltaTime_head%d_x%d_y%d", head , xCry , yCry);
      deltaTime[head][xCry][yCry] = new TH1F(nameDelta, nameDelta, 500, -100e-9 , 100e-9);
      deltaTime[head][xCry][yCry]->GetXaxis()->SetTitle("ns");
      deltaTime[head][xCry][yCry]->GetYaxis()->SetTitle("N");
      promptMap[head][xCry][yCry] = 0;
      randomMap[head][xCry][yCry] = 0;
      sprintf(nameTime, "spectrumTime_head%d_x%d_y%d", head , xCry , yCry);
      spectraTime[head][xCry][yCry] = new TH2F(nameTime, nameTime, totalTime/singleFrameTime,0,totalTime,100,0,1000);
      spectraTime[head][xCry][yCry]->GetXaxis()->SetTitle("Seconds");
      spectraTime[head][xCry][yCry]->GetYaxis()->SetTitle("KeV");
    }
    
    float smX = xSize / 4;
    float smY = ySize / 2;
    double t0 = -1;
    double tf = 0; 
    int preX1 = 0;
    int preX2 = 0;
    int preY1 = 0;
    int preY2 = 0;
    double preE1 = 0;
    double preE2 = 0;
    
    //----input part
    FILE * fIn = NULL;
    //choose between input from file or stdin
    if(strcmp(argv[1], "-") == 0)
    {
      fIn = stdin;
    }
    else
      fIn = fopen(argv[1], "rb");
    if (fIn == NULL) 
    {
      fprintf(stderr, "File %s does not exist\n", argv[1]);
      return 1;
    }
    printf("Reading %s\n", argv[1]);
    
    double lastp = -1;
    double lastr = -1;
    EventFormat fe;        // instantiate input struct
    float angleFound = -1;
    //counter of var length time slices
    int nSlice = 0;
    
    //read input elm2 file
    while(fread((void*)&fe, sizeof(fe), 1, fIn) == 1 /*&& nEvents < 30000000*/) 
    {
      if(t0 == -1) t0 = fe.ts; 		//t0 is effectively calculated only at the first cycle
      tf = fe.ts;
      
//       if((tf - t0) < 5400) continue; // to cut the first 5400 seconds of the dataset
      
      if(nEvents == 0)
	printf("distance = %f\n",fe.d);
      nEvents++;
      if(angleFound != fe.yozRot)
      {
	angleFound = fe.yozRot;
	printf("\nAngle found [rad] = %f \t\t @ event number %lld \t\t @ time %f \n",angleFound,nEvents,tf-t0);
      }
      
      //check time wrt the variable length time slices
      if( ((tf-t0) < totalTime) && ((tf-t0) > xTimeI[nSlice+1]) && (nSlice < (xTimeI.size()-1) ))
      {
	std::cout << std::endl;
	std::cout << "End of slice\t" << nSlice << "\tat time\t"<< tf-t0 << std::endl;
	nSlice++;
      }
      
      float u = fe.x1 - fe.x2;
      float v = fe.y1 - fe.y2;
      float w = fe.z1 - fe.z2;
      
      if(fabs(w) < fe.d/2.0) {
	//printf("%f %f\n",fabs(w),fe.d/2.0);
	nBad++;
	continue;
      }
      
      double realDT = fe.dt;
      if (fe.z1 > 0)
	fe.dt *= -1;
      
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
      
      //a bit of a lame implementation, but this way it should be straightforward to read
      bool sameEnergyAsPrevious1   = false;
      bool sameEnergyAsPrevious2   = false;
      bool samePositionAsPrevious1 = false;
      bool samePositionAsPrevious2 = false;
      bool aboveMaxHits1           = false;
      bool aboveMaxHits2           = false;
      
      if(fe.n1 > maxHits )
	aboveMaxHits1 = true;
      if(fe.n2 > maxHits )
	aboveMaxHits2 = true;
      if(fe.e1 == preE1)
	sameEnergyAsPrevious1 = true;
      if(fe.e2 == preE2)
	sameEnergyAsPrevious2 = true;
      if((preX1 == x1_id) && (preY1 == y1_id))
	samePositionAsPrevious1 = true;
      if((preX2 == x2_id) && (preY2 == y2_id))
	samePositionAsPrevious2 = true;
      
      //fill the correct histos, according to the crystal id found
      if( !(aboveMaxHits1 | (sameEnergyAsPrevious1 && samePositionAsPrevious1)) )// so this is true if the gamma is not multiple and the gamma is not the same of the event before
//       if(true)
      {
	spectraTime[0][x1_id][y1_id]->Fill(tf-t0,fe.e1,1.0/(exp(-((tf-t0)/(109.8*60.0/log(2))))));
	spectra[0][x1_id][y1_id]->Fill(fe.e1);
	spectraSplitTimeWeighted[0][x1_id][y1_id][nSlice]->Fill(fe.e1);
	deltaTime[0][x1_id][y1_id]->Fill(realDT);
      }
      
      if( !(aboveMaxHits2 | (sameEnergyAsPrevious2 && samePositionAsPrevious2)) )// so this is true if the gamma is not multiple and the gamma is not the same of the event before
//       if(true)
      { 
	spectraTime[1][x2_id][y2_id]->Fill(tf-t0,fe.e2,1.0/(exp(-((tf-t0)/(109.8*60.0/log(2))))));
	spectra[1][x2_id][y2_id]->Fill(fe.e2);
	spectraSplitTimeWeighted[1][x2_id][y2_id][nSlice]->Fill(fe.e2);
	deltaTime[1][x2_id][y2_id]->Fill(realDT);
      }
      
      preX1 = x1_id;
      preY1 = y1_id;
      preX2 = x2_id;
      preY2 = y2_id;
      preE1 = fe.e1;
      preE2 = fe.e2;
      
      //randomFlag->Fill(fe.random); 
      bool eOK = (fe.e1 >= eMin) && (fe.e1 <= eMax) && (fe.e2 >= eMin) && (fe.e2 <= eMax);
      bool tOK = fabs(fe.dt) < dMax;
      bool p = fe.random == 0;
      bool hitOK = (fe.n1 < maxHits + 1) && (fe.n2 < maxHits +1);
      bool randomTimeWindow = (fabs(fe.dt) > 20e-9) && (fabs(fe.dt) < 90e-9);
      
      if(eOK && tOK && p) {
	int head1 = fe.z1 < 0 ? 0 : 1;
	int sm1 = ((fe.x1 + xSize/2) / smX);
	int ab1 =  (fe.y1 + ySize/2) / smY;			
	assert(head1 >= 0); assert(head1 <= 1);
	assert(sm1 >= 0); assert(sm1 <= 3);
	assert(ab1 >= 0); assert(ab1 <= 1);
	int head2 = fe.z2 < 0 ? 0 : 1;
	int sm2 = ((fe.x2 + xSize/2) / smX);
	int ab2 =  (fe.y2 + ySize/2) / smY;
	assert(head2 >= 0); assert(head2 <= 1);
	assert(sm2 >= 0); assert(sm2 <= 3);
	assert(ab2 >= 0); assert(ab2 <= 1);
      }
      
      if(eOK && tOK && p) {
	if(lastp != -1)
	  // 					interval->Fill(fe.ts - lastp);
	  lastp = fe.ts;
	// 				nhits->Fill(fe.n1); nhits->Fill(fe.n2);
      }
      else if (eOK && tOK && !p) {
	if(lastr != -1)
	  // 					rinterval->Fill(fe.ts - lastr);
	  lastr = fe.ts;
	// 				rnhits->Fill(fe.n1); rnhits->Fill(fe.n2);
      }
      
      float s = (0 - fe.z1)/w;
      float mx = fe.x1 + s*u;
      float my = fe.y1 + s*v;
      
      // increase the counters of prompts and randoms for the crystals involved in this event
      // FIXME here actually i'm not checking for repetitions!
      if(eOK && tOK && p && hitOK) {  //if event is in the energy window and delta t < 4ns, then it's a true (prompt)
	nTrues++;
	promptMap[0][x1_id][y1_id]++;
	promptMap[1][x2_id][y2_id]++;
	// 			  (p ? h : hr)->Fill(mx, my);
      }
      if(eOK && randomTimeWindow && p && hitOK) {  //if the event is in the energy window and in the delayed delta time, then it's a random
	nRandoms++;
	randomMap[0][x1_id][y1_id]++;
	randomMap[1][x2_id][y2_id]++;
	// 			  (p ? h : hr)->Fill(mx, my);
      }
      
      //feedback to user, number of events processed
      if( (nEvents % 10000) == 0 )
      {
        std::cout << "\r";
        std::cout << "Events " << std::setw(10) << nEvents << " - Acq Time [sec] " << tf-t0;      
      }
    }
    std::cout << std::endl;
    
    // we need to rescale the number of random between the tRandMin and tRandMax to be comparable to the 4 ns time window
    double randomRescaled = nRandoms * (dMax / (tRandMax - tRandMin));
    
    //feedback to the user
    printf("Total Time [s] = %f \n\n", tf-t0);
    printf("%lld Total Events\n", nEvents);
    printf("%lld Prompts\n", nTrues);
    printf("%.0lf Randoms\n", randomRescaled);
    if(nBad > 0) printf("Found %lld insane events\n", nBad);
    int failed[2] = {0,0};         // counter for the crystals where fitting procedure failed    
    int lowStatistics[2] = {0,0};  // counter for the crystals where fitting is not started because of low statistics cutoff
    int success[2] = {0,0};        // counter for the crystals where fitting procedure was performed and it was a success 
    int outOfRangeRelaxed[2] = {0,0};       // counter for the crystals out of the energy tolerance range defined above (#define LOWER_ENERGY_ACCEPTED_RELAXED and HIGHER_ENERGY_ACCEPTED_RELAXED)
    int outOfRangeStrict[2] = {0,0};        // counter for the crystals out of the energy tolerance range defined above (#define LOWER_ENERGY_ACCEPTED_STRICT and HIGHER_ENERGY_ACCEPTED_STRICT)
    
    //----prepare global histograms and canvases
    TH2F *peaks[2], *fwhm[2], *peak_fwhm[2], *scatter[2], *random[2], *randomCounts[2] , *promptCounts[2], *deltaT[2];
    TCanvas *C_peak[2],*C_fwhm[2],*C_peak_fwhm[2], *C_scatter[2], *C_random[2], *C_randomCounts[2] , *C_promptCounts[2], *C_deltaT[2];
    TH1F *peaks_distro[2];
    //2d map of the energy resolutions per pixel 
    fwhm[0] = new TH2F("fwhm0","fwhm0",64,0,64,48,0,48);
    fwhm[1] = new TH2F("fwhm1","fwhm1",64,0,64,48,0,48);
    C_fwhm[0] = new TCanvas("Fwhm0","Fwhm0",1200,800);
    C_fwhm[1] = new TCanvas("Fwhm1","Fwhm1",1200,800);
    //2d map of the 511KeV peak position per pixel
    peaks[0] = new TH2F("peaks0","peaks0",64,0,64,48,0,48);
    peaks[1] = new TH2F("peaks1","peaks1",64,0,64,48,0,48);
    C_peak[0] = new TCanvas("Peaks0","Peaks0",1200,800);
    C_peak[1] = new TCanvas("Peaks1","Peaks1",1200,800);
    //1d histo of the 511KeV peak position distribution
    peaks_distro[0] = new TH1F("peaks_distro0","peaks_distro0",200,300,700);
    peaks_distro[1] = new TH1F("peaks_distro1","peaks_distro1",200,300,700);
    //2d scatter plot of the 511KeV peak position vs. energy resolution
    peak_fwhm[0] = new TH2F("peak_fwhm0","peak_fwhm0",400,460,580,400,0,100);
    peak_fwhm[1] = new TH2F("peak_fwhm1","peak_fwhm1",400,460,580,400,0,100);
    C_peak_fwhm[0] = new TCanvas("peak_fwhm0","peak_fwhm0",1200,800);
    C_peak_fwhm[1] = new TCanvas("peak_fwhm1","peak_fwhm1",1200,800);
    //2d map of the estimated scatter fraction per pixel
    scatter[0] = new TH2F("scatter0","scatter0",64,0,64,48,0,48);
    scatter[1] = new TH2F("scatter1","scatter1",64,0,64,48,0,48);
    C_scatter[0] = new TCanvas("Scatter0","Scatter0",1200,800);
    C_scatter[1] = new TCanvas("Scatter1","Scatter1",1200,800);
    //2d map of the estimated random fraction per pixel
    random[0] = new TH2F("randomFraction0","randomFraction0",64,0,64,48,0,48);
    random[1] = new TH2F("randomFraction1","randomFraction1",64,0,64,48,0,48);
    C_random[0] = new TCanvas("RandomFraction0","RandomFraction0",1200,800);
    C_random[1] = new TCanvas("RandomFraction1","RandomFraction1",1200,800);
    //2d map of the estimated number of random events per pixel
    randomCounts[0] = new TH2F("randomCounts0","randomCounts0",64,0,64,48,0,48);
    randomCounts[1] = new TH2F("randomCounts1","randomCounts1",64,0,64,48,0,48);
    C_randomCounts[0] = new TCanvas("RandomCounts0","RandomCounts0",1200,800);
    C_randomCounts[1] = new TCanvas("RandomCounts1","RandomCounts1",1200,800);
    //2d map of the estimated number of prompt events per pixel
    promptCounts[0] = new TH2F("promptCounts0","promptCounts0",64,0,64,48,0,48);
    promptCounts[1] = new TH2F("promptCounts1","promptCounts1",64,0,64,48,0,48);
    C_promptCounts[0] = new TCanvas("PromptCounts0","PromptCounts0",1200,800);
    C_promptCounts[1] = new TCanvas("PromptCounts1","PromptCounts1",1200,800);
    //2d map of the coincidence time resolution per pixel
    deltaT[0] = new TH2F("deltaT","deltaT0",64,0,64,48,0,48);
    deltaT[1] = new TH2F("deltaT","deltaT1",64,0,64,48,0,48);
    C_deltaT[0] = new TCanvas("DeltaT0","DeltaT0",1200,800);
    C_deltaT[1] = new TCanvas("DeltaT1","DeltaT1",1200,800);
    
    std::cout << std::endl;
    std::cout << "Analyzing single crystals spectra..." << std::endl;
    
    //----run on all the crystals and produce the maps
    for(int head = 0; head < 2; head++) for(int xCry = 0; xCry < 64; xCry++) for(int yCry = 0; yCry < 48 ; yCry++) 
    {
      
      //feedback to the user 
      std::cout << "Head " << std::setw(2) <<  head << "/1" << "\t" << "x " << std::setw(2) << xCry << "/63" << "\t\t" << "y " << std::setw(2) << yCry << "/47"/*<< "\t" << " Subplot " << k*/;
      std::cout << "\r";
      
      std::stringstream histogram;
      histogram << "spectrum_head" << head << "_x" << xCry << "_y" << yCry;
      
      // PEAK POSITION
      //first check: don't even try to fit an histogram if the entries are too few (set by #define...)
      if(spectra[head][xCry][yCry]->GetEntries() < LOW_STATISTICS_CUTOFF)
      {
	lowStatistics[head]++;
      }
      else
      {
	//find an approx peak position, then fit in a reasonable range
	TSpectrum *s = new TSpectrum(1,1);
	Int_t nfound = s->Search(spectra[head][xCry][yCry],20,"");
	Float_t *xpeaks = s->GetPositionX();
	
	double temp = 0;
	for(int i = 0; i < nfound; i++) //find the highest peak (it should be only one, but if the compton convoluted to the trigger mimics a peak, for sure the highest is the photoelectric one
	{
	  if(xpeaks[i]>temp) temp = xpeaks[i];
	}
	
	double mean = temp;
	spectra[head][xCry][yCry]->Fit("gaus","Q","",mean-(mean*0.06),mean+(mean*0.15));
	TF1 *fit = spectra[head][xCry][yCry]->GetFunction("gaus");
	
	if(fit != NULL) //all the following plots can be filled only if the fit is ok
	{
	  if(fit->GetParameter(1) < LOWER_ENERGY_ACCEPTED_RELAXED | fit->GetParameter(1) > HIGHER_ENERGY_ACCEPTED_RELAXED )
	  {
	    outOfRangeRelaxed[head]++;
	  }
	  if(fit->GetParameter(1) < LOWER_ENERGY_ACCEPTED_STRICT | fit->GetParameter(1) > HIGHER_ENERGY_ACCEPTED_STRICT )
	  {
	    outOfRangeStrict[head]++;
	  }
	  
	  peaks[head]->Fill(xCry,yCry,fit->GetParameter(1)); //fill the histo of peak mean 
	  peaks_distro[head]->Fill(fit->GetParameter(1));
	  fwhm[head]->Fill(xCry,yCry,100.0*(fit->GetParameter(2)*2.35)/fit->GetParameter(1)); //fill the histo of en.res fwhm
	  peak_fwhm[head]->Fill(fit->GetParameter(1),100.0*(fit->GetParameter(2)*2.35)/fit->GetParameter(1)); // fill the histo peak pos vs. en res
	  //calculate scatter fraction from plot
	  // find the bins corresponding to the energy window
	  int binMin = spectra[head][xCry][yCry]->GetXaxis()->FindBin(eMin);
	  int binMax = spectra[head][xCry][yCry]->GetXaxis()->FindBin(eMax);
	  double histo_integral = spectra[head][xCry][yCry]->Integral(binMin,binMax,"width");
	  // integrate the fitted gaussian in the same interval
	  double fit_integral = fit->Integral(eMin,eMax);
	  double scatterFraction = (histo_integral-fit_integral)/histo_integral;
	  scatter[head]->Fill(xCry,yCry,scatterFraction);
	  success[head]++;
	}
	else
	  failed[head]++;  // increase the counter of failed fits
      }
      // ------- PEAK POSITION - END
      
      //RANDOM MAPS
      // the random maps are not dependent on the fit
      double randomRescaledPerCrystal = randomMap[head][xCry][yCry] * (dMax / (tRandMax - tRandMin));
      double randomFraction;
      if( (randomRescaledPerCrystal != 0) && (promptMap[head][xCry][yCry] != 0) )
      {
	randomFraction = randomRescaledPerCrystal/promptMap[head][xCry][yCry];
	random[head]->Fill(xCry,yCry,randomFraction);
	randomCounts[head]->Fill(xCry,yCry,randomRescaledPerCrystal);
	promptCounts[head]->Fill(xCry,yCry,promptMap[head][xCry][yCry]);
      }
      else
      {
	random[head]->Fill(xCry,yCry,0);
	randomCounts[head]->Fill(xCry,yCry,0);
	promptCounts[head]->Fill(xCry,yCry,0);
      }
      
      //DELTA T MAP
      //delta T as to be fitted as well
      TSpectrum *s2 = new TSpectrum(1,1);
      Int_t nfound2 = s2->Search(deltaTime[head][xCry][yCry],2e-9,"",0.3);
      Float_t *xpeaks2 = s2->GetPositionX();
      double mean2 = xpeaks2[0];
      deltaTime[head][xCry][yCry]->Fit("gaus","Q","",mean2 - 4e-9,mean2 + 4e-9);
      TF1 *fit2 = deltaTime[head][xCry][yCry]->GetFunction("gaus");
      if(fit2 != NULL)
      {
	deltaT[head]->Fill(xCry,yCry,2.35*fit2->GetParameter(2));
      }
      // ------ RANDOM MAPS - END
      
      // PEAK MOVEMENT DURING ACQ
      // peak movement, histos weighted in time
      for(int k=0 ; k < xTimeI.size()-1 ; k++)
      {
	std::stringstream histogramSplit;
        histogramSplit << "Time_weight_spectrum_head" << head << "_x" << xCry << "_y" << yCry << "_time"<< k;
	
	//find peaks in this projection slice
	TSpectrum *s3 = new TSpectrum(1,1);
	Int_t nfound3 = s3->Search(spectraSplitTimeWeighted[head][xCry][yCry][k],5,"");
	Float_t *xpeaks3 = s3->GetPositionX();
	
	double temp3 = 0;
	for(int i = 0; i < nfound3; i++) //find the highest peak (it should be only one, but if the compton convoluted to the trigger mimics a peak, for sure the highest is the photoelectric one)
	{
	  if(xpeaks3[i]>temp3) temp3 = xpeaks3[i];
	}
	
	//fit the peak
	double mean3 = temp3;
	spectraSplitTimeWeighted[head][xCry][yCry][k]->Fit("gaus","Q","",mean3-(mean3*0.06),mean3+(mean3*0.15));
	TF1 *fit3 = spectraSplitTimeWeighted[head][xCry][yCry][k]->GetFunction("gaus");
	if(fit3 != NULL)
	{
	  corrX_peakVsTime[head][xCry][yCry].push_back(xTimeI[k+1]);
	  corrXerr_peakVsTime[head][xCry][yCry].push_back(0);
	  corrY_peakVsTime[head][xCry][yCry].push_back(fit3->GetParameter(1));
	  corrYerr_peakVsTime[head][xCry][yCry].push_back(fit3->GetParError(1));
	}
	else // now, this is terrible but makes things easier later
	     // basically, if the fit for some reason is not working, instead of skipping the point, put a fake point with no error
             // in (xTimeI[k+1 , -1). This makes the graphs bad because somehow the TBrowser decides to plot the with lines connectiong the points
	     // but if they are properly plotted on the TCanvas, the point is not visible. Then, the analysis don with studyModule becomes fully consistent
	{
	  corrX_peakVsTime[head][xCry][yCry].push_back(xTimeI[k+1]);
	  corrXerr_peakVsTime[head][xCry][yCry].push_back(0);
	  corrY_peakVsTime[head][xCry][yCry].push_back(-1);
	  corrYerr_peakVsTime[head][xCry][yCry].push_back(0);
	}
      }
      // -------- PEAK MOVEMENT DURING ACQ - END
    }
    
    std::cout << std::endl;
    std::cout << "Crystal spectra analysis finished" << std::endl;
    std::cout << std::endl;
    for(int head = 0 ; head < 2 ; head++)
    {
      std::cout << "Detector head "<< head << std::endl;
      std::cout << "Fitting was not performed for "     << lowStatistics[head] << " crystals out of 3072 because of low statistics" << std::endl;
      std::cout << "Fitting photopeak didn't work for " << failed[head]        << " crystals out of "<< 3072-lowStatistics[head] << " analyzed" << std::endl;
      std::cout << "Fitting success for "               << success[head]       << " crystals out of "<< 3072-lowStatistics[head] << " analyzed" << std::endl;
      std::cout << "Channels out of " << LOWER_ENERGY_ACCEPTED_RELAXED << " - " << HIGHER_ENERGY_ACCEPTED_RELAXED << " KeV window " << outOfRangeRelaxed[head] << "/" << 3072-lowStatistics[head] << std::endl;
      std::cout << "Channels out of " << LOWER_ENERGY_ACCEPTED_STRICT << " - " << HIGHER_ENERGY_ACCEPTED_STRICT << " KeV window " << outOfRangeStrict[head] << "/" << 3072-lowStatistics[head] << std::endl;
      std::cout << std::endl;
    }
    std::cout << "Generating graphs and maps..." << std::endl;
    //---- plot the TGraphErrors of peak position vs. time 
    TGraphErrors *corrpeakVsTime[2][64][48];
    TCanvas *corrC_peakVsTime[2][64][48];
    
    for(int head = 0 ; head < 2 ; head++)
    {
      for(int xCry = 0; xCry < 64; xCry++) 
      {
	for(int yCry = 0; yCry < 48; yCry++) 
	{
	  
	  if(corrX_peakVsTime[head][xCry][yCry].size() != 0)
	  {
	    //feedback to the user 
            std::cout << "Head " << std::setw(2) <<  head << "/1" << "\t" << "x " << std::setw(2) << xCry << "/63" << "\t\t" << "y " << std::setw(2) << yCry << "/47"/*<< "\t" << " Subplot " << k*/;
            std::cout << "\r";
	    corrpeakVsTime[head][xCry][yCry] = new TGraphErrors(corrX_peakVsTime[head][xCry][yCry].size(),&corrX_peakVsTime[head][xCry][yCry].at(0),&corrY_peakVsTime[head][xCry][yCry].at(0),&corrXerr_peakVsTime[head][xCry][yCry].at(0),&corrYerr_peakVsTime[head][xCry][yCry].at(0));
	    std::stringstream title;
	    title << "511KeV Peak position vs. Time - Head" << head << "_x" << xCry << "_y" << yCry ;
	    corrC_peakVsTime[head][xCry][yCry] = new TCanvas(title.str().c_str(),title.str().c_str());
	    corrC_peakVsTime[head][xCry][yCry]->cd();
	    corrpeakVsTime[head][xCry][yCry]->SetTitle(title.str().c_str());
	    corrpeakVsTime[head][xCry][yCry]->SetMarkerStyle(21);
	    corrpeakVsTime[head][xCry][yCry]->GetXaxis()->SetTitle("Time [s]");
	    corrpeakVsTime[head][xCry][yCry]->GetYaxis()->SetTitle("[KeV]");
	    corrpeakVsTime[head][xCry][yCry]->GetYaxis()->SetRangeUser(420,650);
	    corrpeakVsTime[head][xCry][yCry]->Draw("AP");
	  }
	}
      }
    }
    
    //----draw the maps into the canvases
    for(int head = 0 ; head < 2 ; head++) 
    {
      std::stringstream PlotFileName;
      
      C_peak[head]->cd();
      peaks[head]->GetZaxis()->SetRangeUser(450, 600);
      peaks[head]->Draw("COLZ");
      PlotFileName << "PhotopeakPosition" << head << ".gif";
      if(printGif)
        C_peak[head]->Print(PlotFileName.str().c_str());
      
      C_fwhm[head]->cd();
      fwhm[head]->GetZaxis()->SetRangeUser(10, 40);
      fwhm[head]->Draw("COLZ");
      PlotFileName.str("");
      PlotFileName << "EnergyResolutionFWHM" << head << ".gif";
      if(printGif)
        C_fwhm[head]->Print(PlotFileName.str().c_str());
      
      C_peak_fwhm[head]->cd();
      peak_fwhm[head]->Draw("COLZ");
      
      C_scatter[head]->cd();
      scatter[head]->GetZaxis()->SetRangeUser(0, 1);
      scatter[head]->Draw("COLZ");
      PlotFileName.str("");
      PlotFileName << "ScatterFraction" << head << ".gif";
      if(printGif)
	C_scatter[head]->Print(PlotFileName.str().c_str());
      
      C_random[head]->cd();
      random[head]->GetZaxis()->SetRangeUser(0, 1);
      random[head]->Draw("COLZ");
      PlotFileName.str("");
      PlotFileName << "RandomFraction" << head << ".gif";
      if(printGif)
        C_random[head]->Print(PlotFileName.str().c_str());
      
      C_randomCounts[head]->cd();
      //     randomCounts[head]->GetZaxis()->SetRangeUser(0, 1);
      randomCounts[head]->Draw("COLZ");
      PlotFileName.str("");
      PlotFileName << "RandomCounts" << head << ".gif";
      if(printGif)
	C_randomCounts[head]->Print(PlotFileName.str().c_str());
      
      C_promptCounts[head]->cd();
      //     promptCounts[head]->GetZaxis()->SetRangeUser(0, 1);
      promptCounts[head]->Draw("COLZ");
      PlotFileName.str("");
      PlotFileName << "PromptCounts" << head << ".gif";
      if(printGif)
        C_promptCounts[head]->Print(PlotFileName.str().c_str());
      
      C_deltaT[head]->cd();
      deltaT[head]->GetZaxis()->SetRangeUser(0, 8e-9);
      deltaT[head]->Draw("COLZ");
      
      PlotFileName.str("");
      PlotFileName << "DeltaTime" << head << ".gif";
      if(printGif)
        C_deltaT[head]->Print(PlotFileName.str().c_str());
    }
    std::cout << std::endl;
    //---- save results to output root file
    std::cout << "Saving spectra to root file " << spectraFileName << std::endl;
    f->cd();
    TDirectory *directory[2][65][49][2]; //TDirectory 
    for(int head = 0; head < 2; head++)  // save data to root file
    {
      char head_name[256];
      sprintf(head_name, "head%d", head);
      directory[head][0][0][0] = f->mkdir(head_name);
      directory[head][0][0][0]->cd();
      C_peak[head]->Write();
      C_fwhm[head]->Write();
      C_peak_fwhm[head]->Write();
      C_scatter[head]->Write();
      C_random[head]->Write();
      C_randomCounts[head]->Write();
      C_promptCounts[head]->Write();
      C_deltaT[head]->Write();
      peaks[head]->Write();
      fwhm[head]->Write();
      peak_fwhm[head]->Write();
      scatter[head]->Write();
      random[head]->Write();
      randomCounts[head]->Write();
      promptCounts[head]->Write();
      deltaT[head]->Write();
      peaks_distro[head]->Write();
      for(int xCry = 0; xCry < 64; xCry++) 
      {
	char x_name[256];
	sprintf(x_name, "x_%d", xCry);
	directory[head][xCry+1][0][0] = directory[head][0][0][0]->mkdir(x_name);
	directory[head][xCry+1][0][0]->cd();
	for(int yCry = 0; yCry < 48; yCry++) 
	{
	  //feedback to the user 
          std::cout << "Head " << std::setw(2) <<  head << "/1" << "\t" << "x " << std::setw(2) << xCry << "/63" << "\t\t" << "y " << std::setw(2) << yCry << "/47"/*<< "\t" << " Subplot " << k*/;
          std::cout << "\r";
	  
	  char y_name[256];
	  sprintf(y_name, "y_%d", yCry);
	  directory[head][xCry+1][yCry+1][0] = directory[head][xCry+1][0][0]->mkdir(y_name);
	  directory[head][xCry+1][yCry+1][0]->cd();
	  spectra[head][xCry][yCry]->Write();
	  spectraTime[head][xCry][yCry]->Write();
	  deltaTime[head][xCry][yCry]->Write();
	  if(corrX_peakVsTime[head][xCry][yCry].size() != 0)
	  {
	    corrC_peakVsTime[head][xCry][yCry]->Write();
	    corrpeakVsTime[head][xCry][yCry]->Write();
	  }
	  directory[head][xCry+1][yCry+1][1] = directory[head][xCry+1][yCry+1][0]->mkdir("SplitSpectrumVsTime");
	  directory[head][xCry+1][yCry+1][1]->cd();
	  for(int i = 0 ; i < xTimeI.size()-1 ; i++)
	  {
	    spectraSplitTimeWeighted[head][xCry][yCry][i]->Write();
	  }
	}
      }
    }
    std::cout << std::endl;
    std::cout << "Closing output file..." << std::endl;
    gROOT->GetListOfFiles()->Remove(f);
    f->Close();
    std::cout << "File Saved." << std::endl;
    return 0;
  }
}