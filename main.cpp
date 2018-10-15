#include <iostream>
#include<cmath>
using namespace std;

#define CORE_COUNT 4

#define L2_SETS 2048
#define L2_WAYS 8

#define L3_SETS 2048
#define L2_WAYS 8


/*Fuction to extract tag and set from given addr */
void extract_TagSet(int cacheID, unsigned long long addr, unsigned long long *tag, unsigned long long *set)
{
  //cacheID is L2
  if (cacheID == 0)
  {
    *set = (addr & 0xFFC0) >> 6;
    *tag = (addr & 0xFFFFFFFFFFFF0000) >> log2(L2_SETS);
  }
  //cacheID is L3
  else if (cacheID == 1)
  {
    *set = (addr & 0x1FFC0) >> 6;
    *tag = (addr & 0xFFFFFFFFFFFE0000) >> log2(L3_SETS);
  }
}


int main(){
    // L2 L2[4];

    // for(int i=0; i<){

    // }    
    cout << log2(L2_SETS)<<" "<<log2(L2_WAYS);

}