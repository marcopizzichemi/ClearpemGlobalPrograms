// compile with
// g++ -o ../build/compareMap compareMap.cpp `root-config --cflags --glibs`

// run with

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
#include <TError.h>
#include <TStyle.h>

int main(int argc, char * argv[]) 
{
  gStyle->SetOptStat(0);
  if(argc < 3)
  {
    std::cout << "USAGE:\t\t compareMap file1.root file2.root [file3.root] " << std::endl;
    std::cout << std::endl;
    return 1;
  }
  
  int N = argc - 1;
  std::string* arguments = new std::string[argc];
  for(int i = 0 ; i < argc ; i++)
  {
    arguments[i] = argv[i];
  }
  
  TApplication theApp("App", &argc, argv);
  
  //take the n of files, create the input tfiles and th2f
  TFile** inputFile = new TFile*[N];
  TH2F*** inputMap = new TH2F**[N];
  TH2F*** comparisonMap = new TH2F**[N-1];
  TCanvas*** canvas = new TCanvas**[N-1];
  for(int i = 0 ; i < N ; i++)
  {
    inputMap[i] = new TH2F*[2];
    comparisonMap[i] = new TH2F*[2];
    canvas[i] = new TCanvas*[2];
  }
  
  //open input files
  for(int i = 0 ; i < N ; i++)
  {
    inputFile[i] = new TFile(arguments[i+1].c_str(),"READ");
  }
  
  //take the th2fs 
  for(int i = 0 ; i < N ; i++)
  {
    inputFile[i]->cd();
    inputFile[i]->cd("head0");
    inputMap[i][0] = (TH2F*) ((TCanvas*) gDirectory->Get("Peaks0"))->GetPrimitive("peaks0");
    inputFile[i]->cd();
    inputFile[i]->cd("head1");
    inputMap[i][1] = (TH2F*) ((TCanvas*) gDirectory->Get("Peaks1"))->GetPrimitive("peaks1");
  }
  
  for(int i = 0 ; i < N-1 ; i++)
  {
    //run on two input maps, produce the comparison map 
    for(int j = 0 ; j < 2 ; j++)
    {
      std::stringstream sname;
      sname << "Head_" << j << " , (file" << i+2 << "-file" << i+1 << ")/(file" << i+1 << ")";
      
      comparisonMap[i][j] = new TH2F(sname.str().c_str(),sname.str().c_str(),64,0,64,48,0,48);
      comparisonMap[i][j]->GetZaxis()->SetRangeUser(-0.05,0.05); 
      
      sname.str("");
      sname << "C_Head_" << j << " , (file" << i+2 << "-file" << i+1 << ")/(file" << i+1 << ")";
      canvas[i][j] = new TCanvas(sname.str().c_str(),sname.str().c_str(),1200,800);
      
      for(int cx = 0 ; cx < 64 ; cx++)
      {
	for(int cy = 0 ; cy < 48 ; cy++)
	{
	  float c1 = inputMap[i][j]->GetBinContent(cx+1,cy+1);
	  float c2 = inputMap[i+1][j]->GetBinContent(cx+1,cy+1);
	  if(c1 != 0) comparisonMap[i][j]->Fill(cx,cy,(c2-c1)/c1);
	}
      }
      canvas[i][j]->cd();
      comparisonMap[i][j]->Draw("COLZ");
    }
  }
    

  TFile *f = new TFile("outputCompareMap.root","RECREATE");
  f->cd();
  for(int i = 0 ; i < N-1 ; i++)
  {
    for(int j = 0 ; j < 2 ; j++)
    {
      canvas[i][j]->Write();
      comparisonMap[i][j]->Write();
    }
  }
  std::cout << "aaaaaaaa" << std::endl;
//   f->Close();
  return 0;
}