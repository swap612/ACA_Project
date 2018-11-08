#include <iostream>
#include "LLC.h"
#include "L2.h"
#include <cmath>
using namespace std;
unsigned long long Bus;
unsigned long long cache_read;
#define CORE_COUNT 8
#define PR 'R'
#define PW 'W'
#define I 'I'
#define S 'S'
#define M 'M'
#define L2_ID 0
#define L3_ID 1


unsigned long long timeStampCntr = 0;

int Search_and_update(L2 *L2, char current_state, int op, unsigned long long set, unsigned long long tag, int cpu)
{
  cout << endl
       << "Search:"
       << "current_state " << current_state << "op " << op << "set " << set << "tag " << tag << "cpu " << cpu << endl;
  char state[CORE_COUNT];
  int way, mod_found = 0, mod_way, mod_cpu, found = 0, shared_way, shared_cpu, shared_found=0;
  int found_ways[CORE_COUNT];
  for (int i = 0; i < CORE_COUNT; i++)
  {
    int f_way = 0;
    state[i] = L2[i].get_state(set, tag, &f_way);

    found_ways[i] = f_way;
    if (state[i] == M)
    {
      mod_found = 1;
      mod_way = f_way;
      mod_cpu = i;
    }
    if (state[i] == S)
    {
      shared_way = i;
      shared_cpu = i;
      shared_found = 1;
    }
  }

  if (current_state == I && op == PR)
  {
    cout << "I and R \n";
    if (mod_found == 1)
    { // if in any cache  block with M state is found then update new block,mark S,mark found block S and  update counters.(one memory write also in parrallel)
      way = L2[cpu].insert(set, tag, &timeStampCntr);
      L2[mod_cpu].dir[set][mod_way] = S;
      L2[cpu].dir[set][way] = S;
      Bus += 2;
      cache_read++;
    }

    else if (shared_found == 1)
    {
      //if in any cache  block with S state is found then just update new block,mark S and update counters.
      way = L2[cpu].insert(set, tag, &timeStampCntr);
      L2[cpu].dir[set][way] = S;
      Bus += 2;
      cache_read++;
    }
    else
    { //C.if in any cache block is not found,get it from memory(that may bemiss or hit in LLC)
      //get from LLC

      return 1;
    }
  }

  if (current_state == I && op == PW)
  {
    cout << "I and W \n";
    if (mod_found == 1)
    {
      way = L2[cpu].insert(set, tag, &timeStampCntr);

      L2[mod_cpu].update_timestamp(set, mod_way, 0, &timeStampCntr);
      L2[cpu].dir[set][way] = M;
      Bus += 2;
      cache_read++;
    }
    else if (shared_found == 1)
    {
      way = L2[cpu].insert(set, tag, &timeStampCntr);
      L2[cpu].dir[set][way] = M;
      for (int i = 0; i < CORE_COUNT; i++)
      {
        if (i!=cpu && state[i] == S)
          L2[i].update_timestamp(set, found_ways[i], 0, &timeStampCntr);
      }
      Bus += 2;
      cache_read++;
    }
    else
    { //C.if in any cache block is not found,get it from memory(that may bemiss or hit in LLC)
      //get from LLC

      return 1;
    }
  }

  if (current_state == S && op == PW)
  {
     L2[cpu].dir[set][ found_ways[cpu]] = M;
    if (shared_found == 1)
    { 
      cout << "S and W shared found\n";
      for (int i = 0; i < CORE_COUNT; i++)
      {
        cout << "inside loop\n";
        if (state[i] == S)
          L2[i].update_timestamp(set, found_ways[i], 0, &timeStampCntr);
      }
      cout << "out loop\n";

      Bus += 2;
      cache_read++;
    }
  }
  return 0;
}
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

