#pragma once
#include <Global.h>
#include "utilities/String.h"

typedef struct Resource
{
    String title;
    String path;
    void *data;
    size_t dataSize;
} Resource;

typedef struct ResourceArray
{
    Resource *resources;
    size_t length;
    size_t capacity;
} ResourceArray;
