/**
    Messing around with bit twiddling for the paper "Single Pass GPU Voxelization" section 6.2

    helpfull resource for bitwise operations:
    http://graphics.stanford.edu/~seander/bithacks.html
*/

#include <iostream>
#include <bitset>

using namespace std;

void pair_sidewise_sum(unsigned int c, unsigned int r, const string& test_name ){
    static const unsigned int odd_mask  = 0xAAAAAAAA; // odd mask  = 1010 1010 ...
    static const unsigned int even_mask = 0x55555555; // even mask = 0101 0101 ...
    unsigned int d = (c & even_mask) + ((c & odd_mask)>>1); // from the paper mentioned above in section 6.2
    cout << test_name << " " << boolalpha << (r == d) << endl;
}

void parallel_sum(unsigned int x0, unsigned int x1, unsigned int r0, unsigned int r1, const string& test_name ){
    // the values coming in will have counters between [0-2] contained in 2 bits
    // the sumed vaules would be stored in 4 bits
    // if  a : 0010 0010 0001 0001
    // and b : 0001 0010 0010 0001 and then the sum of these two would be
    //   sum : 0011 0100 0011 0010
    // The values are stored into two variables "even" and "odd"
    // They hold the sums of the bit pair they relate to.
    static const unsigned int even_mask = 0x33333333;   // 0011 0011 ...
    static const unsigned int odd_mask  = 0xCCCCCCCC;   // 1100 1100 ...
    unsigned int even = 0;
    unsigned int odd = 0;

    // sum the lower bits of each nibble
    even = (x0 & even_mask) + (x1 & even_mask);
    // sum the upper bits of each nibble
    odd  = ((x0 & odd_mask) >> 2) + ((x1 & odd_mask) >> 2);

    std::bitset<8> pa(x0);
    std::bitset<8> pb(x1);
    std::bitset<8> e(even);
    std::bitset<8> o(odd);
    cout << test_name << boolalpha << " " << (r0 == even) << " " << (r1 == odd)
         << "\n\t" << "a: " << pa << "    " << pa
         << "\n\t" << "b: " << pb << "    " << pb
         << "\n\t" << "e: " << e  << " o: " << o
         << "\n\t" << (r0 == even) << " " << (r1 == odd)
         << endl;
    std::bitset<32> total(r0+r1);
    cout << total << endl;
}

void interleave_bit_nibbles(unsigned int even, unsigned int odd, unsigned int r, const string& test_name){
    // http://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
    static const unsigned int B[] = {0x0F0F0F0F, 0x00FF00FF};
    static const unsigned int S[] = {4, 8};

    unsigned int x; // Interleave lower 16 bits of x and y, so the bits of x
    unsigned int y; // are in the even positions and bits from y in the odd;
    unsigned int z; // z gets the resulting 32-bit Morton Number.
                    // x and y must initially be less than 65536.

    x = even;
    y = odd;

    x = (x | (x << S[1])) & B[1];
    x = (x | (x << S[0])) & B[0];

    y = (y | (y << S[1])) & B[1];
    y = (y | (y << S[0])) & B[0];

    z = x | (y << 4);

    cout << test_name << boolalpha << " " <<(z==r)
         << "\n\t" << bitset<32>(z)
         << "\n\t" << hex << z
         << endl;
}

