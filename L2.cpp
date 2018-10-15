#include <iostream>
using namespace std;

#define L2_SETS 2048
#define L2_WAYS 8

class L2
{
    int way, set;
    unsigned long hitCount, missCount;
    unsigned long tag[L2_SETS][L2_WAYS];
    char directory[L2_SETS][L2_WAYS];
    unsigned long timeStamp[L2_SETS][L2_WAYS];
}

L2::L2()
{
    hitCount = 0, missCount = 0;
    //clearing L2 cache
    for (int i = 0; i < L2_SETS; i++)
    {
        for (int j = 0; j < L2_WAYS; j++)
        {
            tag[i][j] = 0;
        }
    }

    //clearing L2 Timestamp
    for (int i = 0; i < L2_SETS; i++)
    {
        for (int j = 0; j < L2_WAYS; j++)
        {
            Time_Stamp_L2[i][j] = 0;
        }
    }

    for (int i = 0; i < L2_SETS; i++)
    {
        for (int j = 0; j < L2_WAYS; j++)
        {
            directory[i][j] = 0;
        }
    }

}

L2::