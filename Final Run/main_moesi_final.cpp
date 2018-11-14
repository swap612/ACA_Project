#include <iostream>
#include "LLC.h"
#include "L2.h"
#include <cmath>
using namespace std;


//Declaring Queue Variables
int q_empty=queue_size;
unsigned long long waiting_time=0, service_time= 5;


//Declaring Variables
unsigned long long Bus, cache_read, mem_write, invalidation, timeStampCntr;

//Creating L2 and LLC Objects
L2 L2[CORE_COUNT];
LLC LLC;
void queue_wait(int cpu)
{
    if(q_empty==0)
        {
            waiting_time+=service_time;
        }
    else{
           q_empty--;
        }

    if(L2[cpu].q_empty==0)
        {
            L2[cpu].waiting_time+=service_time;
        }
    else{
           L2[cpu].q_empty--;
        }

}

int Search_and_update(char current_state, int op, unsigned long long set, unsigned long long tag, int cpu)
{
    queue_wait(cpu);
    // cout << endl
    //      << "Search:"
    //      << "current_state " << current_state << "op " << op << "set " << set << "tag " << tag << "cpu " << cpu << endl;
    char state[CORE_COUNT];
    int way, mod_found = 0, mod_way, mod_cpu, found = 0, shared_way, shared_cpu, shared_found = 0, excl_found = 0, excl_way, excl_cpu, owner_found = 0, owner_way, owner_cpu;
    int found_ways[CORE_COUNT];
    for (int i = 0; i < CORE_COUNT; i++)
    {
        int f_way = 0;
        state[i] = L2[i].get_state(set, tag, &f_way);

        //if way not found f_way contains 0. it may lead to wrong result
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
        if (state[i] == E)
        {
            excl_found = 1;
            excl_way = f_way;
            excl_cpu = i;
        }

        if (state[i] == O)
        {
            owner_found = 1;
            owner_way = f_way;
            owner_cpu = i;
        }
    }

    if (current_state == I && op == PR)
    {
        // cout << "I and R \n";
        if (mod_found == 1)
        { // if in any cache  block with M state is found then update new block,mark S,mark found block S and  update counters.(one memory write also in parrallel)

            //set to O state
            L2[mod_cpu].dir[set][mod_way] = O;

            Bus += 2;
            cache_read++;
        }

        else if (shared_found == 1 || owner_found == 1)
        {
            //if in any cache  block with S state is found then just update new block,mark S and update counters.

            Bus += 2;
            cache_read++;
        }
        else if (excl_found == 1)
        {
            //if in any cache  block with E state is found then just update excl cpu block to S, mark current block S and update counters.
            L2[excl_cpu].dir[set][excl_way] = S;

            Bus += 2;
            cache_read++;
        }
        else
        { //C.if in any cache block is not found,get it from memory(that may bemiss or hit in LLC)
            //get from LLC

            return 1;
        }
    }

    if ((current_state == I || current_state == O) && op == PW)
    {
        // cout << "I and W \n";
        // if in any cache  block with M state is found then invalidate that block, mark S ,mark found block S and  update counters.(one memory write also in parrallel)
        if (mod_found == 1)
        {
            //set to I state
            L2[mod_cpu].update_timestamp(set, mod_way, 0, &timeStampCntr);
            invalidation++;
            Bus += 2;
            cache_read++;
        }
        else if (shared_found == 1 || owner_found == 1)
        {
            //Invalidate the blocks which was in shared state
            for (int i = 0; i < CORE_COUNT; i++)
            {
                if (state[i] == S)
                    {L2[i].update_timestamp(set, found_ways[i], 0, &timeStampCntr);invalidation++;}
            }
            //invalidaate O state block
            if (owner_found == 1 && owner_cpu != cpu)
            {
                L2[owner_cpu].update_timestamp(set, owner_way, 0, &timeStampCntr);
                invalidation++;
                Bus += 1;
                cache_read++;
            }

            Bus += 1;
        }
        else
        { //C.if in any cache block is not found,get it from memory(that may bemiss or hit in LLC)
            //get from LLC

            return 1;
        }
    }

    //Current State = S and want to write
    if (current_state == S && op == PW)
    {

        // cout << "S and W \n";
        if (mod_found == 1)
        {
            // cout << "S and W modfound\n";
            L2[mod_cpu].update_timestamp(set, mod_way, 0, &timeStampCntr);
            invalidation++;
            Bus += 2;
            cache_read++;
        }
        else if (shared_found == 1 || owner_found == 1)
        {
            // cout << "S and W shared found\n";
            for (int i = 0; i < CORE_COUNT; i++)
            {
                // cout << "inside loop\n";
                if (state[i] == S && (i != cpu))
                   { L2[i].update_timestamp(set, found_ways[i], 0, &timeStampCntr);invalidation++;}
            }

            //invalidaate O state block
            if(owner_found == 1 && owner_cpu != cpu)
               { L2[owner_cpu].update_timestamp(set, owner_way, 0, &timeStampCntr);invalidation++;}

            // cout << "out loop\n";

            Bus += 2;
            cache_read++;
        }
    }

    if (current_state == O && op == PW)
    {
        //O-> M state

        for (int i = 0; i < CORE_COUNT; i++)
        {
            // cout << "inside loop\n";
            if (state[i] == S && (i != cpu))
               { L2[i].update_timestamp(set, found_ways[i], 0, &timeStampCntr);invalidation++;  }
        }

            Bus += 2;
    }

    //Current State = E and want to read
    //Do nothing
    //Current State = E and want to write
    // if (current_state == E && op == PW)
    // {
    //   cout<<"E and Wr";
    //   L2[cpu].dir[set][way] = M;
    //   //Bus Transactions check with jatin
    //   Bus += 2;
    // }

    return 0;
}

