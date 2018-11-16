#include <iostream>
#include "LLC.h"
#include "L2.h"
#include <cmath>
#include<stdlib.h>
using namespace std;


//Declaring Queue Variables
int q_empty = queue_size, CORE_COUNT;
unsigned long long req_waiting_time = 0, service_time = 20;

unsigned long long Bus_transactions, cache_read, mem_write, Coherence_invalidation, timeStampCntr;

//Calculating response and request wait time
void queue_wait(int cpu, L2 *L2)
{
  L2[cpu].no_requests++;
  if (q_empty == 0)
  {
    req_waiting_time += service_time;
  }
  else
  {
    q_empty--;
  }

  if (L2[cpu].q_empty == 0)
  {
    L2[cpu].res_waiting_time += service_time;
  }
  else
  {
    L2[cpu].q_empty--;
  }
}
//function to generate Bus_transactions transactions requests and responses for every processor read /write.
int Search_and_update(L2 *L2, char current_state, int op, unsigned long long set, unsigned long long tag, int cpu)
{
  queue_wait(cpu, L2);
  char state[CORE_COUNT];
  int way, mod_found = 0, mod_way, mod_cpu, found = 0, shared_way, shared_cpu, shared_found = 0;
  int found_ways[CORE_COUNT];
  //Search for all processors for given block (snop the line)
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
    // if block is modified in any processor private cache
    if (mod_found == 1)
    {
      L2[mod_cpu].dir[set][mod_way] = S;
      Bus_transactions += 2;
      mem_write++;
      cache_read++;
    }
    // if block is in shared in any processor private cache
    else if (shared_found == 1)
    {
      Bus_transactions += 2;
      cache_read++;
    }
    else
    { //if in any cache block is not found,get it from memory(that may be miss or hit in LLC)
      return 1;
    }
  }

  if (current_state == I && op == PW)
  {
    if (mod_found == 1)
    {
      //invalidate modified block
      L2[mod_cpu].update_timestamp(set, mod_way, 0, &timeStampCntr);
      Bus_transactions += 2;
      cache_read++;
      Coherence_invalidation++;
    }
    else if (shared_found == 1)
    {

      for (int i = 0; i < CORE_COUNT; i++)
      {
        if (i != cpu && state[i] == S)
        {
          L2[i].update_timestamp(set, found_ways[i], 0, &timeStampCntr);
          Coherence_invalidation++;
        }
      }
      Bus_transactions += 2;
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

    if (shared_found == 1)
    {
      for (int i = 0; i < CORE_COUNT; i++)
      {

        if (i != cpu && state[i] == S)
        {
          L2[i].update_timestamp(set, found_ways[i], 0, &timeStampCntr);
          Coherence_invalidation++;
        }
      }
      Bus_transactions += 2;
      cache_read++;
    }
  }
  return 0;
}

