// small program 

// compile with
// g++ -o calibrationVsTime calibrationVsTime.cpp `root-config --cflags --glibs`

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
#include <TGraph.h>
#include <TGraphErrors.h>
#include "TError.h"
#include <assert.h>
// temporary - it should be done in a text file

// const int Nfiles = 10;
// int daysFromCalibration[] = 
// {
//   4,
//   19,
//   29,
//   38,
//   48,
//   104,
//   111,
//   114,
//   116,
//   121,
//   146
//   
//   //   17,
//   //   43,
//   //   57,
//   //   58,
//   //   65,
//   //   72,
//   //   78,
//   //   137,
//   //   137,
//   //   186
// };
// std::string dataFiles[] = 
// {
//   "/media/marco/Elements/Home/marco/Universita/Cern/ClearPEM/Monza/Normalizations/2015-02-24/spectra_data.root",
//   "/home/marco/data_db/pemNormalization/listMode/209/2015-03-21/spectra_stdin.root",
//   "/media/marco/Elements/ClearPEM/PeakTest/00359/pem/spectra_stdin.root",
//   "/media/marco/Elements/ClearPEM/PeakTest/00365/pem/spectra_stdin.root",
//   "/media/marco/Elements/ClearPEM/PeakTest/00366/pem/spectra_stdin.root",
//   "/media/marco/Elements/ClearPEM/PeakTest/00379/pem/spectra_stdin.root",
//   "/media/marco/Elements/ClearPEM/PeakTest/00398/pem/spectra_stdin.root",
//   "/home/marco/data_db/exam/00205/00401/pem/spectra_stdin.root",
//   "/home/marco/data_db/exam/00207/00405/pem/spectra_stdin.root",
//   "/home/marco/data_db/exam/00209/00415/pem/spectra_stdin.root",
//   "/home/marco/data_db/exam/00210/00446/pem/spectra_data.root"
//   
//   //   "/media/My Book/PEM/HSG/pemNormalization/listMode/209/2014-06-05/spectra_stdin.root",
//   //   "/media/My Book/PEM/HSG/pemNormalization/listMode/209/2014-07-01/spectra_stdin.root",
//   //   "/media/My Book/PEM/HSG/pemNormalization/listMode/209/2014-07-15/spectra_stdin.root",
//   //   "/share/clearpem/data_db/exam/00189/00266/pem/spectra_stdin.root",
//   //   "/media/My Book/PEM/HSG/exam/00191/00305/pem/spectra_stdin.root",
//   //   "/home/clearpem/data_db/exam/00191/00313/pem/spectra_stdin.root",
//   //   "/share/clearpem/data_db/exam/00192/00317/pem/spectra_stdin.root",
//   //   "/home/clearpem/data_db/exam/00193/00320/pem/spectra_stdin.root",
//   //   "/media/My Book/PEM/HSG/exam/00194/00321/pem/spectra_stdin.root",
//   //   "/share/clearpem/data_db/exam/00196/00327/pem/spectra_stdin.root"
// };





