const unsigned char CartROM[] = { // REMEMBER: THIS IS LOADED AT $C000
 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
 0x7e, 0xc1, 0x00, 0x7e, 0xc0, 0x71, 0x7e, 0xc0, 0xa8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
 0x8d, 0x09, 0x29, 0x43, 0x1f, 0x12, 0x86, 0x2d, 0xbd, 0xc0, 0xa8, 0x8d, 0x0f, 0x29, 0x38, 0x1f,
 0x01, 0x8d, 0x09, 0x29, 0x32, 0x34, 0x10, 0xa7, 0x61, 0x35, 0x10, 0x39, 0x8d, 0x11, 0x29, 0x27,
 0x48, 0x48, 0x48, 0x48, 0x1f, 0x89, 0x8d, 0x07, 0x29, 0x1d, 0x34, 0x04, 0xab, 0xe0, 0x39, 0x8d,
 0x19, 0x81, 0x30, 0x25, 0x12, 0x81, 0x39, 0x22, 0x03, 0x80, 0x30, 0x39, 0x81, 0x41, 0x25, 0x07,
 0x81, 0x46, 0x22, 0x03, 0x80, 0x37, 0x39, 0x1a, 0x02, 0x39, 0x8d, 0x05, 0x84, 0x7f, 0x7e, 0xc0,
 0xa8, 0xb6, 0xe1, 0x00, 0x47, 0x24, 0xfa, 0xb6, 0xe1, 0x01, 0x39, 0x8d, 0x06, 0x8d, 0x04, 0x86,
 0x20, 0x20, 0x25, 0xa6, 0x80, 0x34, 0x02, 0x44, 0x44, 0x44, 0x44, 0x8d, 0x02, 0x35, 0x02, 0x84,
 0x0f, 0x8b, 0x30, 0x81, 0x39, 0x2f, 0x02, 0x8b, 0x07, 0x20, 0x0d, 0x8d, 0x0b, 0xa6, 0x80, 0x26,
 0xfa, 0x39, 0x86, 0x0d, 0x8d, 0x02, 0x86, 0x0a, 0x34, 0x02, 0xb6, 0xe1, 0x00, 0x47, 0x47, 0x24,
 0xf9, 0x35, 0x02, 0xb7, 0xe1, 0x01, 0x39, 0x34, 0x02, 0xb6, 0xe1, 0x00, 0x85, 0x20, 0x35, 0x02,
 0x39, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
 0x1a, 0x50, 0x86, 0x1f, 0xb7, 0xe2, 0x02, 0x86, 0x10, 0xb7, 0xe2, 0x00, 0x10, 0xce, 0xbf, 0x80,
 0xbd, 0xf1, 0x5d, 0x86, 0x15, 0xb7, 0xe1, 0x00, 0x86, 0x34, 0xb7, 0xe2, 0x06, 0xcc, 0x00, 0x00,
 0x83, 0x00, 0x01, 0x26, 0xfb, 0x86, 0x18, 0xb7, 0xe2, 0x0f, 0x8e, 0xc1, 0x70, 0xbd, 0xc0, 0x9d,
 0xbd, 0xc1, 0xf0, 0x81, 0x42, 0x26, 0x05, 0xbd, 0xc1, 0xa0, 0x20, 0xee, 0x81, 0x44, 0x26, 0x05,
 0xbd, 0xc1, 0xfe, 0x20, 0xf5, 0x81, 0x4d, 0x26, 0x05, 0xbd, 0xc2, 0x40, 0x20, 0xf5, 0x81, 0x4c,
 0x26, 0x05, 0xbd, 0xc2, 0x90, 0x20, 0xf5, 0x81, 0x50, 0x26, 0x05, 0xbd, 0xc3, 0x30, 0x20, 0xf5,
 0x81, 0x4a, 0x26, 0x03, 0x7e, 0xc1, 0x90, 0x81, 0x51, 0x26, 0xf3, 0x7e, 0xf0, 0xf0, 0xff, 0xff,
 0x0d, 0x0a, 0x45, 0x6e, 0x73, 0x6f, 0x6e, 0x69, 0x71, 0x20, 0x4d, 0x6f, 0x6e, 0x69, 0x74, 0x6f,
 0x72, 0x20, 0x52, 0x4f, 0x4d, 0x0d, 0x0a, 0x3e, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
 0xbd, 0xc0, 0x2b, 0x34, 0x10, 0x39, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
 0xbd, 0xc1, 0xba, 0x29, 0x08, 0x1e, 0x12, 0x34, 0x20, 0xac, 0xe1, 0x25, 0x01, 0x39, 0xa6, 0x80,
 0xbd, 0xc0, 0xa8, 0x20, 0xf2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbd, 0xc0, 0x20, 0x28, 0x01, 0x39,
 0xcc, 0x00, 0x00, 0x83, 0x00, 0x01, 0x26, 0xfb, 0x86, 0x40, 0xbd, 0xc0, 0xa8, 0x39, 0xff, 0xff,
 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
 0xbd, 0xc0, 0x6a, 0x34, 0x02, 0xbd, 0xc0, 0x7f, 0x35, 0x02, 0x39, 0xff, 0xff, 0xff, 0xbd, 0xc0,
 0x20, 0x29, 0x08, 0x1e, 0x12, 0x34, 0x20, 0xac, 0xe1, 0x25, 0x01, 0x39, 0xbd, 0xc0, 0xb7, 0x26,
 0xfa, 0xbd, 0xc0, 0xa2, 0x34, 0x10, 0x1f, 0x41, 0xbd, 0xc0, 0x7b, 0xae, 0xe4, 0xc6, 0x10, 0xbd,
 0xc0, 0x7d, 0x5a, 0x26, 0xfa, 0xae, 0xe1, 0xc6, 0x10, 0xa6, 0x80, 0x81, 0x20, 0x25, 0x04, 0x81,
 0x7e, 0x23, 0x02, 0x86, 0x2e, 0xbd, 0xc0, 0xa8, 0x5a, 0x26, 0xee, 0x20, 0xc8, 0xff, 0xff, 0xff,
 0xbd, 0xc0, 0x2b, 0x29, 0x25, 0x34, 0x10, 0x8e, 0xc2, 0x84, 0xbd, 0xc0, 0x9d, 0x1f, 0x41, 0xbd,
 0xc0, 0x7b, 0x35, 0x10, 0x1f, 0x12, 0xbd, 0xc0, 0x7d, 0xbd, 0xc0, 0x3c, 0x28, 0x0d, 0x81, 0x08,
 0x27, 0x19, 0x81, 0x5e, 0x27, 0x19, 0x81, 0x0d, 0x26, 0x0f, 0x39, 0xa7, 0xa4, 0xa1, 0xa4, 0x27,
 0x08, 0xbd, 0xc0, 0x7f, 0x86, 0x3f, 0xbd, 0xc0, 0xa8, 0x31, 0x32, 0x1f, 0x21, 0x20, 0xc6, 0x31,
 0x3f, 0x20, 0xf8, 0xff, 0x0d, 0x0a, 0x2d, 0x20, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
 0xbd, 0xc3, 0x1b, 0x81, 0x53, 0x26, 0xf9, 0xbd, 0xc3, 0x1b, 0x81, 0x39, 0x27, 0x3d, 0x81, 0x31,
 0x26, 0xf1, 0xbd, 0xc2, 0xed, 0x34, 0x02, 0x29, 0x26, 0xbd, 0xc2, 0xdc, 0x29, 0x21, 0x34, 0x10,
 0xe6, 0xe0, 0xeb, 0xe0, 0xeb, 0xe4, 0x6a, 0xe4, 0x6a, 0xe4, 0x34, 0x04, 0xbd, 0xc2, 0xed, 0x35,
 0x04, 0x29, 0x0c, 0x34, 0x02, 0xeb, 0xe0, 0x6a, 0xe4, 0x27, 0x05, 0xa7, 0x80, 0x20, 0xeb, 0x5f,
 0x35, 0x02, 0xc1, 0xff, 0x27, 0xba, 0x86, 0x3f, 0xbd, 0xc0, 0xa8, 0x39, 0x8d, 0x0f, 0x29, 0x38,
 0x1f, 0x01, 0x8d, 0x09, 0x29, 0x32, 0x34, 0x10, 0xa7, 0x61, 0x35, 0x10, 0x39, 0x8d, 0x11, 0x29,
 0x27, 0x48, 0x48, 0x48, 0x48, 0x1f, 0x89, 0x8d, 0x07, 0x29, 0x1d, 0x34, 0x04, 0xab, 0xe0, 0x39,
 0x8d, 0x19, 0x81, 0x30, 0x25, 0x12, 0x81, 0x39, 0x22, 0x03, 0x80, 0x30, 0x39, 0x81, 0x41, 0x25,
 0x07, 0x81, 0x46, 0x22, 0x03, 0x80, 0x37, 0x39, 0x1a, 0x02, 0x39, 0xbd, 0xc0, 0x71, 0x84, 0x7f,
 0x39, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
 0x6f, 0xe2, 0xbd, 0xc0, 0x20, 0x34, 0x30, 0x29, 0x45, 0xac, 0x62, 0x25, 0x41, 0x30, 0x01, 0xaf,
 0xe4, 0xec, 0xe4, 0xa3, 0x62, 0x27, 0x06, 0x10, 0x83, 0x00, 0x20, 0x23, 0x02, 0xc6, 0x20, 0xe7,
 0x64, 0x8e, 0xc3, 0x8d, 0xbd, 0xc0, 0x9d, 0xcb, 0x03, 0x1f, 0x98, 0xbd, 0xc0, 0x85, 0xae, 0x62,
 0xbd, 0xc3, 0x81, 0xeb, 0x62, 0xeb, 0x63, 0xeb, 0x84, 0xa6, 0x80, 0xbd, 0xc0, 0x85, 0x6a, 0x64,
 0x26, 0xf5, 0x53, 0x1f, 0x98, 0xbd, 0xc0, 0x85, 0xaf, 0x62, 0xac, 0xe4, 0x26, 0xc3, 0x32, 0x65,
 0x39, 0x34, 0x10, 0x35, 0x02, 0xbd, 0xc0, 0x85, 0x35, 0x02, 0x7e, 0xc0, 0x85, 0x0d, 0x0a, 0x53,
 0x31, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