/*Fuction to extract tag and set from given addr */
void extract_TagSet(int cacheID, unsigned long long addr, unsigned long long *tag, unsigned long long *set)
{
    int lShift = 0;
        //cacheID is L2
    if (cacheID == 0)
    {
        *set = ((addr>> 6) & (int)(pow(2,log2(L2_SETS))-1)) ;
        lShift = (log2(L2_SETS) + 6);
        *tag = (addr) >> lShift;
    }
    //cacheID is L3
    else if (cacheID == 1)
    {
        *set = ((addr>> 6) & (int)(pow(2,log2(L3_SETS))-1)) ;
        lShift = (log2(L3_SETS) + 6);
        *tag = (addr) >> lShift;
    }

}


int main(int argc, char **argv)
{

    FILE *fp;
    char input_name[100];
    unsigned long long addr, newAddr, blockAddr, way, tag, set, pc, count = 0;

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
        if(threadid >=  CORE_COUNT) continue;
       count++;
    if(count==service_time){
        if (q_empty <queue_size)
             q_empty++;

    }
    for(int q=0;q<CORE_COUNT;q++)
    {
      if(count==service_time){
        if (L2[q].q_empty <queue_size)
             L2[q].q_empty++;

          }
    }
        // cout << threadid << op << addr << "\n";
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

            //S and read -> do nothing

            //shared and Write
            if (current == S && op == PW)
            {
                R = Search_and_update(current, op, set, tag, threadid);
                L2[threadid].dir[set][way] = M;
            }

            //O and wr
            if (current == O && op == PW)
            {
                R = Search_and_update(current, op, set, tag, threadid);
                L2[threadid].dir[set][way] = M;
            }
            //O and Read -> do nothing

            //E and write
            if (current == E && op == PW)
            {
                L2[threadid].dir[set][way] = M;
            }
            //E and Read -> do nothing

            //M and Rd and Wr -> do nothing

            //L2 Hit
        //   cout << "L2 Hit found at way " << way;

            L2[threadid].hitCount++;
            //update lru L2 timestamp
            L2[threadid].update_timestamp(set, way, 1, &timeStampCntr);
        }
        else
        {
            /********* L2 miss,check for read/write and search in other caches and modify states accordingly******/
            //L2 Miss

            R = Search_and_update(I, op, set, tag, threadid);
            // cout << "L2 Miss found at way " << way;
            L2[threadid].missCount++;
            if (R == 1)
            {
                // cout << "R=1 " << way;
                //extract bits according to L3
                extract_TagSet(L3_ID, addr, &tag, &set);
                //search in L3
                way = LLC.search(set, tag);
                //way return -1 if not present in L2
                if (way != -1)
                {
                    //L3 Hit
                    // cout << "L3 Hit" << way;
                    LLC.hitCount++;
                    //update LRU L3
                    LLC.update_timestamp(set, way, 1, &timeStampCntr);
                    //find L2 address and extract tag,set
                    extract_TagSet(L2_ID, addr, &tag, &set);
                    //Query L2 LRU for way no., fill L2 cache and Update LRU L2
                    way = L2[threadid].insert(set, tag, &timeStampCntr);
                    if (L2[threadid].dir[set][way] == M || L2[threadid].dir[set][way] == O)
                        mem_write++;
                    if (op == PR)
                    {
                        L2[threadid].dir[set][way] = E;
                    }
                    else
                        L2[threadid].dir[set][way] = M;
                }
                else
                {
                    //L3 Miss
                    // cout << "L3 Miss" << way;
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
                        if (L2[threadid].dir[set][way] == M || L2[threadid].dir[set][way] == O)
                            mem_write++;
                        if (op == PR)
                            L2[threadid].dir[set][way] = E;
                        else
                            L2[threadid].dir[set][way] = M;
                    }
                    else
                    //if conflict happens in L3 at way
                    {
                        // cout << "conflict at L3 " << way;
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
                        if (L2[threadid].dir[set][way] == M || L2[threadid].dir[set][way] == O)
                            mem_write++;
                        if (op == PR)
                            L2[threadid].dir[set][way] = E;
                        else
                            L2[threadid].dir[set][way] = M;
                    }
                }
            }
            else
            {
                way = L2[threadid].insert(set, tag, &timeStampCntr);
                if (L2[threadid].dir[set][way] == M || L2[threadid].dir[set][way] == O)
                    mem_write++;
                if (op == PR)
                    L2[threadid].dir[set][way] = S;
                else
                    L2[threadid].dir[set][way] = M;
            }
        }
    }
    fclose(fp);

     printf("\n\nUsing MESI POLICY");
  for (int i = 0; i < CORE_COUNT; i++)
  {
    cout << " L2[" << i << "]Hit:" << L2[i].hitCount << " L2 Miss:" << L2[i].missCount;
    cout << " L2[" << i << "]:" <<"waiting time" << L2[i].waiting_time<<endl;
  }
  cout << " L3 Hit:" << LLC.hitCount << " L3 Miss: " << LLC.missCount << " Bus transactions: " << Bus << " Cache reads: " << cache_read<<" Total bus wait time: "<<waiting_time<<" mem_write "<<mem_write<<" invalidation "<<invalidation;
}