int main(int argc, char * argv[]) 
{
  
  if(argc < 2)
  {
    std::cout << "USAGE:\t\t calibrationVsTime <data.txt>" << std::endl;
    std::cout << "data.txt \t file with time from calibration and filename for each of the point desired in the calibrationVsTime plots" << std::endl;
    std::cout << std::endl;
    std::cout << "data.txt structure has to be like" << std::endl;
    std::cout << "<days from calibration> <spectra root file 1>" << std::endl;
    std::cout << "<days from calibration> <spectra root file 2>..." << std::endl;
    std::cout << std::endl;
    std::cout << "Results will be stored in a root file with name calibrationVsTime<data>.root" << std::endl;
    std::cout << std::endl;
    return 1;
  }
  else
  {
    std::string dataFileName = argv[1];
    
    
    
    
    //Calling TApplication is needed because of some problem loading libraries
    TApplication theApp("App", &argc, argv);
    
    //open and read data.txt file
    std::string nome = dataFileName.substr(0, dataFileName.length() - 4);    
    ifstream stream;
    stream.open(dataFileName.c_str());
    int day_t;
    std::string file_t;
    std::vector<int> daysFromCalibration;
    std::vector<std::string> dataFiles;
    while(!stream.eof())
    {
      stream >> day_t >> file_t;
      daysFromCalibration.push_back(day_t);
      dataFiles.push_back(file_t);
    }
    stream.close();	
    int Nfiles = daysFromCalibration.size();
    
    // fill the graphs
    std::vector<double> xPeaksRMS[2],yPeaksRMS[2],yMeanDistro[2],yOutCount[2];
    TGraph *RMSvsTime[2];
    TGraph *MeanvsTime[2];
    TGraph *OutvsTime[2];
    
    TGraph *PeakPerCrystalVsTime[2][64][48];
    for(int head = 0 ; head < 2 ; head++)
    {
      for(int xCry = 0; xCry < 64; xCry++) 
      {
	for(int yCry = 0; yCry < 48; yCry++) 
	{
	  PeakPerCrystalVsTime[head][xCry][yCry] = new TGraph(Nfiles);
	}
      }
    }
    
    for(int i = 0 ; i < Nfiles ; i++)
    {
      // number of crystals out of 507-515 KeV range
      int crystalsOut[2] = {0,0};
      //open this spectra root file 
      TFile *spectraFile = new TFile(dataFiles[i].c_str());
      //write the folder and spectra names
      std::stringstream headDir[2],peaks_distro_name[2],peaks_name[2];
      for(int head = 0 ; head < 2 ; head++)
      {
	xPeaksRMS[head].push_back(daysFromCalibration[i]);
	headDir[head] << "head" << head;
	peaks_distro_name[head] << "peaks_distro" << head;  // distribution of peak positions
	peaks_name[head] << "peaks" << head; 
      }
      
      // take the spectra from root file, fill the tgraphs
      TH1F *peaks_distro[2];
      TH2F *peaks[2];
      
      for(int head = 0 ; head < 2 ; head++)
      {
	spectraFile->cd();
	spectraFile->cd(headDir[head].str().c_str());
	peaks_distro[head] = (TH1F*)gDirectory->Get(peaks_distro_name[head].str().c_str());
	peaks[head] = (TH2F*)gDirectory->Get(peaks_name[head].str().c_str());
	// write the RMS in the variables
	yPeaksRMS[head].push_back(peaks_distro[head]->GetRMS());
	yMeanDistro[head].push_back(peaks_distro[head]->GetMean());
	//
	for(int xCry = 0; xCry < 64; xCry++) 
	{
	  for(int yCry = 0; yCry < 48; yCry++) 
	  {
	    PeakPerCrystalVsTime[head][xCry][yCry]->SetPoint(i,daysFromCalibration[i],peaks[head]->GetBinContent(xCry + 1 , yCry + 1));
	    std::stringstream title;
	    title << "511KeV Peak position vs. Time - Head" << head << "_x" << xCry << "_y" << yCry ;
	    PeakPerCrystalVsTime[head][xCry][yCry]->SetTitle(title.str().c_str());
	    PeakPerCrystalVsTime[head][xCry][yCry]->GetXaxis()->SetTitle("Days from Calibration");
	    PeakPerCrystalVsTime[head][xCry][yCry]->GetYaxis()->SetTitle("[KeV]");
	    PeakPerCrystalVsTime[head][xCry][yCry]->GetYaxis()->SetRangeUser(460,600);
	    PeakPerCrystalVsTime[head][xCry][yCry]->Draw("A*");
	    if( (peaks[head]->GetBinContent(xCry + 1 , yCry + 1) < 507.0) | (peaks[head]->GetBinContent(xCry + 1 , yCry + 1) > 515.0) )
	      crystalsOut[head]++;
	    
	  }
	}
	yOutCount[head].push_back(crystalsOut[head]);
      }
      
      
    }
    
    for(int head = 0 ; head < 2 ; head++)  
    {
      
      //RMS vs Time 
      RMSvsTime[head] = new TGraph(Nfiles,&xPeaksRMS[head].at(0),&yPeaksRMS[head].at(0));
      std::stringstream title;
      title << "RMS of 511KeV distro vs. Time - Head" << head ;
      RMSvsTime[head]->SetTitle(title.str().c_str());
      RMSvsTime[head]->GetXaxis()->SetTitle("Days from Calibration");
      RMSvsTime[head]->GetYaxis()->SetTitle("[KeV]");
      RMSvsTime[head]->Draw("A*");
      
      //Mean position distro
      MeanvsTime[head] = new TGraph(Nfiles,&xPeaksRMS[head].at(0),&yMeanDistro[head].at(0));
      //std::stringstream title;
      title.str("");
      title << "Mean of 511KeV distro vs. Time - Head" << head ;
      MeanvsTime[head]->SetTitle(title.str().c_str());
      MeanvsTime[head]->GetXaxis()->SetTitle("Days from Calibration");
      MeanvsTime[head]->GetYaxis()->SetTitle("[KeV]");
      MeanvsTime[head]->Draw("A*");
      
      //Mean position distro
      OutvsTime[head] = new TGraph(Nfiles,&xPeaksRMS[head].at(0),&yOutCount[head].at(0));
      //std::stringstream title;
      title.str("");
      title << "N of Crystals out of 507-515 window vs. Time - Head" << head ;
      OutvsTime[head]->SetTitle(title.str().c_str());
      OutvsTime[head]->GetXaxis()->SetTitle("Days from Calibration");
      OutvsTime[head]->GetYaxis()->SetTitle("N");
      OutvsTime[head]->Draw("A*");
    }
    
    
    
    
    std::string fileCalibrationVsTimeOutput = "calibrationVsTime_" + nome + ".root"; 
    std::cout << "Saving spectra to root file " << fileCalibrationVsTimeOutput << std::endl;
    TFile *f = new TFile(fileCalibrationVsTimeOutput.c_str(), "RECREATE");
    
    f->cd();
    TDirectory *directory[2][65][49]; //TDirectory 
    for(int head = 0; head < 2; head++)  // save data to root file
    {
      char head_name[256];
      sprintf(head_name, "head%d", head);
      directory[head][0][0] = f->mkdir(head_name);
      directory[head][0][0]->cd();
      
      RMSvsTime[head]->Write();
      MeanvsTime[head]->Write();
      OutvsTime[head]->Write();
      //     peaks_distro[head]->Write();
      
      for(int xCry = 0; xCry < 64; xCry++) 
      {
	char x_name[256];
	sprintf(x_name, "x_%d", xCry);
	directory[head][xCry+1][0] = directory[head][0][0]->mkdir(x_name);
	directory[head][xCry+1][0]->cd();
	for(int yCry = 0; yCry < 48; yCry++) 
	{
	  char y_name[256];
	  sprintf(y_name, "y_%d", yCry);
	  directory[head][xCry+1][yCry+1] = directory[head][xCry+1][0]->mkdir(y_name);
	  directory[head][xCry+1][yCry+1]->cd();
	  
	  // 	spectra[head][xCry][yCry]->Write();
	  PeakPerCrystalVsTime[head][xCry][yCry]->Write();
	}
      }
    }
    f->Close();
    
    
    
    return 0;
  }
}
