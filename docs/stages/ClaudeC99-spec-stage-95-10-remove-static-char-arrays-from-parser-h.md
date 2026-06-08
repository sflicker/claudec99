# ClaudeC99 Stage 95-10 convert static char arrays in parser.h to const char *

## Task
parser.h has numerous struct that contain an char array with a static size of MAX_NAME_LEN
convert these to use `const char *`. 
replace copy operations like memcpy and strncpy to assignments and other relevant code.


Since there are numbers structs with this form in parser.h make a checklist with an item for each
conversion. After each conversion run the test suite. after it successfully passes commit
the code changes for that conversions, update the checklist then repeat for the next item.

After all the conversions update the fixed-capacity-inventory.md

## Tests
add tests where appropriate to ensure test converage
