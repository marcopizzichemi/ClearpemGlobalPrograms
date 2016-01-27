// simple program to generate a "flat" eml2 file, for testing purposes

// compile with
// g++ -o generateElm generateElm.cpp

//syntax

// generaElm N
// N = number of events to generate

// output is a file called flat_file.elm2

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <getopt.h>
#include <iostream>
#include <set>
#include <assert.h>
#include <string.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <time.h>


struct EventFormat {
  double ts;                     // time of the event, in seconds. if i'm not mistaken is the absolute machine time in seconds 
  u_int8_t random;               // 
  float d;                       // distance between the heads, fixed to the head distance value... 
  float yozRot;                  // rotation of the heads 
  float x1;                      // x coordinate of the event, for detector 1		
  float y1;                      // y coordinate of the event, for detector 1
  float z1;                      // z coordinate of the event, for detector 1
  float e1;                      // energy deposited in detector 1
  u_int8_t n1;                   // 
  float x2;                      // x coordinate of the event, for detector 2
  float y2;                      // y coordinate of the event, for detector 2
  float z2;                      // z coordinate of the event, for detector 2
  float e2;                      // energy deposited in detector 2
  u_int8_t n2;                   // 
  float dt;                      // delta time between event in detector 1 and event in detector 2 
} __attribute__((__packed__));


int main(int argc, char * argv[]) 
{
  srand ( time(NULL) ); //initialize the random seed
  //output file
  std::ofstream* CurrentOutput = NULL;
  CurrentOutput = new std::ofstream("flat_file.elm2", std::ios::binary);
  
  long long int nEvents = atoi(argv[1]); //read the input (number of events to generate)
  float distance = 209.08; //hardcoded
  
  // arrays with positions of crystals in the heads
  float x_pos[64] = 
  {
    -85.75,-83.45,-81.15,-78.85,-76.55,-74.25,-71.95,-69.65,-63.85,-61.55,-59.25,-56.95,-54.65,-52.35,-50.05,-47.75,-41.25,-38.95,-36.65,-34.35,-32.05,-29.75,-27.45,-25.15,-19.35,-17.05,-14.75,-12.45,-10.15,-7.85,-5.55,-3.25,3.25,5.55,7.85,10.15,12.45,14.75,17.05,19.35,25.15,27.45,29.75,32.05,34.35,36.65,38.95,41.25,47.75,50.05,52.35,54.65,56.95,59.25,61.55,63.85,69.65,71.95,74.25,76.55,78.85,81.15,83.45,85.75
  };
  float y_pos[48] = 
  {
    -75.10,-72.8,-70.2,-67.9,-62.1,-59.8,-57.2,-54.9,-49.1,-46.8,-44.2,-41.9,-36.1,-33.8,-31.2,-28.9,-23.1,-20.8,-18.2,-15.9,-10.1,-7.8,-5.2,-2.9,2.9,5.2,7.8,10.1,15.9,18.2,20.8,23.1,28.9,31.2,33.8,36.1,41.9,44.2,46.8,49.1,54.9,57.2,59.8,62.1,67.9,70.2,72.8,75.1
  };

  // instantiate the event 
  EventFormat fe;
  
  // run as many times as the user wants
  for(long long int i = 0 ; i < nEvents; i++) 
  {
    //generate a random position in head 0 and 1
    int RandIndexX1 = rand() % 64;
    int RandIndexY1 = rand() % 48;
    int RandIndexX2 = rand() % 64;
    int RandIndexY2 = rand() % 48;
    //prepares the fe event
    fe.ts     = 0;
    fe.random = 0;
    fe.d      = distance;
    fe.yozRot = 0;
    fe.x1     = x_pos[RandIndexX1];
    fe.y1     = y_pos[RandIndexY1];
    fe.z1     = (distance/2.0) + 10.0;
    fe.e1     = 511.0;
    fe.n1     = 1;
    fe.x2     = x_pos[RandIndexX2];
    fe.y2     = y_pos[RandIndexY1];;
    fe.z2     = - (distance/2.0) - 10.0;
    fe.e2     = 511.0;
    fe.n2     = 1;
    fe.dt     = 1e-9;
    //write in the file
    CurrentOutput->write((char*)&fe,sizeof(fe));
  }

  CurrentOutput->close();
  return 0;
}