#include <stdio.h>
#include <fstream>

int main(int argc, char * argv[]) 
{
  ifstream cmap ("crystal_map.txt");
  while(!cmap.eof()) 	
  {
    int cid, x, y;
    cmap >> cid >> x >> y;
    if(cid >= 6144/2)
    y += 48;
    crystalX[cid] = x;
    crystalY[cid] = y;
  }
  
  
  
  
  return 0;
}