# Dynamic-bitset
A header-only resizable container for storing binary data, with a guarantee that each 1 bit takes up 1/8th of a byte. Also allows for saving/retrieving of trivially_copyable types.

The bits here are stored as part of chars, which in turn are stored inside a vector. This ensures that 1 bit is actually taking up the space of 1 bit, as opposed to std::bitset. This also means that getting/retrieving values is going to be slightly slower than std::bitset. If you know at compile time what size the bitset is going to be, it is highly recommended to use std::bitset. If you don't need to store/retrieve triviably copyable types and/or entire bytes in binary format, it is recommended to use std::vector<bool>.