int main(int argc, char **argv)
{
  L2 L2[CORE_COUNT];
  LLC LLC;
  FILE *fp;
  char input_name[100];
  unsigned long long addr, newAddr, blockAddr, way, tag, set, pc;

  int threadid, R;

  char op, current;

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

    fscanf(fp, "%d %llx %c\n", &threadid, &addr, &op);

    cout << threadid << op << addr << "\n";
    //addr>>= 6;
    //extract bits according to L2
    extract_TagSet(L2_ID, addr, &tag, &set);
    //search in l2
    way = L2[threadid].search(set, tag);
    //way return -1 if not present in L2

    if (way != -1)
    {
      /*****L2 cache hit, check to see if state transition is from s to M and update counters accordingly*********/
      current = L2[threadid].dir[set][way];
      if (current == S && op == PW)
        R = Search_and_update(L2, current, op, set, tag, threadid);
      //L2 Hit
      cout << "L2 Hit found at way " << way;

      L2[threadid].hitCount++;
      //update lru L2 timestamp
      L2[threadid].update_timestamp(set, way, 1, &timeStampCntr);
    }
    else
    {
      /********* L2 miss,check for read/write and search in other caches and modify states accordingly******/
      //L2 Miss

      R = Search_and_update(L2, I, op, set, tag, threadid);
      cout<<"R="<<R<<endl;
      cout << "L2 Miss found at way " << way;
      L2[threadid].missCount++;
      if (R == 1)
      {
        cout << "R==1 " << way;
        //extract bits according to L3
        extract_TagSet(L3_ID, addr, &tag, &set);
        //search in L3
        way = LLC.search(set, tag);
        //way return -1 if not present in L2
        if (way != -1)
        {
          //L3 Hit
          cout << "L3 Hit" << way;
          LLC.hitCount++;
          //update LRU L3
          LLC.update_timestamp(set, way, 1, &timeStampCntr);
          //find L2 address and extract tag,set
          extract_TagSet(L2_ID, addr, &tag, &set);
          //Query L2 LRU for way no., fill L2 cache and Update LRU L2
          way = L2[threadid].insert(set, tag, &timeStampCntr);
          if (op == PR)
            L2[threadid].dir[set][way] = S;
          else
            L2[threadid].dir[set][way] = M;
        }
        else
        {
          //L3 Miss
          cout << "L3 Miss" << way;
          LLC.missCount++;

          //query LRU L3 to get way no.
          way = LLC.find_victim(set);
          //if valid data is not present in L3 at way
          if (LLC.timeStamp[set][way] == 0)
          {
            //put data in L3 and update L3
            LLC.tags[set][way] = tag;
            LLC.update_timestamp(set, way, 1, &timeStampCntr);
            //Extract Tag and set for L2 and insert in L2
            extract_TagSet(L2_ID, addr, &tag, &set);
            way = L2[threadid].insert(set, tag, &timeStampCntr);
            if (op == PR)
              L2[threadid].dir[set][way] = S;
            else
              L2[threadid].dir[set][way] = M;
          }
          else
          //if conflict happens in L3 at way
          {
            cout << "conflict at L3 " << way;
            //Get the tag of conflicting block and invalidate evicted line from L2
            tag = LLC.tags[set][way];
            int shift = log2(L3_SETS);
            newAddr = tag << shift;
            newAddr = (newAddr | set);
            newAddr <<= 6;
            extract_TagSet(L2_ID, newAddr, &tag, &set);
            //get way
            for (int i = 0; i < CORE_COUNT; i++)
            {
              way = L2[i].search(set, tag);
              if (way != -1 && L2[i].dir[set][way] != I)
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
            way = L2[threadid].insert(set, tag, &timeStampCntr);
            if (op == PR)
              L2[threadid].dir[set][way] = S;
            else
              L2[threadid].dir[set][way] = M;
          }
        }
      }
    }
  }
  fclose(fp);

  printf("\n\nUsing INCLUSIVE POLICY");
  for (int i = 0; i < CORE_COUNT; i++)
  {
    cout << " L2[" << i << "]Hit:" << L2[i].hitCount << " L2 Miss:" << L2[i].missCount << endl;
  }
  cout << " L3 Hit:" << LLC.hitCount << " L3 Miss:" << LLC.missCount << " Bus read:" << Bus << "Cache reads:" << cache_read;
}
