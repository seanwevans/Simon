// parseutf8.c

typedef enum {
    UTF8_ACCEPT  = 0,
    UTF8_REJECT  = 1,
    UTF8_EXPECT1 = 2,
    UTF8_EXPECT2 = 3,
    UTF8_EXPECT3 = 4
} utf8_state;

typedef enum {
    UTF16_ACCEPT = 0,
    UTF16_REJECT = 1,
    UTF16_EXPECT_LOW_SURROGATE = 2
} utf16_state;

typedef enum {
    UTF32_ACCEPT = 0,
    UTF32_REJECT = 1
} utf32_state;

static const unsigned char utf8_transition_table[] = {
    UTF8_ACCEPT, UTF8_REJECT, UTF8_REJECT,  UTF8_REJECT,  UTF8_REJECT,  // 0xxxxxxx
    UTF8_REJECT, UTF8_REJECT, UTF8_EXPECT1, UTF8_EXPECT2, UTF8_EXPECT3  // 110xxxxx, 1110xxxx, 11110xxx
};

static const unsigned char utf16_transition[] = {
    UTF16_ACCEPT,                // ACCEPT + non-surrogate
    UTF16_ACCEPT,                // ACCEPT + low surrogate (invalid but recoverable)
    UTF16_EXPECT_LOW_SURROGATE,  // ACCEPT + high surrogate
    UTF16_ACCEPT,                // EXPECT_LOW + non-surrogate (error)
    UTF16_ACCEPT,                // EXPECT_LOW + low surrogate
    UTF16_EXPECT_LOW_SURROGATE   // EXPECT_LOW + high surrogate (error)
};

int validate_utf8(const unsigned char *input, const unsigned maxlen) {
    unsigned i = 0;
    utf8_state state = UTF8_ACCEPT;

    for (; i < maxlen && input[i] != '\0'; ++i) {

        unsigned type = (input[i] >> 3) & 0x1F;

        utf8_state next = utf8_transition_table[
            (type == 0x1E) +
            (type == 0x1F) + 
            (2 * (type >= 0x1C))
        ];

        state = (input[i] & 0xC0) == 0x80 ? state : next;

        if (state == UTF8_REJECT)
            return i;
    }
    
    return i;
}

int validate_utf16(const unsigned char *input, const unsigned maxlen) {
    unsigned i = 0;
    utf16_state state = UTF16_ACCEPT;    

    for (; i < maxlen - 1; i += 2) {
        
        unsigned curr = (input[i] << 8) | input[i + 1];
                
        if (curr == 0) 
            break;

        unsigned is_high_surrogate = (curr - 0xD800) < 0x400;
        unsigned is_low_surrogate = (curr - 0xDC00) < 0x400;
        
        unsigned combined = (state << 4) | (is_high_surrogate << 1) | is_low_surrogate;

        state = utf16_transition[combined & 0x07];
        
        if (state == UTF16_REJECT) 
            return i;
    }
    
    return i;
}


int validate_utf32(const unsigned char *input, const unsigned maxlen) {
    unsigned i = 0;
    utf32_state state = UTF32_ACCEPT;    

    for (; i < maxlen - 3; i += 4) {
        
        unsigned curr = (input[i]     << 24) | 
                        (input[i + 1] << 16) | 
                        (input[i + 2] <<  8) | 
                         input[i + 3];
                
        if (curr == 0) 
            break;
        
        unsigned is_surrogate_range = (curr - 0xD800) < 0x800;
        unsigned is_out_of_bounds = curr > 0x10FFFF;

        state |= is_surrogate_range | is_out_of_bounds;
        
        if (state == UTF32_REJECT) 
            return i;
    }
    
    return i;
}
