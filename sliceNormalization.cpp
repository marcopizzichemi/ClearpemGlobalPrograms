#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <getopt.h>
//#include <boost/lexical_cast.hpp>
#include <iostream>
#include <set>
#include <assert.h>
#include <sstream>
#include <string>

#include <TCanvas.h>
#include <TH2F.h>
#include <TFile.h>
#include <TH1F.h>
#include <TGraphErrors.h>

// ****** Compilation command *************************************************
// g++ -o sliceNormalization sliceNormalization.cpp `root-config --cflags --glibs`
// ****************************************************************************


static float eMin = 400;		//minimum energy to accept event, in KeV
static float eMax = 650;		//minimum energy to accept event, in KeV
static float dMax = 4E-9;		//maximum time difference (dt) between events in detector 1 and 2 to accept event, in seconds
static int   maxHits = 4;		//not used here
static const int nCols = 6;		//not used here

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
  if(argc < 2)
  {
    std::cout << "USAGE:\t\t sliceNormalization <N> <file.elm2>" << std::endl;
    std::cout << "N \t\t Number of seconds per slice" << std::endl;
    std::cout << "file.elm2 \t input .elm2 file" << std::endl;
    std::cout << std::endl;
    std::cout << "WARNING: output (besides some .gif file...) will be stored in files output.root and projections.root" << std::endl;
    return 1;
  }
  else
  {
    //initialize counters
    long long nRandoms = 0;
    long long nPrompts = 0;
    long long nBad = 0;
    long long nEvents = 0;
    
    //define dimensions of the detector
    float xSize = 171.5 + 2;
    float ySize =  150.2 + 2;
    float pxSize = 0.2;
    int nx = (xSize)/pxSize;
    int lx = nx * pxSize;
    int ny = (ySize)/pxSize;
    int ly = ny * pxSize;
    
    //root declarations (file and histograms where the data analyzed will be stored)
    TFile *f = new TFile("output.root", "RECREATE");
    TFile *fProjections = new TFile("projections.root","recreate");
    TH2F *h = new TH2F("projection", "projection", nx, -lx/2, lx/2, ny, -ly/2, ly/2);
    TH2F *hr = new TH2F("rprojection", "rprojection", nx, -lx/2, lx/2, ny, -ly/2, ly/2);
    TH2F *h1 = new TH2F("head0", "head 0", nx, -lx/2, lx/2, ny, -ly/2, ly/2);
    TH2F *h2 = new TH2F("head1", "head 1", nx, -lx/2, lx/2, ny, -ly/2, ly/2);    
    TH1F *dt = new TH1F("time", "Time Spectrum", 1000, -100E-9, 100E-9);
    TH1F *e = new TH1F("energy", "Energy Spectrum", 1536, 0, 1536);
    TH1F *interval= new TH1F("interval", "Time Interval", 1000000, 0, 1E-3);
    TH1F *rdt = new TH1F("rtime", "Delayed Time Spectrum", 200, -20E-9, 20E-9);
    TH1F *re = new TH1F("renergy", "Delayed Energy Spectrum", 1536, 0, 1536);
    TH1F *rinterval= new TH1F("rinterval", "Delayed Time Interval", 1000000, 0, 1E-3);
    TH1F *nhits = new TH1F("nhits", "nHits", 4, 1, 4);
    TH1F *rnhits = new TH1F("rnhits", "Delayed nHits", 4, 1, 4);
    TH2F *energy2 = new TH2F("e1_vs_e2", "E1 vs E2", 1536, 0, 1536, 1536, 0, 1536);
    TH2F *energyTime = new TH2F("energy_time", "Energy vs Time", 3600, 0, 3600, 1536, 0, 1536);
    TH1F *doi [2][4][2];
    for(int head = 0; head < 2; head++) for(int sm = 0; sm < 4; sm++) for(int ab = 0; ab < 2; ab++) {
      char name[256];		
      sprintf(name, "doi_sm%d%c", 1 + head + 2*sm, 'A' + ab);
      doi[head][sm][ab] = new TH1F(name, name, 4000, -500, 500);
    }
    
    
    double timeSliceLength = atof(argv[1]);				//in seconds
    //creates an array of TH2F
    TH2F *projectionSlice[100];  				//terrible implementation
    TGraphErrors *yGraph,*xGraph;				//Graphs of the average x and y of each slide
    std::vector<double> sliceTime,xMean,yMean,xMeanError,yMeanError;
    TCanvas *cProjectionSlice[100];
    int projectionCounter = 0;
    double TimeFromZero = 0;				//delta time of each event from first event
    double timeTarget = 0;
    bool timeFlag = true;
    //inizialize time variables
    double t0 = -1;
    double tMax = -INFINITY;
    double tMin = INFINITY;
    float smX = xSize / 4;
    float smY = ySize / 2;
    
    
    //open for reading the input files
    for(int i = 2; i < argc; i++) 
    {
      FILE * fIn = fopen(argv[i], "r");	
      if (fIn == NULL) 
      {
	fprintf(stderr, "File %s does not exist\n", argv[i]);
	return 1;
      }
      printf("Reading %s\n", argv[i]);
      
      double lastp = -1;
      double lastr = -1;
      
      //create the event struct fe
      EventFormat fe;
      
      //readout loop (for each input file)
      while(/*nEvents < 1000000 &&*/ fread((void*)&fe, sizeof(fe), 1, fIn) == 1) 
      {
	nEvents++; //counts the events (lines in the binary input file)
	
	//outputs the events counted
	if(nEvents % 10000 == 0) { 
	  printf("%lld events\r", nEvents); 
	  fflush(stdout);
	}
	
	//looks for t0, tMin e tMax, updating at each loop. 
	if(t0 == -1) t0 = fe.ts; 		//t0 is effectively calculated only at the first cycle
	
	if(fe.ts < tMin) tMin = fe.ts;
	if(fe.ts > tMax) tMax = fe.ts;
	
	//slice histograms
	TimeFromZero = fe.ts - t0;    //this will hold the time from 0
	if(TimeFromZero >= timeTarget) {
	  if (TimeFromZero !=0) {
	    std::stringstream sCtitle,sFile;
	    sCtitle << "projection_T" <<  projectionCounter*timeSliceLength << "_T" << (projectionCounter+1)*timeSliceLength;
	    std::string Ctitle = sCtitle.str();
	    std::string FileTitle = Ctitle + ".gif";
	    cProjectionSlice[projectionCounter] = new TCanvas(Ctitle.c_str(),Ctitle.c_str(),1200,800);
	    projectionSlice[projectionCounter]->Draw("COLZ");
	    fProjections->cd();
	    cProjectionSlice[projectionCounter]->Write();
	    projectionSlice[projectionCounter]->Write();
	    cProjectionSlice[projectionCounter]->Print(FileTitle.c_str());
	    
	    sliceTime.push_back(projectionCounter*timeSliceLength);
	    xMean.push_back(projectionSlice[projectionCounter]->GetMean(1));
	    xMeanError.push_back(projectionSlice[projectionCounter]->GetMeanError(1));
	    yMean.push_back(projectionSlice[projectionCounter]->GetMean(2));
	    yMeanError.push_back(projectionSlice[projectionCounter]->GetMeanError(2));
	    
	    
	    projectionCounter++;
	  }
	  //create the new histo
	  std::stringstream stitle;
	  stitle << "projection_T" <<  projectionCounter*timeSliceLength << "_T" << (projectionCounter+1)*timeSliceLength;
	  std::string title = stitle.str();
	  /*cProjectionSlice[projectionCounter] = new TCanvas(title.c_str(),title.c_str(),1200,800);*/	
	  projectionSlice[projectionCounter] = new TH2F(title.c_str(),title.c_str(), nx, -lx/2, lx/2, ny, -ly/2, ly/2);
	  //set the new timeTarget
	  timeTarget += timeSliceLength;
	}
	
	
	
	
	//calculates the coordinate u,v,w (distance on x,y,z for events in detector 1 and 2)
	float u = fe.x1 - fe.x2;
	float v = fe.y1 - fe.y2;
	float w = fe.z1 - fe.z2;
	
	//check if the distance in z is less of the distance between detectors in this run, which of course shouldn't be. If it is, 
	//set the event as "insane" and skip the rest of the actions in this cycle
	if(fabs(w) < fe.d/2.0) {
	  nBad++;
	  continue;
	}
	
	//invert dt if z1 is positive (not really sure why it's needed)
	if (fe.z1 > 0)
	  fe.dt *= -1;
	
	//check if the "flags" n1 and n2 are not 1. If at least one of them is not 1, skips this event.
	//n1 and n2 are already written in the binary and they are the number of hits on detector 1 and 2 for this event
	//n1 = 1 means gamma leaving energy only in one crystal
	if(fe.n1 != 1 || fe.n2 != 1)
	  continue;
	
	//set the "cuts" to accept or reject the event in writing the histograms. 
	bool eOK = (fe.e1 >= eMin) && (fe.e1 <= eMax) && (fe.e2 >= eMin) && (fe.e2 <= eMax);		//energy deposited in both detectors has to be between 400 and 650
	bool tOK = fabs(fe.dt) < dMax;									//time difference between event in det1 and det2 has to be less than 20 ns
	bool p = fe.random == 0;										//random flag has to be = 0. random variable is written in the binary file
	//so it's the result of some check performed at the level of acquisition of 
	//when converting from raw to elm2
	
	
	//basically a sanity check, if i understood it correctly...
	if(eOK && tOK && p) {
	  int head1 = fe.z1 < 0 ? 0 : 1;
	  int sm1 = ((fe.x1 + xSize/2) / smX);
	  int ab1 =  (fe.y1 + ySize/2) / smY;			
	  
	  assert(head1 >= 0); assert(head1 <= 1);
	  assert(sm1 >= 0); assert(sm1 <= 3);
	  assert(ab1 >= 0); assert(ab1 <= 1);
	  doi[head1][sm1][ab1]->Fill(fe.z1);
	  
	  int head2 = fe.z2 < 0 ? 0 : 1;
	  int sm2 = ((fe.x2 + xSize/2) / smX);
	  int ab2 =  (fe.y2 + ySize/2) / smY;			
	  
	  assert(head2 >= 0); assert(head2 <= 1);
	  assert(sm2 >= 0); assert(sm2 <= 3);
	  assert(ab2 >= 0); assert(ab2 <= 1);
	  doi[head2][sm2][ab2]->Fill(fe.z2);
	}			
	
	//fills the head0 and head1 histos
	if (eOK && tOK && p) {
	  float inc = pxSize;
	  for(float dx = -1; dx <= 1; dx += inc) {
	    for(float dy = -1; dy <= 1; dy += inc) {
	      (fe.z1 < 0 ? h1 : h2)->Fill(fe.x1+dx, fe.y1+dy, inc*inc);
	      (fe.z2 < 0 ? h1 : h2)->Fill(fe.x2+dx, fe.y2+dy, inc*inc);
	    }
	  }
	}
	
	
	if(eOK && tOK && p) {
	  if(lastp != -1)
	    interval->Fill(fe.ts - lastp);
	  lastp = fe.ts;
	  nhits->Fill(fe.n1); nhits->Fill(fe.n2);
	}
	else if (eOK && tOK && !p) {
	  if(lastr != -1)
	    rinterval->Fill(fe.ts - lastr);
	  lastr = fe.ts;
	  rnhits->Fill(fe.n1); rnhits->Fill(fe.n2);
	}
	
	//some opearations for calculating the intersection of LORs with z=0
	float s = (0 - fe.z1)/w;
	float mx = fe.x1 + s*u;
	float my = fe.y1 + s*v;
	
	// 	printf("%s %s %s\n", 
	// 		       eOK  ? "true" : "false", 
	// 		       tOK  ? "true" : "false", 		       
	// 		       p  ? "true" : "false");
	
	(p ? e : re)->Fill(fe.e1);
	(p ? e : re)->Fill(fe.e2);
	energyTime->Fill(fe.ts - t0, fe.e1);
	energyTime->Fill(fe.ts - t0, fe.e2);
	energy2->Fill(fe.e1, fe.e2);
	
	if (eOK) {
	  (p ? dt : rdt)->Fill(fe.dt);
	}
	
	//fills the "global" projection histogram if energy and time are good, and if the random flag is 0
	if(eOK && tOK) {
	  (p ? h : hr)->Fill(mx, my);
	  if (p)
	    projectionSlice[projectionCounter]->Fill(mx, my);
	}
	//cProjectionSlice[projectionCounter]->cd();
	//projectionSlice[projectionCounter]->Draw("COLZ");
      }
      
      fclose(fIn);
    }
    
    //write the last one...
    std::stringstream sCtitle,sFile;
    sCtitle << "projection_T" <<  projectionCounter*timeSliceLength << "_T" << (projectionCounter+1)*timeSliceLength;
    std::string Ctitle = sCtitle.str();
    std::string FileTitle = Ctitle + ".gif";
    cProjectionSlice[projectionCounter] = new TCanvas(Ctitle.c_str(),Ctitle.c_str(),1200,800);
    projectionSlice[projectionCounter]->Draw("COLZ");
    fProjections->cd();
    cProjectionSlice[projectionCounter]->Write();
    projectionSlice[projectionCounter]->Write();
    cProjectionSlice[projectionCounter]->Print(FileTitle.c_str());
    
    sliceTime.push_back(projectionCounter*timeSliceLength);
    xMean.push_back(projectionSlice[projectionCounter]->GetMean(1));
    xMeanError.push_back(projectionSlice[projectionCounter]->GetMeanError(1));
    yMean.push_back(projectionSlice[projectionCounter]->GetMean(2));
    yMeanError.push_back(projectionSlice[projectionCounter]->GetMeanError(2));
    
    
    TCanvas *CanvasXgraph = new TCanvas("Average X vs. Time","Average X vs. Time",1200,800);
    xGraph = new TGraphErrors(sliceTime.size(),&sliceTime[0],&xMean[0],0,&xMeanError[0]);
    xGraph->SetTitle("Average X vs. Time");
    xGraph->GetXaxis()->SetTitle("Time [s]");
    xGraph->GetYaxis()->SetTitle("Average X [mm]");
    xGraph->Draw("A*");
    fProjections->cd();
    CanvasXgraph->Write();
    CanvasXgraph->Print("xMean.gif");
    
    TCanvas *CanvasYgraph = new TCanvas("Average Y vs. Time","Average Y vs. Time",1200,800);
    yGraph = new TGraphErrors(sliceTime.size(),&sliceTime[0],&yMean[0],0,&yMeanError[0]);
    yGraph->SetTitle("Average Y vs. Time");
    yGraph->GetXaxis()->SetTitle("Time [s]");
    yGraph->GetYaxis()->SetTitle("Average Y [mm]");
    yGraph->Draw("A*");
    fProjections->cd();
    CanvasYgraph->Print("yMean.gif");
    //   for(int i = 0; i < projectionCounter+1 ; i++){
    //     std::stringstream stitle1;
    //     stitle1 << "projection_T" <<  i*timeSliceLength << "_T" << (i+1)*timeSliceLength << ".gif";
    //     std::string title1 = stitle1.str();
    //     cProjectionSlice[i]->Print(title1.c_str());
    //   }
    
    //Write the "usual" output file"
    f          ->cd();
    h         ->Write();
    hr        ->Write();
    h1        ->Write();
    h2        ->Write();
    dt        ->Write();
    e         ->Write();
    interval  ->Write();
    rdt       ->Write();
    re        ->Write();
    rinterval ->Write();
    nhits     ->Write();
    rnhits    ->Write();
    energy2   ->Write();
    energyTime->Write();
    for(int head = 0; head < 2; head++) for(int sm = 0; sm < 4; sm++) for(int ab = 0; ab < 2; ab++) {
      doi[head][sm][ab] ->Write();    
    }
    
    //close the output files
    fProjections->Close();
    f->Close();
    
    
    printf("%lld events\n", nEvents);
    if(nBad > 0) printf("Found %lld insane events\n", nBad);
    printf("Acquisition time: %lf (%lf to %lf)\n", tMax - tMin, tMin, tMax);
    
    return 0;
  }
}
