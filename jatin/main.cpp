#include <iostream>
#include "LLC.h"
#include "L2.h"
#include<cmath>
using namespace std;

#define CORE_COUNT 4



#define L2_ID 0
#define L3_ID 1

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


int main(int)
    {
    L2 L2[CORE_COUNT];
    LLC LLC;    
    FILE *fp;
    char input_name[100];
    unsigned long long addr, blockAddr,TAG,SET;
    int threadid;
    sprintf(input_name, "%s", argv[1]);
    fp = fopen(input_name, "r");
    //Checking error during file read
    if (fp == NULL)
    {
        printf("Error!");
        exit(1);
    }
    while (!feof(fp))
    {
        fscanf(fp, "%d  %llx\n", &threadid, &addr);
        addr>> = 6;
        //Reading the file
       
          //extract bits according to L2
          extract_TagSet(L2_ID, addr, &TAG, &SET);
          //search in l2
          WAY= L2[threadid].search(SET,TAG);                                   
          //WAY return -1 if not present in L2
          if(WAY!=-1)
          {
            //L2 Hit
            L2[threadid].Hit_l2++;
            //update lru L2 timestamp
            L2[threadid].update_timestamp(SET,WAY,1);
          }
          else
          { 
            //L2 Miss 
            L2[threadid].Miss_l2++;
            //extract bits according to L3
            extract_TagSet(L3_ID, addr, &TAG, &SET);
            //search in L3
            WAY= LLC.SEARCH(SET,TAG);
            //WAY return -1 if not present in L2
            if(WAY!=-1)
            { 
              //L3 Hit 
              LLC.Hit_l3++;
              //update LRU L3 
              LLC.update_timestamp(SET,WAY,1);
              //find L2 address and extract tag,set
              extract_TagSet(L2_ID, addr, &TAG, &SET);
              //Query L2 LRU for way no., fill L2 cache and Update LRU L2	
              L2[threadid].insert(SET,TAG);        
            }
            else
            {
              //L3 Miss
              LLC.Miss_l3++;
              //query LRU L3 to get way no.
              WAY=LLC.find_victim(SET);
              //if valid data is not present in L3 at way 
              if(LLC.time_Stamp_L3[SET][WAY]==0)
              { 
                //put data in L3 and update L3
                 LLC.L3[SET][WAY]=TAG;
                 LLC.update_timestamp(SET,WAY,1); 
                //Extract Tag and set for L2 and insert in L2
                extract_TagSet(L2_ID, addr, &TAG, &SET);
                L2[threadid].insert(SET,TAG);         
              } 
              else  
              //if conflict happens in L3 at WAY
              {
                //Get the tag of conflicting block and invalidate evicted line from L2 
                TAG = L3[SET][WAY];
                New_addr=TAG<<log2(L3_SETS);
                New_addr=(New_addr|SET);
                New_addr<<=6;
                extract_TagSet(L2_ID, New_addr, &TAG, &SET);
                //get WAY
                for(int i=0;i<CORE_COUNT;i++)
                {
                WAY=L2[i].search(SET,TAG);
                if(WAY!=-1)
                {
                  //invalidate WAY in L2---set timestamp 0
                   L2[i].update_timestamp(SET,WAY,0);
                  
                }

                }
                //extract tag acc. to L3 and insert in L3  
                extract_TagSet(L3_ID, addr, &TAG, &SET);
                LLC.insert(SET,TAG);
                
                //extract tag acc. to L2 and insert in L2
                extract_TagSet(L2_ID, addr, &TAG, &SET);
               L2[threadid].insert(SET,TAG);
              }
            }
          
          }
      }
      fclose(fp);
    
    printf("\n\nUsing INCLUSIVE POLICY");
    cout<<" L3 Hit:"<<LLC.Hit_l3 <<" L3 Miss:"<<LLC.Miss_l3;
    //printf("\nL2 Hit  >= %lld\nL2 Miss = %lld\nL3 Hit  = %lld\nL3 Miss = %lld",L2.Hit_l2,L2.Miss_l2,LLC.Hit_l3,LLC.Miss_l3);
    //print();
  }

