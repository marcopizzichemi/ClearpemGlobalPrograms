// small program to cut an elm2 dataset based on the analysis of each crystal spectrum
// the program "plotsFromImage" needs to be run before this one on the same dataset

// compile with
// g++ -o customEnergyThreshold customEnergyThreshold.cpp `root-config --cflags --glibs`

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
#include <fstream>
#include <TApplication.h>

#include <TSpectrum.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TH2F.h>
#include <TFile.h>
#include <TH1F.h>
#include <TGraphErrors.h>
#include "TError.h"


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
  //parse command line args. we need to do it now because the call to TApplication later will mess up things (who knows why)
  std::vector<std::string> arguments;
  for(int i = 0 ; i < argc ; i++)
  {
    arguments.push_back(argv[i]);
  }
  int Narguments = argc;
  
  //Calling TApplication is needed because of some problem loading libraries
  TApplication theApp("App", &argc, argv);
  
  //coordinates of x and y events, i.e. coordinates of the crystals, found directly in the acquisition data. it's the same for head 1 and head 2 
  std::vector<double> xCoord1,yCoord1;
  std::vector<double> xCoord2,yCoord2;
  double xCoord[64] = 
  {
    -85.75,-83.45,-81.15,-78.85,-76.55,-74.25,-71.95,-69.65,-63.85,-61.55,-59.25,-56.95,-54.65,-52.35,-50.05,-47.75,-41.25,-38.95,-36.65,-34.35,-32.05,-29.75,-27.45,-25.15,-19.35,-17.05,-14.75,-12.45,-10.15,-7.85,-5.55,-3.25,3.25,5.55,7.85,10.15,12.45,14.75,17.05,19.35,25.15,27.45,29.75,32.05,34.35,36.65,38.95,41.25,47.75,50.05,52.35,54.65,56.95,59.25,61.55,63.85,69.65,71.95,74.25,76.55,78.85,81.15,83.45,85.75
  };
  double yCoord[48] = 
  {
    -75.10,-72.8,-70.2,-67.9,-62.1,-59.8,-57.2,-54.9,-49.1,-46.8,-44.2,-41.9,-36.1,-33.8,-31.2,-28.9,-23.1,-20.8,-18.2,-15.9,-10.1,-7.8,-5.2,-2.9,2.9,5.2,7.8,10.1,15.9,18.2,20.8,23.1,28.9,31.2,33.8,36.1,41.9,44.2,46.8,49.1,54.9,57.2,59.8,62.1,67.9,70.2,72.8,75.1
  };
  
  //check if all the args were passed, otherwise provide usage instructions
  if(Narguments < 4)
  {
    std::cout << "USAGE:\t\t customEnergyThreshold <N> <spectra.root> <file.elm2> " << std::endl;
    std::cout << "N \t\t Percentage for the variable energy threshold -> for example N = 5 means Emin = Peak - 0.05*Peak" << std::endl;
    std::cout << "spectra.root \t Root file with the analysis on crystal spectra"<< std::endl;
    std::cout << "file.elm2 \t input .elm2 file" << std::endl;
    std::cout << std::endl;
    std::cout << "A new .elm2 file will be produced" << std::endl; 
    return 1;
  }
  else
  {    
    //set the percentage for the Emin
    double userPercentage = atof(arguments.at(1).c_str());
    std::cout << userPercentage << std::endl;
    
    //set filenames for inputs and output 
    std::string filenameElm2,filenameNoExtension;
    if(arguments.at(3) == "-")
    {
      filenameElm2 = arguments.at(3);
      filenameNoExtension = "stdin";
    }
    else
    {
      filenameElm2 = arguments.at(3); //first input elm2 filename
      filenameNoExtension = filenameElm2.substr(0,filenameElm2.length() -5 );
    }
    
    std::cout << filenameNoExtension << std::endl;
    std::string filenameSpectra = arguments.at(2); //root file with spectra
    std::cout << filenameSpectra << std::endl;
    std::stringstream outFileNameStream; //output elm2 file
    outFileNameStream << filenameNoExtension << "Energy" << userPercentage << "perc.elm2";
    std::string outFileName = outFileNameStream.str();
    
    //declare input stream
    FILE * fIn = NULL;
    
    //open spectra root file 
    TFile *spectraFile = new TFile(filenameSpectra.c_str());
    //and get the two th2f of the heads
    std::stringstream headDir[2],histoname[2];
    for(int head = 0 ; head < 2 ; head++)
    {
      headDir[head] << "head" << head;
      histoname[head] << "peaks" << head;
    }
    
    // take the spectra from root file
    TH2F *peaks[2];
    for(int head = 0 ; head < 2 ; head++)
    {
      spectraFile->cd();
      spectraFile->cd(headDir[head].str().c_str());
      peaks[head] = (TH2F*)gDirectory->Get(histoname[head].str().c_str());
    }
    
    //open output stream
    ofstream output_file(outFileName.c_str(), std::ios::binary);
    
    std::cout << "Producing a new listmode file with custom min threshold per channel Emin = Peak - Peak * " << userPercentage << "%..." << std::endl;
    
    // decide between input from stdin and from file
    if(strcmp(filenameElm2.c_str(), "-") == 0)
      fIn = stdin;
    else
      fIn = fopen(filenameElm2.c_str(), "rb");
    
    if (fIn == NULL) 
    {
      fprintf(stderr, "File %s does not exist\n", filenameElm2.c_str());
      return 1;
    }
    printf("Reading %s\n", filenameElm2.c_str());
        
    // filter the input elm2 files
    // event by event, accept the coincidence only if both events are in the custom energy filter window
    // defined by [peak-0.06*peak,650Kev]
    // open output file
    // binary output: create the name based on the first input file name
    
    EventFormat fe_filter_in;
    EventFormat fe_filter_out;
    //go back to beginning of input elm2
    rewind(fIn);
    
    long long int nEventsFilter = 0;
    
    while(fread((void*)&fe_filter_in, sizeof(fe_filter_in), 1, fIn) == 1/* && nEventsFilter < 10*/) 
    {
      nEventsFilter++;
      
      if (fe_filter_in.z1 > 0)
	fe_filter_in.dt *= -1;
      
      // localize the events in the heads 
      // --> e1 are in head 0, e2 in head 1
      // and find the crystals IDs
      int x1_id,y1_id,x2_id,y2_id;
      for(int i = 0; i < 64 ; i++)
      {
	if( (fe_filter_in.x1 > (xCoord[i] - 0.01) ) && (fe_filter_in.x1 < (xCoord[i] + 0.01)) )
	{
	  x1_id = i;
	  // 	  printf("%d ",i);
	}
	if( (fe_filter_in.x2 > (xCoord[i] - 0.01) ) && (fe_filter_in.x2 < (xCoord[i] + 0.01)) )
	{
	  x2_id = i;
	  // 	  printf("%d ",i);
	}
      }
      for(int i = 0; i < 48 ; i++)
      {
	if( (fe_filter_in.y1 > (yCoord[i] - 0.01) ) && (fe_filter_in.y1 < (yCoord[i] + 0.01)) )
	{  
	  y1_id = i;
	  // 	  printf("%d ",i);
	}
	if( (fe_filter_in.y2 > (yCoord[i] - 0.01) ) && (fe_filter_in.y2 < (yCoord[i] + 0.01)) )
	{
	  y2_id = i;
	  // 	  printf("%d ",i);
	}
      }
      
      double photopeak0 = peaks[0]->GetBinContent(x1_id +1 , y1_id + 1);
      double photopeak1 = peaks[1]->GetBinContent(x2_id +1 , y2_id + 1);

      double thresholdFraction = userPercentage / 100.0;
      
      if( (fe_filter_in.e1 > (photopeak0 - thresholdFraction*photopeak0) ) && (fe_filter_in.e2 > (photopeak1 - thresholdFraction*photopeak1) ) )
      {
	//copy the input event to the output event struct
	fe_filter_out.ts      = fe_filter_in.ts    ;
	fe_filter_out.random  = fe_filter_in.random;  
	fe_filter_out.d       = fe_filter_in.d     ;  
	fe_filter_out.yozRot  = fe_filter_in.yozRot;  
	fe_filter_out.x1      = fe_filter_in.x1    ;  
	fe_filter_out.y1      = fe_filter_in.y1    ;  
	fe_filter_out.z1      = fe_filter_in.z1    ;  
	fe_filter_out.e1      = fe_filter_in.e1    ;  
	fe_filter_out.n1      = fe_filter_in.n1    ;  
	fe_filter_out.x2      = fe_filter_in.x2    ;  
	fe_filter_out.y2      = fe_filter_in.y2    ;  
	fe_filter_out.z2      = fe_filter_in.z2    ;  
	fe_filter_out.e2      = fe_filter_in.e2    ;  
	fe_filter_out.n2      = fe_filter_in.n2    ;  
	fe_filter_out.dt      = fe_filter_in.dt    ;  
	//write the event in the output elm2 file
	output_file.write((char*)&fe_filter_out,sizeof(fe_filter_out)); //store the event in the elm2 file	  
      }
    }
    fclose(fIn);
    
    output_file.close();
    std::cout << "Filtered dataset written to file " << outFileName << std::endl;
    
  }
  
  return 0;
}