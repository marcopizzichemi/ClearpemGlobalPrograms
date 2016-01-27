// compile with
// g++ -o angleSplit angleSplit.cpp

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
#include <sstream>
#include <fstream>
#include <vector>

// #include <TCanvas.h>
// #include <TH2F.h>
// #include <TFile.h>
// #include <TH1F.h>

static float eMin = 400;
static float eMax = 650;
static float dMax = 4E-9;
static int   maxHits = 4;

static float tRandMin = 20e-9;
static float tRandMax = 90e-9;

static const int nCols = 6;

struct EventFormat {
  double ts;
  u_int8_t random;
  float d;
  float yozRot;
  float x1;
  float y1;
  float z1;
  float e1;
  u_int8_t n1;
  float x2;
  float y2;
  float z2;
  float e2;
  u_int8_t n2;
  float dt;
} __attribute__((__packed__));


int main(int argc, char * argv[]) 
{
  
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
  
  
  
  // 	TFile *f = new TFile(argv[1], "RECREATE");
  // 	TH2F *h = new TH2F("projection", "projection", nx, -lx/2, lx/2, ny, -ny/2, ny/2);
  // 	TH2F *hr = new TH2F("rprojection", "rprojection", nx, -lx/2, lx/2, ny, -ny/2, ny/2);
  // 	TH2F *h1 = new TH2F("head0", "head 0", nx, -lx/2, lx/2, ny, -ny/2, ny/2);
  // 	TH2F *h2 = new TH2F("head1", "head 1", nx, -lx/2, lx/2, ny, -ny/2, ny/2);
  // 
  // 	TH1F *dt = new TH1F("time", "Time Spectrum", 200, -100E-9, 100E-9);
  // 	TH1F *e = new TH1F("energy", "Energy Spectrum", 1000, 0, 1000);
  // 	TH1F *interval= new TH1F("interval", "Time Interval", 1000000, 0, 1E-3);
  // 	
  // 	TH1F *rdt = new TH1F("rtime", "Delayed Time Spectrum", 200, -20E-9, 20E-9);
  // 	TH1F *re = new TH1F("renergy", "Delayed Energy Spectrum", 1000, 0, 1000);
  // 	TH1F *rinterval= new TH1F("rinterval", "Delayed Time Interval", 1000000, 0, 1E-3);
  // 
  // 	TH1F *nhits = new TH1F("nhits", "nHits", 4, 1, 4);
  // 	TH1F *rnhits = new TH1F("rnhits", "Delayed nHits", 4, 1, 4);
  // 	
  //         TH1F *randomFlag = new TH1F("randomFlag", "Random Flag", 20, -10, 10);
  
  
  // 	TH1F *doi [2][4][2];
  for(int head = 0; head < 2; head++) for(int sm = 0; sm < 4; sm++) for(int ab = 0; ab < 2; ab++) {
    char name[256];		
    sprintf(name, "doi_sm%d%c", 1 + head + 2*sm, 'A' + ab);
    // 		doi[head][sm][ab] = new TH1F(name, name, 4000, -500, 500);
    
  }
  
  float smX = xSize / 4;
  float smY = ySize / 2;
  
  for(int i = 1; i < argc; i++) {
    
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
    
    double lastp = -1;
    double lastr = -1;
    EventFormat fe;
    float angleFound = 0;
    int angleNum = 0;
//     ofstream output_file(outFileName.c_str(), std::ios::binary);
    //std::vector<std::ofstream*> output_file;
    std::ofstream* CurrentOutput = NULL;
    
    while(fread((void*)&fe, sizeof(fe), 1, fIn) == 1 /*&& nEvents < 500000*/) 
    {
      
      if(nEvents == 0)
      {
	angleFound = fe.yozRot;
	angleNum++;
	printf("Found angle [rad] = %f\nDistance = %f\nTime = %f\n",angleFound,fe.d,fe.ts);
	//create the string for the output file
        std::stringstream OutStringStream;
        OutStringStream << "angle_" << angleNum <<  "_" << angleFound << ".elm2";
        CurrentOutput = new std::ofstream(OutStringStream.str().c_str(), std::ios::binary);
//         output_file.push_back(&CurrentOutput);
      }
      else if(angleFound != fe.yozRot)
      {
	CurrentOutput->close();
	angleFound = fe.yozRot;
	angleNum++;
	printf("Found angle [rad] = %f\nDistance = %f\nTime = %f\n",angleFound,fe.d,fe.ts);
	//create the string for the output file
       std::stringstream OutStringStream;
       OutStringStream << "angle_" << angleNum << "_" << angleFound << ".elm2";
       CurrentOutput = new std::ofstream(OutStringStream.str().c_str(), std::ios::binary);
//        output_file.push_back(&CurrentOutput);
      }
      
      nEvents++;
      
      CurrentOutput->write((char*)&fe,sizeof(fe));
      
//       printf("%f %f\n",fe.d,fe.yozRot);
      
//       float u = fe.x1 - fe.x2;
//       float v = fe.y1 - fe.y2;
//       float w = fe.z1 - fe.z2;
//       
//       
//       if(fabs(w) < fe.d/2.0) {
// 	//printf("%f %f\n",fabs(w),fe.d/2.0);
// 	nBad++;
// 	continue;
//       }
//       
//       if (fe.z1 > 0)
// 	fe.dt *= -1;
//       
//       
//       //randomFlag->Fill(fe.random); //add by me, to plot all the radom flag values
//       
//       bool eOK = (fe.e1 >= eMin) && (fe.e1 <= eMax) && (fe.e2 >= eMin) && (fe.e2 <= eMax);
//       bool tOK = fabs(fe.dt) < dMax;
//       bool p = fe.random == 0;
//       bool hitOK = (fe.n1 < 4) && (fe.n2 <4);
//       bool randomTimeWindow = (fabs(fe.dt) > 20e-9) && (fabs(fe.dt) < 90e-9);
//       
//       
//       if(eOK && tOK && p) {
// 	int head1 = fe.z1 < 0 ? 0 : 1;
// 	int sm1 = ((fe.x1 + xSize/2) / smX);
// 	int ab1 =  (fe.y1 + ySize/2) / smY;			
// 	
// 	assert(head1 >= 0); assert(head1 <= 1);
// 	assert(sm1 >= 0); assert(sm1 <= 3);
// 	assert(ab1 >= 0); assert(ab1 <= 1);
// 	// 				doi[head1][sm1][ab1]->Fill(fe.z1);
// 	
// 	int head2 = fe.z2 < 0 ? 0 : 1;
// 	int sm2 = ((fe.x2 + xSize/2) / smX);
// 	int ab2 =  (fe.y2 + ySize/2) / smY;			
// 	
// 	//printf("%f %f %d %d\n",smY,fe.y1,ab1,ab2);
// 	
// 	assert(head2 >= 0); assert(head2 <= 1);
// 	assert(sm2 >= 0); assert(sm2 <= 3);
// 	assert(ab2 >= 0); assert(ab2 <= 1);
// 	// 				doi[head2][sm2][ab2]->Fill(fe.z2);
//       }			
//       
//       
//       if (eOK && tOK && p) {
// 	//(fe.z1 < 0 ? h1 : h2)->Fill(fe.x1, fe.y1);
// 	//(fe.z2 < 0 ? h1 : h2)->Fill(fe.x2, fe.y2);
//       }
//       
//       
//       if(eOK && tOK && p) {
// 	if(lastp != -1)
// 	  // 					interval->Fill(fe.ts - lastp);
// 	  lastp = fe.ts;
// 	// 				nhits->Fill(fe.n1); nhits->Fill(fe.n2);
//       }
//       else if (eOK && tOK && !p) {
// 	if(lastr != -1)
// 	  // 					rinterval->Fill(fe.ts - lastr);
// 	  lastr = fe.ts;
// 	// 				rnhits->Fill(fe.n1); rnhits->Fill(fe.n2);
//       }
//       
//       
//       float s = (0 - fe.z1)/w;
//       float mx = fe.x1 + s*u;
//       float my = fe.y1 + s*v;
//       
//       // 	printf("%s %s %s\n", 
//       // 		       eOK  ? "true" : "false", 
//       // 		       tOK  ? "true" : "false", 		       
//       // 		       p  ? "true" : "false");
//       
//       // 			(p ? e : re)->Fill(fe.e1);
//       // 			(p ? e : re)->Fill(fe.e2);
//       
//       if (eOK) {
// 	// 				(p ? dt : rdt)->Fill(fe.dt);
//       }
//       
//       
//       if(eOK && tOK && p && hitOK) {  //if event is in the energy window and delta t < 4ns, then it's a true (prompt)
// 	nTrues++;
// 	// 			  (p ? h : hr)->Fill(mx, my);
//       }
//       
//       if(eOK && randomTimeWindow && p && hitOK) {  //if the event is in the energy window and in the delayed delta time, then it's a random
// 	nRandoms++;
// 	// 			  (p ? h : hr)->Fill(mx, my);
//       }
    }
    
    CurrentOutput->close();
  }
  // 	f->Write();
  // 	f->Close();
  //but we need to rescale the number of random between the tRandMin and tRandMax to be comparable to the 4 ns time window
//   double randomRescaled = nRandoms * (dMax / (tRandMax - tRandMin));
//   
//   
//   printf("%lld Total Events\n", nEvents);
//   printf("%lld Prompts\n", nTrues);
//   printf("%.0lf Randoms\n", randomRescaled);
//   if(nBad > 0) printf("Found %lld insane events\n", nBad);
  return 0;
  
}