int main()
{
    cout << "All of the following tests should return true" << endl;

    // column of colors, 1 voxel is 1 bit, 8 bits total, 8 voxels
    unsigned int c;
    unsigned int ans; // what the end result should be.
    {
        c = 0x03;   // 0000 0011
        ans = 0x02; // 0000 0010
        pair_sidewise_sum(c, ans, "Test 1");
    }

    {
        c = 0x0F;   // 0000 1111
        ans = 0x0A; // 0000 1010
        pair_sidewise_sum(c, ans, "Test 2");
    }

    {
        c = 0xA5;   // 1010 0101
        ans = 0x55; // 0101 0101
        pair_sidewise_sum(c, ans, "Test 3");
    }

    {
        c = 0x50;   // 0101 0000
        ans = 0x50; // 0101 0000
        pair_sidewise_sum(c, ans, "Test 4");
    }

    {
        c = 0xBE;   // 1011 1110
        ans = 0x69; // 0110 1001
        pair_sidewise_sum(c, ans, "Test 5");
    }

    {
        c = 0xFE;   // 1111 1110
        ans = 0xA9; // 1010 1001
        pair_sidewise_sum(c, ans, "Test 6");
    }

    {   // combining tests 1,2,3,6
        c = 0x030FA5FE;
        ans = 0x020A55A9;
        pair_sidewise_sum(c, ans, "Test 7");
    }

    {
        unsigned int a = 0xA5;  // 1010 0101
        unsigned int b = 0x66;  // 0110 0110

        unsigned int r0 = 0x43; // 0100 0011
        unsigned int r1 = 0x32; // 0011 0010 (its shifted by >> 2)

        parallel_sum(a, b, r0, r1, "Test 10");
    }

    {
        unsigned int a = 0x95;  // 1001 0101
        unsigned int b = 0xA6;  // 1010 0110

        unsigned int r0 = 0x33; // 0011 0011
        unsigned int r1 = 0x42; // 0100 0010 (its shifted by >> 2)

        parallel_sum(a, b, r0, r1, "Test 11");
    }

    {
        unsigned int a = 0xAA;  // 1010 1010
        unsigned int b = 0xAA;  // 1010 1010

        unsigned int r0 = 0x44; // 0100 0100
        unsigned int r1 = 0x44; // 0100 0100 (its shifted by >> 2)

        parallel_sum(a, b, r0, r1, "Test 12");
    }

    {
        unsigned int a = 0x59;  // 0101 1001
        unsigned int b = 0x59;  // 0101 1001

        unsigned int r0 = 0x22; // 0010 0010
        unsigned int r1 = 0x24; // 0010 0100 (its shifted by >> 2)

        parallel_sum(a, b, r0, r1, "Test 13");
    }

    {   // combining tests 10 - 13
        unsigned int a = 0xA595AA59;  // 0101 1001
        unsigned int b = 0x66A6AA59;  // 0101 1001

        unsigned int r0 = 0x43334422; // 0010 0010
        unsigned int r1 = 0x32424424; // 0010 0100 (its shifted by >> 2)

        // 0111 0101 0111 0101 1000 1000 0100 0110 from console
        // 0111 0101 0111 0101 1000 1000 0100 0110 my sum
        parallel_sum(a, b, r0, r1, "Test 14");
    }

    {
        unsigned int a = 0xFF;
        unsigned int b = 0xFF;
        unsigned int r = 0xFFFF;
        interleave_bit_nibbles(a, b, r, "Test 15 - interleaving nibbles:");
    }

    {
        unsigned int a = 0xF;
        unsigned int b = 0xF;
        unsigned int r = 0xFF;
        interleave_bit_nibbles(a, b, r, "Test 16 - interleaving nibbles:");
    }

    {
        unsigned int a = 0x1;
        unsigned int b = 0x1;
        unsigned int r = 0x11;
        interleave_bit_nibbles(a, b, r, "Test 17 - interleaving nibbles:");
    }

    {
        unsigned int a = 0xBDF0;
        unsigned int b = 0xACE0;
        unsigned int r = 0xABCDEF00;
        interleave_bit_nibbles(a, b, r, "Test 18 - interleaving nibbles:");
    }

    {
        unsigned int a = 0x80;
        unsigned int b = 0x80;
        unsigned int r = 0x8800;
        interleave_bit_nibbles(a, b, r, "Test 19 - interleaving nibbles:");
    }

    {
        unsigned int a = 0xABCD;
        unsigned int b = 0x3892;
        unsigned int r = 0x3A8B9C2D;
        interleave_bit_nibbles(a, b, r, "Test 20 - interleaving nibbles:");
    }

    {
        unsigned int a = 0xABCD0000;
        unsigned int b = 0x38920000;
        unsigned int r = 0x00000000;
        interleave_bit_nibbles(a&0xFFFF, b&0xFFFF, r, "Test 21 - interleaving nibbles:");
    }

    {
        unsigned int a = 0xABCD0000;
        unsigned int b = 0x38920000;
        unsigned int r = 0x3A8B9C2D;
        interleave_bit_nibbles(a>>16, b>>16, r, "Test 22 - interleaving nibbles:");
    }

    return 0;
}
