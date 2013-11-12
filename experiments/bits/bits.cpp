/**
    Messing around with bit twiddling for the paper "Single Pass GPU Voxelization" section 6.2

    helpfull resource for bitwise operations:
    http://graphics.stanford.edu/~seander/bithacks.html
*/

#include <iostream>

using namespace std;

void pairwise_sum(unsigned int c, unsigned int r, const string& test_name ){
    static const unsigned int odd_mask  = 0xAAAAAAAA; // odd mask  = 1010 1010 ...
    static const unsigned int even_mask = 0x55555555; // even mask = 0101 0101 ...
    unsigned int d = (c & even_mask) + ((c & odd_mask)>>1); // from the paper mentioned above in section 6.2
    cout << test_name << " " << boolalpha << (r == d) << endl;
}

void pairwise_sum_sep(unsigned int a, unsigned int b, unsigned int r, const string& test_name ){
    // the values coming in will have counters between [0-2] contained in 2 bits
    // the sumed vaules would be stored in 4 bits
    // if  a : 0010 0010 0001 0001
    // and b : 0001 0010 0010 0001 and then the sum of these two would be
    //   sum : 0011 0100 0011 0010
    static const unsigned int mask_33 = 0x33333333;   // 0011 0011 ...
    static const unsigned int mask_CC = 0xCCCCCCCC;   // 1100 1100 ...
    static const unsigned int odd_mask  = 0xAAAAAAAA; // odd mask  = 1010 1010 ...
    static const unsigned int even_mask = 0x55555555; // even mask = 0101 0101 ...
    unsigned int x = a & mask_33;
    unsigned int y = b & mask_CC;
    unsigned int d = 0;

    d |= (a & even_mask) + ((a & odd_mask)>>1); // from the paper mentioned above in section 6.2
    d |= (b & even_mask) + ((b & odd_mask)>>1);
    cout << test_name << " " << boolalpha << (r == d) << endl;
}

int main()
{
    // intermedite density map, 1 voxel is 2 bits, 8 bits total, 4 voxels

    // column of colors, 1 voxel is 1 bit, 8 bits total, 8 voxels
    unsigned int c;
    unsigned int ans; // what the end result should be.
    {
        c = 0x03;   // 0000 0011
        ans = 0x02; // 0000 0010
        pairwise_sum(c, ans, "Test 1");
    }

    {
        c = 0x0F;   // 0000 1111
        ans = 0x0A; // 0000 1010
        pairwise_sum(c, ans, "Test 2");
    }

    {
        c = 0xA5;   // 1010 0101
        ans = 0x55; // 0101 0101
        pairwise_sum(c, ans, "Test 3");
    }

    {
        c = 0x50;   // 0101 0000
        ans = 0x50; // 0101 0000
        pairwise_sum(c, ans, "Test 4");
    }

    {
        c = 0xBE;   // 1011 1110
        ans = 0x69; // 0110 1001
        pairwise_sum(c, ans, "Test 5");
    }

    {
        c = 0xFE;   // 1111 1110
        ans = 0xA9; // 1010 1001
        pairwise_sum(c, ans, "Test 6");
    }

    {   // combining tests 1,2,3,6
        c = 0x030FA5FE;
        ans = 0x020A55A9;
        pairwise_sum(c, ans, "Test 7");
    }


    {   // combining tests 1,2,3,6
        unsigned int a = ;  //
        unsigned int b = ;  //
        ans = 0x020A55A9;
        pairwise_sum(a, b, ans, "Test 10");
    }
    return 0;
}
