// small program to analyze the spectra 
// compile with
// g++ -o studyModule studyModule.cpp `root-config --cflags --glibs`

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
#include <TApplication.h>
#include <iomanip>      // std::setw

#include <TSpectrum.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TH2F.h>
#include <TFile.h>
#include <TH1F.h>
#include <TGraphErrors.h>
#include "TError.h"
#include "TStyle.h"

int main(int argc, char * argv[]) 
{
  if(argc < 7)
  {
    std::cout << "USAGE:\t\t studyModule <spectra_file.root> head xMin xMax yMin yMax" << std::endl;
    std::cout << "spectra_file.root \t input .root file with the crystal spectra" << std::endl;
    std::cout << "head \t\t head where the module you want to analyze sits" << std::endl;
    std::cout << "xMin \t\t x ID of the first row" << std::endl;
    std::cout << "xMax \t\t x ID of the last row" << std::endl;
    std::cout << "yMin \t\t y ID of the first row" << std::endl;
    std::cout << "yMax \t\t y ID of the last row" << std::endl;
    std::cout << "eMin \t\t [OPTIONAL] Minimum energy of the 'good' energy window - Default 507KeV" << std::endl;
    std::cout << "eMax \t\t [OPTIONAL] Maximum energy of the 'good' energy window - Default 515KeV" << std::endl;
    std::cout << std::endl;
    std::cout << "TGraphErrors will be printed in multi.gif" << std::endl;
    std::cout << "and in .root file multi.root" << std::endl;
    std::cout << std::endl;
    return 1;
  }
  else
  {
    gStyle->SetTitleSize(0.09,"t"); //needed for setting title size
    gStyle->SetOptStat(0);
    //parse command line args. we need to do it now because the call to TApplication later will mess up things (who knows why)
    std::vector<std::string> arguments;
    for(int i = 0 ; i < argc ; i++)
    {
      arguments.push_back(argv[i]);
    }
    int Narguments = argc;
    //Calling TApplication is needed because of some problem loading libraries...
    TApplication theApp("App", &argc, argv);
    int userHead = atoi(arguments[2].c_str());
    int userXmin = atoi(arguments[3].c_str());
    int userXmax = atoi(arguments[4].c_str());
    int userYmin = atoi(arguments[5].c_str());
    int userYmax = atoi(arguments[6].c_str());
    double Emin,Emax;
    if(argc > 7)
    {
      Emin = atof(arguments[7].c_str());
      Emax = atof(arguments[8].c_str());
    }
    else 
    {
      Emin = 507.0;
      Emax = 515.0;
    }
    
    //open spectra root file 
    std::cout << "Opening file " << arguments[1].c_str() << std::endl;
    TFile *spectraFile = new TFile(arguments[1].c_str());
    //and get the two th2f of the heads
    std::stringstream headDir[2],histoname[2];
    std::string directories[2][64][48]; 
    for(int head = 0 ; head < 2 ; head++)//we write all the dir names
    {
      for(int xCry = 0; xCry < 64; xCry++) 
      {
	for(int yCry = 0; yCry < 48; yCry++) 
	{
	  std::stringstream sstream;
	  sstream << "head" << head << "/x_" << xCry << "/y_"<< yCry; 
	  //       std::cout << sstream.str() << std::endl;
	  directories[head][xCry][yCry] = sstream.str(); 
	  //       std::cout << directories[userHead][xCry][yCry] << std::endl;
	}
      }
    }
    TCanvas *multi = new TCanvas("MultiCanvas","MultiCanvas",3000,1500);
    multi->Divide(userXmax-userXmin+1,userYmax-userYmin+1);
    
    //plot the 2d peak map of the chosen head and the zoomed map
    spectraFile->cd();
    std::stringstream aString;
    aString << "head" << userHead;
    spectraFile->cd(aString.str().c_str());
    aString.str("");
    aString << "peaks" << userHead;
    TH2F *plot2d = (TH2F*)gDirectory->Get(aString.str().c_str());
    TCanvas *globalMap = new TCanvas((aString.str()+"_full").c_str(),(aString.str()+"_full").c_str(),1200,800);
    globalMap->cd();
    plot2d->GetZaxis()->SetRangeUser(450, 600);
    plot2d->Draw("COLZ");
    //   globalMap->Print((aString.str()+".gif").c_str());
    aString << "head" << userHead << "_x" << userXmin << "-"<< userXmax << "_y"<< userYmin << "-" << userYmax;
    TCanvas *zoomMap = new TCanvas((aString.str()+"_zoom").c_str(),(aString.str()+"_zoom").c_str(),1200,800);
    zoomMap->cd();
    plot2d->GetXaxis()->SetRangeUser(userXmin, userXmax);
    plot2d->GetYaxis()->SetRangeUser(userYmin, userYmax);
    plot2d->Draw("COLZ");
    //   zoomMap->Print((aString.str()+".gif").c_str());
    
    int canvascounter = 0;
    for(int yCry = userYmax; yCry > (userYmin-1); yCry--) 
    {
      for(int xCry = userXmin; xCry < userXmax+1; xCry++) 
      {
	canvascounter++;
	spectraFile->cd();
	spectraFile->cd(directories[userHead][xCry][yCry].c_str());
	TGraphErrors *graph = (TGraphErrors*)gDirectory->Get("Graph");
	
	if(graph!=NULL)
	{
	  // 	std::cout << xCry << " " << yCry << " " << graph->GetN() << std::endl;
	  multi->cd(canvascounter);
	  std::stringstream title;
	  title << "head" << userHead << "_x" << xCry << "_y" << yCry ;
	  // 	graph->SetTitleW(2);
	  // 	graph->SetTitleH(2);
	  graph->SetTitle(title.str().c_str());
	  graph->SetMarkerStyle(21);
	  graph->SetMarkerSize(0.1);
	  graph->GetXaxis()->SetTitle("Time [s]");
	  graph->GetYaxis()->SetTitle("[KeV]");
	  graph->GetXaxis()->SetLabelSize(0.06);
	  graph->GetYaxis()->SetLabelSize(0.06);
	  graph->GetXaxis()->SetNdivisions(505);
	  graph->GetYaxis()->SetNdivisions(505);
	  graph->GetYaxis()->SetTitleSize(0.06);
	  graph->GetYaxis()->SetTitleOffset(1.4);
	  graph->GetXaxis()->SetTitleSize(0.06);
	  graph->GetXaxis()->SetTitleOffset(0.9);
	  graph->GetYaxis()->SetRangeUser(450,600);
	  graph->Draw("AP");
	}
      }
    }
    
    
    // all the graphs should have the same number of points, so check the first one 
    spectraFile->cd();
    spectraFile->cd(directories[0][0][0].c_str());
    TGraphErrors *firstgraph = (TGraphErrors*)gDirectory->Get("Graph");
    int Npoints = firstgraph->GetN();
    int *xvect[2];
    //take the time vector
    for(int i = 0; i < 2 ; i++) 
    {
      xvect[i] = new int[Npoints];
      for(int j = 0 ; j < Npoints ; j++)
      {
	double x,y;
	firstgraph->GetPoint(j,x,y);
	xvect[i][j] = x;
      }
    }
    
    // construct graphs and maps accordingly
    // crystals out of window
    int *outCry[2];
    
    TH2F **outMap[2];
    TCanvas **C_outMap[2];
    
    for(int i = 0; i < 2 ; i++) 
    {
      outCry[i] = new int[Npoints];
      
      outMap[i] = new TH2F*[Npoints];
      C_outMap[i] = new TCanvas*[Npoints];
      for (int j = 0; j < Npoints ; j++)
      {
	std::stringstream mapName;
	if(j==0)
	  mapName << "Head " << i  << " - Channels out of "<< Emin << "-"<< Emax << "Kev - 0 to " << xvect[i][j] << " sec ";
	else
          mapName << "Head " << i  << " - Channels out of "<< Emin << "-"<< Emax << "Kev - " << xvect[i][j-1] <<" to " << xvect[i][j] << " sec ";
	outMap[i][j] = new TH2F(mapName.str().c_str(),mapName.str().c_str(),64,0,64,48,0,48);
	mapName.str();
	if(j==0)
	  mapName << "C_Head " << i  << " - Channels out of "<< Emin << "-"<< Emax << "Kev - 0 to " << xvect[i][j] << " sec ";
	else
          mapName << "C_Head " << i  << " - Channels out of "<< Emin << "-"<< Emax << "Kev - " << xvect[i][j-1] <<" to " << xvect[i][j] << " sec ";
	C_outMap[i][j] = new TCanvas(mapName.str().c_str(),mapName.str().c_str(),1200,800);
// 	xvect[i][j] = 0;
	outCry[i][j] = 0;
      }
    }
    
    
    
    for(int head = 0; head < 2; head++)
    {
      for(int xCry = 0; xCry < 64; xCry++) 
      {
	for(int yCry = 0; yCry < 48; yCry++)
	{
	  //feedback to the user 
	  std::cout << "Head " << std::setw(2) <<  head << "/1" << "\t" << "x " << std::setw(2) << xCry << "/63" << "\t\t" << "y " << std::setw(2) << yCry << "/47"/*<< "\t" << " Subplot " << k*/;
	  std::cout << "\r";
	  spectraFile->cd();
	  spectraFile->cd(directories[head][xCry][yCry].c_str());
	  TGraphErrors *graph = (TGraphErrors*)gDirectory->Get("Graph");
	  if(graph!=NULL)
	  {
	    for(int t = 0 ; t < graph->GetN() ; t++)
	    {
	      double x,y;
	      graph->GetPoint(t,x,y);
	      if(y > 0 && (y < Emin | y > Emax))
	      {
		outMap[head][t]->Fill(xCry,yCry,1);
		outCry[head][t]++;
// 		xvect[head][t] = x;
	      }
	      else
	      {
		outMap[head][t]->Fill(xCry,yCry,0);
	      }
	    }
	  }
	}
      }
    }
    
    for(int i = 0; i < 2 ; i++) 
    {
      for (int j = 0; j < Npoints ; j++)
      {
	std::stringstream gifName;
	gifName << "Head" << i << "_t" << j<< ".gif";
	C_outMap[i][j]->cd();
	outMap[i][j]->GetXaxis()->SetTitle("X");
	outMap[i][j]->GetYaxis()->SetTitle("Y");
	outMap[i][j]->Draw("COLZ");
// 	C_outMap[i][j]->Print(gifName.str().c_str());
      }
    }
    
    TGraph *inTime[2];
    TCanvas *C_inTime[2];
    for(int head = 0; head < 2; head++)
    {
      std::stringstream title;
      title << "Head" << head << " - Num. of crystals out of " << Emin  << " - " << Emax << "KeV";
      C_inTime[head] = new TCanvas(title.str().c_str(),title.str().c_str(),1200,800);
      inTime[head] = new TGraph(Npoints,&xvect[head][0],&outCry[head][0]);
      inTime[head]->SetTitle(title.str().c_str());
      inTime[head]->GetXaxis()->SetTitle("Time [s]");
      inTime[head]->GetYaxis()->SetTitle("N");
      inTime[head]->SetMarkerStyle(21);
      C_inTime[head]->cd();
      inTime[head]->Draw("AP");
    }
    
    std::stringstream title;
    title << "module_head" << userHead << "_x" << userXmin << "-"<< userXmax << "_y"<< userYmin << "-" << userYmax ;
    
    //   multi->Print((title.str()+".gif").c_str());
    TFile *multiFile = new TFile((title.str()+".root").c_str(),"RECREATE");
    multiFile->cd();
    plot2d->GetXaxis()->SetRangeUser(0, 64);
    plot2d->GetYaxis()->SetRangeUser(0, 48);
    globalMap->Write();
    plot2d->GetXaxis()->SetRangeUser(userXmin, userXmax);
    plot2d->GetYaxis()->SetRangeUser(userYmin, userYmax);
    zoomMap->Write();
    C_inTime[0]->Write();
    C_inTime[1]->Write();
    inTime[0]->Write();
    inTime[1]->Write();
    for(int i = 0; i < 2 ; i++) 
    {
      for (int j = 0; j < Npoints ; j++)
      {
	C_outMap[i][j]->Write();
      }
    }
    multi->Write();
    multiFile->Close();
    
    return 0;
  }
}