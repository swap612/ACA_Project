#include <iostream>
#include "LLC.h"
#include "L2.h"
#include<cmath>
using namespace std;

#define CORE_COUNT 1

#define L2_ID 0
#define L3_ID 1


/*Fuction to extract tag and set from given addr */
void extract_TagSet(int cacheID, unsigned long long addr, unsigned long long *tag, unsigned long long *set)
{
    // int lShift = 0;
    //     //cacheID is L2
    // if (cacheID == 0)
    // {
    //     *set = ((addr>> 6) & (int)(pow(2,log2(L2_SETS)+1)-1)) ;
    //     lShift = (log2(L2_SETS) + 6);
    //     *tag = (addr) >> lShift;
    // }
    // //cacheID is L3
    // else if (cacheID == 1)
    // {
    //     *set = ((addr>> 6) & (int)(pow(2,log2(L3_SETS)+1)-1)) ;
    //     lShift = (log2(L3_SETS) + 6);
    //     *tag = (addr) >> lShift;
    // }
     //cacheID is L2
  if (cacheID == 0)
  {
    *set = (addr & 0xFFC0) >> 6;
    *tag = (addr & 0xFFFFFFFFFFFF0000) >> 16;
  }
  //cacheID is L3
  else if (cacheID == 1)
  {
    *set = (addr & 0x1FFC0) >> 6;
    *tag = (addr & 0xFFFFFFFFFFFE0000) >> 17;
  }
}


int main(int argc, char** argv)
    {
    L2 L2[CORE_COUNT];
    LLC LLC;    
    FILE *fp;
    char input_name[100];
    unsigned long long addr, newAddr, blockAddr, way, tag, set, pc;
    unsigned long long timeStampCntr = 0;
    int threadid;

  char iord, type;

    sprintf(input_name, "%s", argv[1]);
    fp = fopen(input_name, "rb");
    //Checking error during file read
    if (fp == NULL)
    {
        printf("Error!");
        exit(1);
    }
    while (!feof(fp))
    {
        //Reading the file
        fread(&iord, sizeof(char), 1, fp);
        fread(&type, sizeof(char), 1, fp);
        fread(&addr, sizeof(unsigned long long), 1, fp);
        fread(&pc, sizeof(unsigned), 1, fp);

        if(type == 0)
           continue;
        
        cout<<addr<<"\n";        

        //fscanf(fp, "%d  %llx\n", &a, &threadid, &addr, &b);
        addr>>= 6;
        
        threadid = 0;
       
          //extract bits according to L2
          extract_TagSet(L2_ID, addr, &tag, &set);
          //search in l2
          way= L2[threadid].search(set,tag);                                   
          //way return -1 if not present in L2
          if(way!=-1)
          {
            //L2 Hit
            cout<< "L2 Hit found at way "<<way ;

            L2[threadid].hitCount++;
            //update lru L2 timestamp
            L2[threadid].update_timestamp(set, way, 1, &timeStampCntr);
          }
          else
          { 
            //L2 Miss 
            cout<< "L2 Miss found at way "<<way ;
            L2[threadid].missCount++;
            //extract bits according to L3
            extract_TagSet(L3_ID, addr, &tag, &set);
            //search in L3
            way= LLC.search(set,tag);
            //way return -1 if not present in L2
            if(way!=-1)
            { 
              //L3 Hit 
              LLC.hitCount++;
              //update LRU L3 
              LLC.update_timestamp(set, way, 1, &timeStampCntr);
              //find L2 address and extract tag,set
              extract_TagSet(L2_ID, addr, &tag, &set);
              //Query L2 LRU for way no., fill L2 cache and Update LRU L2	
              L2[threadid].insert(set,tag, &timeStampCntr);        
            }
            else
            {
              //L3 Miss
              LLC.missCount++;
              //query LRU L3 to get way no.
              way=LLC.find_victim(set);
              //if valid data is not present in L3 at way 
              if(LLC.timeStamp[set][way]==0)
              { 
                //put data in L3 and update L3
                 LLC.tags[set][way] = tag;
                 LLC.update_timestamp(set, way, 1, &timeStampCntr); 
                //Extract Tag and set for L2 and insert in L2
                extract_TagSet(L2_ID, addr, &tag, &set);
                L2[threadid].insert(set, tag, &timeStampCntr);         
              } 
              else  
              //if conflict happens in L3 at way
              {
                //Get the tag of conflicting block and invalidate evicted line from L2 
                tag = LLC.tags[set][way];
                int shift = log2(L3_SETS);
                newAddr = tag<< shift ;
                newAddr=(newAddr|set);
                newAddr<<=6;
                extract_TagSet(L2_ID, newAddr, &tag, &set);
                //get way
                for(int i=0;i<CORE_COUNT;i++)
                {
                way=L2[i].search(set,tag);
                if(way!=-1)
                {
                  //invalidate way in L2---set timestamp 0
                   L2[i].update_timestamp(set, way, 0, &timeStampCntr);
                  
                }

                }
                //extract tag acc. to L3 and insert in L3  
                extract_TagSet(L3_ID, addr, &tag, &set);
                LLC.insert(set, tag, &timeStampCntr);
                
                //extract tag acc. to L2 and insert in L2
                extract_TagSet(L2_ID, addr, &tag, &set);
                L2[threadid].insert(set, tag, &timeStampCntr);
              }
            }
          
          }
      }
      fclose(fp);
    
    printf("\n\nUsing INCLUSIVE POLICY");
    cout<<" L2 Hit:"<<L2[threadid].hitCount <<" L2 Miss:"<<L2[threadid].missCount;
    cout<<" L3 Hit:"<<LLC.hitCount <<" L3 Miss:"<<LLC.missCount;
   
  }

