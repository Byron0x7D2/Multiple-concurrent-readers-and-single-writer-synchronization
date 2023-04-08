#pragma once
#include "sharedmemory.h"

void init(sharedmemory mem, int linespersegment, int maxline, int segments);
void del(sharedmemory mem, int linespersegment);
