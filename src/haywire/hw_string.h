#include "haywire.h"

hw_string* create_string(const char* value);
void append_string(hw_string* destination, hw_string* source);
char* dupstr(const char *s);
void string_from_int(hw_string* str, int val, int base);

void free_string(hw_string* str);
