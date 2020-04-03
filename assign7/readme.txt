File: readme.txt
Author: DEREK CHUNG
----------------------

implicit
--------
I decided to store the size of each block in the header. Each header would therefore
require at least 8 byes of space. However, since the 3 least significant bits of the size would
always be zero, I changed the least significant bit to 1 whenever a block was malloced and
back to zero when it was freed. This way, I only need 8 byte headers. To malloc, I scan through
all the blocks with zero as the least significant bit and find the size that will fit. For free,
I set the least significant bit to 0. For realloc, I called free and then malloc.

explicit
--------
For explicit, I needed all blocks to be at least 24 bytes. I need 8 bytes for the header,
8 for the previous free pointer, and 8 for the next free pointer. I created a pointer struct
that stores the previous and the next free pointer. Mymalloc would loop through the free pointers
and return the address that leaves the least free blocks remaining after the free block was malloced over.
My free set the header bit to 0 and coalesced the free blocks. Realloc was a bit tricky. I returned the same
pointer if the size is zero or the size is the same. If the new size was less, I reduced the size only if
there is 24 or more free bytes remaining after the resize OR if the preceding block is free. If the new size was more,
I looked at the very next block. If the next block could fulfill the request, I chopped off part of that free
block for the new request. Else, I combined the entire block and called the function realloc again. If the next block
wasn't free and the realloc request cannot be fulfilled, I free the memory and call mymalloc again.


Tell us about your quarter in CS107!
-----------------------------------
It was a very fun quarter. I learned a lot, even if the assignments were hard at times.


