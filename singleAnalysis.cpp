
// compile with
// g++ -o singleAnalysis singleAnalysis.cpp `root-config --cflags --glibs` -lSpectrum




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
#include <TApplication.h>
#include "TError.h"
#include "TTree.h"

#define LOWER_ENERGY_ACCEPTED_STRICT 507
#define HIGHER_ENERGY_ACCEPTED_STRICT 515
#define LOWER_ENERGY_ACCEPTED_RELAXED 501
#define HIGHER_ENERGY_ACCEPTED_RELAXED 521
#define LOW_STATISTICS_CUTOFF 2000



int main(int argc, char * argv[]) 
{
  std::string InputFileName;
  std::string OutputFileName;
  
  if(argc < 2)
  {
    std::cout << "USAGE:\t\t singleAnalysis inputFile.root [output.root]" << std::endl;
    std::cout << std::endl;
    std::cout << "inputFile.root \t input .root file with the crystal spectra" << std::endl;
    std::cout << "head \t\t [OPTIONAL] name of the output file (otherwise it will be output.root)" << std::endl;
    std::cout << std::endl;
    return 1;
  }
  else 
  {
    if(argc < 3)
    {
      InputFileName = argv[1];
      OutputFileName = "output.root";
    }
    else 
    {
      InputFileName = argv[1];
      OutputFileName = argv[2];
    }
    
    TApplication theApp("App", &argc, argv); //Calling TApplication is needed because of some problem loading libraries...
    
    //open INPUT root file   
    std::cout << "Opening file " << InputFileName << std::endl;
    TFile *spectraFile = new TFile(InputFileName.c_str());
    TTree *data = (TTree*)spectraFile->Get("data");
    
    //prepare OUTPUT root file
    TFile *f = new TFile(OutputFileName.c_str(), "RECREATE");
    
    //input variables
    Float_t crystal;
    Float_t ts;
    Float_t energy;
    Float_t topEnergy;
    Float_t bottomEnergy;
    //input branches
    TBranch *b_crystal;
    TBranch *b_ts;
    TBranch *b_energy;
    TBranch *b_topEnergy;
    TBranch *b_bottomEnergy;
    //set branches
    data->SetBranchAddress("crystal"     , &crystal     , &b_crystal);
    data->SetBranchAddress("ts"          , &ts          , &b_ts);
    data->SetBranchAddress("energy"      , &energy      , &b_energy);
    data->SetBranchAddress("topEnergy"   , &topEnergy   , &b_topEnergy);
    data->SetBranchAddress("bottomEnergy", &bottomEnergy, &b_bottomEnergy);
    
    // Read crystal map - adapted from doReport.C
    Int_t crystalX [6144];
    Int_t crystalY [6144];
    Int_t crystalHead[6144];
    ifstream cmap ("crystal_map.txt");
    while(!cmap.eof()) 
    {
      int cid, x, y;
      cmap >> cid >> x >> y; 
      
      if(cid >= 6144/2)
	crystalHead[cid] = 1;
      else
	crystalHead[cid] = 0;
      crystalX[cid] = x;
      crystalY[cid] = y;
    }
    //-----------------------------------------
    int nSlices = 10;
    float sliceLength = 60e12;
    
    long long int nevent = data->GetEntries();
    TH1F *spectra[2][64][48];   
    TH2F *spectraVsTime[2][64][48];
    TH1F *spectraTime[2][64][48][nSlices];   
    TGraph *mean[2][64][48];
    
    for(int head = 0; head < 2; head++) for(int xCry = 0; xCry < 64; xCry++) for(int yCry = 0; yCry < 48; yCry++) 
    {
      char name[256],nameGraph[256],nameTime[256],nameTimeOriginal[256];
      
      sprintf(name, "spectrum_head%d_x%d_y%d", head , xCry , yCry);
      spectra[head][xCry][yCry] = new TH1F(name, name, 500, 0 , 1500);
      spectra[head][xCry][yCry]->GetXaxis()->SetTitle("Energy");
      spectra[head][xCry][yCry]->GetYaxis()->SetTitle("Counts");
      
      sprintf(nameTime, "spectrumVsTime_head%d_x%d_y%d", head , xCry , yCry);
      spectraVsTime[head][xCry][yCry] = new TH2F(nameTime, nameTime,10,0,700e12, 500, 0 , 1500);
      spectraVsTime[head][xCry][yCry]->GetXaxis()->SetTitle("Time");
      spectraVsTime[head][xCry][yCry]->GetYaxis()->SetTitle("Energy");
      
      sprintf(nameGraph, "graph_head%d_x%d_y%d", head , xCry , yCry);
      
      
      
      for(int slice = 0; slice < nSlices ; slice++)
      {
	char nameSlice[256];
	sprintf(nameSlice, "spectrum_time%d_head%d_x%d_y%d",slice, head , xCry , yCry);
	spectraTime[head][xCry][yCry][slice] = new TH1F(nameSlice, nameSlice, 500, 0 , 1500);
        spectraTime[head][xCry][yCry][slice]->GetXaxis()->SetTitle("Energy");
        spectraTime[head][xCry][yCry][slice]->GetYaxis()->SetTitle("Counts");
	
	
      }
      //       sprintf(nameDelta, "deltaTime_head%d_x%d_y%d", head , xCry , yCry);
      //       deltaTime[head][xCry][yCry] = new TH1F(nameDelta, nameDelta, 500, -100e-9 , 100e-9);
      //       deltaTime[head][xCry][yCry]->GetXaxis()->SetTitle("ns");
      //       deltaTime[head][xCry][yCry]->GetYaxis()->SetTitle("N");
      //       promptMap[head][xCry][yCry] = 0;
      //       randomMap[head][xCry][yCry] = 0;
      //       sprintf(nameTime, "spectrumTime_head%d_x%d_y%d", head , xCry , yCry);
      //       spectraTime[head][xCry][yCry] = new TH2F(nameTime, nameTime, totalTime/singleFrameTime,0,totalTime,100,0,1000);
      //       spectraTime[head][xCry][yCry]->GetXaxis()->SetTitle("Seconds");
      //       spectraTime[head][xCry][yCry]->GetYaxis()->SetTitle("KeV");
    }
    
    for (int i=0 ; i< nevent; i++) //loop on all the entries of ttree
    { 
      data->GetEvent(i);              //read event 
      
      int cry = (int) crystal;
      int head = crystalHead[cry];
      int xCry = crystalX[cry];
      int yCry = crystalY[cry];		
      
      spectra[head][xCry][yCry]->Fill(energy);
      spectraVsTime[head][xCry][yCry]->Fill(ts,energy);
      int slice = (int) (ts / 70e12);
      spectraTime[head][xCry][yCry][slice]->Fill(energy);
      
      
      if( (i % 10000) == 0 )
      {
	std::cout << "\r";
	std::cout << "Events " <<  i << " / " << nevent;      
      }
    }
    
    for(int head = 0; head < 2; head++) for(int xCry = 0; xCry < 64; xCry++) for(int yCry = 0; yCry < 48; yCry++) 
    {
      std::vector<double> x,y;
      for(int slice = 0; slice < nSlices ; slice++)
      {
	x.push_back(slice);
	y.push_back(spectraTime[head][xCry][yCry][slice]->GetMean());
      }
      mean[head][xCry][yCry] = new TGraph(nSlices,&x[0],&y[0]);
    }
    
    
    //prepare canvases
//     TCanvas *c_spectra[2][64][48];   
//     TCanvas *c_spectraVsTime[2][64][48];
//     for(int head = 0; head < 2; head++) for(int xCry = 0; xCry < 64; xCry++) for(int yCry = 0; yCry < 48; yCry++) 
//     {
//       char name[256],nameTime[256];
//       sprintf(name, "spectrum_head%d_x%d_y%d", head , xCry , yCry);
//       sprintf(nameTime, "spectrumVsTime_head%d_x%d_y%d", head , xCry , yCry);
//       
//       c_spectra[head][xCry][yCry] = new TCanvas(name,name,1200,800);
//       c_spectraVsTime[head][xCry][yCry] = new TCanvas(nameTime,nameTime,1200,800);
//     }
//      //draw on canvases
//     for(int head = 0; head < 2; head++) for(int xCry = 0; xCry < 64; xCry++) for(int yCry = 0; yCry < 48; yCry++) 
//     {
//       c_spectra[head][xCry][yCry]->cd();
//       spectra[head][xCry][yCry]->Draw();
//       c_spectraVsTime[head][xCry][yCry]->cd();
//       spectraVsTime[head][xCry][yCry]->Draw("COLZ");
//     }
    
   
    
    
    std::cout << std::endl;
    //---- save results to output root file
    std::cout << "Saving spectra to root file " << OutputFileName << std::endl;
    f->cd();
    TDirectory *directory[2][65][49][2]; //TDirectory 
    for(int head = 0; head < 2; head++)  // save data to root file
    {
      char head_name[256];
      sprintf(head_name, "head%d", head);
      directory[head][0][0][0] = f->mkdir(head_name);
      directory[head][0][0][0]->cd();
//       C_peak[head]->Write();
//       C_fwhm[head]->Write();
//       C_peak_fwhm[head]->Write();
//       C_scatter[head]->Write();
//       C_random[head]->Write();
//       C_randomCounts[head]->Write();
//       C_promptCounts[head]->Write();
//       C_deltaT[head]->Write();
//       peaks[head]->Write();
//       fwhm[head]->Write();
//       peak_fwhm[head]->Write();
//       scatter[head]->Write();
//       random[head]->Write();
//       randomCounts[head]->Write();
//       promptCounts[head]->Write();
//       deltaT[head]->Write();
//       peaks_distro[head]->Write();
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
	  spectraVsTime[head][xCry][yCry]->Write();
	  mean[head][xCry][yCry]->Write();
	  for(int slice = 0; slice < nSlices ; slice++)
          {
	    spectraTime[head][xCry][yCry][slice]->Write();
          }
// 	  spectraTime[head][xCry][yCry]->Write();
// 	  deltaTime[head][xCry][yCry]->Write();
// 	  if(corrX_peakVsTime[head][xCry][yCry].size() != 0)
// 	  {
// 	    corrC_peakVsTime[head][xCry][yCry]->Write();
// 	    corrpeakVsTime[head][xCry][yCry]->Write();
// 	  }
// 	  directory[head][xCry+1][yCry+1][1] = directory[head][xCry+1][yCry+1][0]->mkdir("SplitSpectrumVsTime");
// 	  directory[head][xCry+1][yCry+1][1]->cd();
// 	  for(int i = 0 ; i < xTimeI.size()-1 ; i++)
// 	  {
// 	    spectraSplitTimeWeighted[head][xCry][yCry][i]->Write();
// 	  }
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