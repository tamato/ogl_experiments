#version 430

out uvec4 Frag;

void main() {
    uvec2 uv = uvec2(gl_FragCoord.xy);
    uint v = 0xFFFFFFFFU >> uv.x;
    Frag = uvec4(v, 0, 0, 0);
}

/*
    const GLuint depth_mask = 0xFFFFFFFF;
    for (GLuint i=0; i<column_length; ++i)
    {
        GLuint red_mask = 0;
        GLuint green_mask = 0;
        GLuint blue_mask = 0;
        GLuint alpha_mask = 0;

        if (i < 32){
            red_mask   = depth_mask >> (i);
            green_mask = depth_mask;
            blue_mask  = depth_mask;
            alpha_mask = depth_mask;
        }
        else if (i < 64){
            green_mask = depth_mask >> (i - 32);
            blue_mask  = depth_mask;
            alpha_mask = depth_mask;
        }
        else if (i < 96){
            blue_mask  = depth_mask >> (i - 64);
            alpha_mask = depth_mask;
        }
        else if (i < 128){
           alpha_mask = depth_mask >> (i - 96);
        }

        data[i*stride + R] = red_mask;
        data[i*stride + G] = green_mask;
        data[i*stride + B] = blue_mask;
        data[i*stride + A] = alpha_mask;
        // cout << bitset<32>(red_mask)
        //      << bitset<32>(green_mask)
        //      << bitset<32>(blue_mask)
        //      << bitset<32>(alpha_mask)
        //      << endl;
    }
    */