/*Fuction to extract tag and set from given addr */
void extract_TagSet(int cacheID, unsigned long long addr, unsigned long long *tag, unsigned long long *set)
{
  int lShift = 0;
  //cacheID is L2
  if (cacheID == 0)
  {
    *set = ((addr >> 6) & (int)(pow(2, log2(L2_SETS)) - 1));
    lShift = (log2(L2_SETS) + 6);
    *tag = (addr) >> lShift;
  }
  //cacheID is L3
  else if (cacheID == 1)
  {
    *set = ((addr >> 6) & (int)(pow(2, log2(L3_SETS)) - 1));
    lShift = (log2(L3_SETS) + 6);
    *tag = (addr) >> lShift;
  }
}
int main(int argc, char **argv)
{

  FILE *fp;
  char input_name[100];
  unsigned long long addr, newAddr, blockAddr, way, tag, set, pc, Clk_count = 0;
  //Checking arguments
  if (argc != 3) {
       printf("Need two argument to run: 1.Number of Threads, 2.Trace file.\nEx: ./mesi 8 trace\n" );
       exit (1);
    }
  CORE_COUNT = strtol(argv[1],NULL,10);

  L2 L2[CORE_COUNT];
  LLC LLC;
  int threadid, R;

  char op, current;

  sprintf(input_name, "%s", argv[2]);
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
    if (threadid >= CORE_COUNT)
      continue;
L2[threadid].Clk_count++;
    Clk_count++;
    //removing one entry from the request queue
    if (Clk_count == service_time)
    {
      if (q_empty < queue_size)
        q_empty++;
    }
    //removing one entry from each of the response queue
   if (L2[threadid].Clk_count == service_time)
            {
                if (L2[threadid].q_empty < res_queue_size)
                    L2[threadid].q_empty++;
                L2[threadid].Clk_count=0;
            }

    if (Clk_count == service_time)
      Clk_count = 0;
    extract_TagSet(L2_ID, addr, &tag, &set);
    //search in l2
    way = L2[threadid].search(set, tag);
    //way return -1 if not present in L2
    if (way != -1)
    {
      //L2 cache hit, check to see if state transition is from S to M
      current = L2[threadid].dir[set][way];
      if (current == S && op == PW)
      {
        R = Search_and_update(L2, current, op, set, tag, threadid);
        L2[threadid].dir[set][way] = M;
      }
      //L2 hit
      L2[threadid].hitCount++;
      //update L2 lru timestamp
      L2[threadid].update_timestamp(set, way, 1, &timeStampCntr);
    }
    else
    {
      /********* L2 miss,check for read/write and search in other caches and modify states accordingly******/
      L2[threadid].missCount++;
      R = Search_and_update(L2, I, op, set, tag, threadid);
      if (R == 1)
      {
        //extract bits according to L3
        extract_TagSet(L3_ID, addr, &tag, &set);
        //search in L3
        way = LLC.search(set, tag);
        //way return -1 if not present in L2
        if (way != -1)
        {
          //L3 Hit
          LLC.hitCount++;
          //update LRU L3
          LLC.update_timestamp(set, way, 1, &timeStampCntr);
          //find L2 address and extract tag,set
          extract_TagSet(L2_ID, addr, &tag, &set);
          //Query L2 LRU for way no., fill L2 cache and Update LRU L2
          way = L2[threadid].insert(set, tag, &timeStampCntr);
          //Update the L2 directory
          if (L2[threadid].dir[set][way] == M)
            mem_write++;
          if (op == PR)
            L2[threadid].dir[set][way] = S;
          else
            L2[threadid].dir[set][way] = M;
        }
        else
        {
          //L3 Miss
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
            if (L2[threadid].dir[set][way] == M)
              mem_write++;
            if (op == PR)
              L2[threadid].dir[set][way] = S;
            else
              L2[threadid].dir[set][way] = M;
          }
          else
          //if conflict happens in L3 at way
          {
            //Get the tag of conflicting block and invalidate evicted line from L2
            tag = LLC.tags[set][way];
            //calculate the address L2 from L3
            int shift = log2(L3_SETS);
            newAddr = tag << shift;
            newAddr = (newAddr | set);
            newAddr <<= 6;
            extract_TagSet(L2_ID, newAddr, &tag, &set);

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
            if (L2[threadid].dir[set][way] == M)
              mem_write++;
            if (op == PR)
              L2[threadid].dir[set][way] = S;
            else
              L2[threadid].dir[set][way] = M;
          }
        }
      }
      else
      {
        // found in other proc cache
        way = L2[threadid].insert(set, tag, &timeStampCntr);
        if (L2[threadid].dir[set][way] == M)
          mem_write++;
        if (op == PR)
          L2[threadid].dir[set][way] = S;
        else
          L2[threadid].dir[set][way] = M;
      }
    }
  }
  fclose(fp);
  printf("\n\nUsing MSI POLICY");
 for (int i = 0; i < CORE_COUNT; i++)
    {
        cout << " L2[" << i << "]Hit: " << L2[i].hitCount << " L2[" << i << "] Miss: " << L2[i].missCount;
        cout << " L2[" << i << "] "
             << "Average Response waiting  time: " <<(L2[i].res_waiting_time)<<"/"<<(L2[i].no_requests)<<"="<< ((float)L2[i].res_waiting_time)/(float)(L2[i].no_requests) << endl;
    }
    cout << " L3 Hit: " << LLC.hitCount << " L3 Miss: " << LLC.missCount << " Bus_transactions transactions: " << Bus_transactions << " Cache reads: " << cache_read << " Average Request waiting time: " << (float)req_waiting_time / (float)Bus_transactions << " Mem_write: " << mem_write << " Coherence_invalidation: " << Coherence_invalidation;
}
