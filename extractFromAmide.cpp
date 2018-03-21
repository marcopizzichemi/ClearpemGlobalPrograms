//simple analysis

// compile with 
// g++ -o ../build/doifit doifit.cpp `root-config --cflags --glibs`


#include "TROOT.h"
#include "TTree.h"
#include "TFile.h"
#include "TH2F.h"
#include "TH1F.h"
#include "TChain.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TGraphErrors.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "TMath.h"


class input_t
{
public:
  int x;
  int y;
  double w;
  double z;
  double s;
  double sqrt_nentries;
  
  friend std::istream& operator>>(std::istream& input, point_t& s)
  {
    input >> s.x; 
    input >> s.y;           
    input >> s.w; 
    input >> s.z;    
    input >> s.s; 
    input >> s.sqrt_nentries;
    return input;
  }
  
};


int main (int argc, char** argv)
{

  
  
  return 0;
}
