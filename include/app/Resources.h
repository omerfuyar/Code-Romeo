#pragma once

#include "Global.h"
#include "utilities/String.h"

#define RESOURCE_FILE_MAX_LINE_CHAR_COUNT 1024
#define RESOURCE_FILE_MAX_LINE_COUNT 128

typedef struct Resource
{
    String title;
    String path;
    void *data;
    size_t dataSize;
} Resource;

Resource *Resource_Create(String title, String path);

void Resource_Destroy(Resource *resource);
