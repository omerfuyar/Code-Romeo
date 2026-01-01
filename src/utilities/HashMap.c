#include "utilities/HashMap.h"

#define HashMap_Min(a, b) ((a) < (b) ? (a) : (b))
#define HashMap_Max(a, b) ((a) > (b) ? (a) : (b))

#pragma region Source Only

/// @brief Hash function for HashMap.
/// @param map The HashMap to hash for.
/// @param key The key to hash.
/// @return The hash value.
static RJ_Size HashMap_Hash(const HashMap *map, const char *key)
{
    RJ_Size strLength = (RJ_Size)strlen(key);

    RJ_Size sum = 0;
    RJ_Size mul = 1;

    for (RJ_Size i = 0; i < strLength; i++)
    {
        mul = (i % 4 == 0) ? 1 : mul * 256;
        sum += (RJ_Size)key[i] * mul;
    }

    return sum % map->capacity;
}

#pragma endregion Source Only

RJ_Return HashMap_Create(HashMap *retHashMap, const char *title, RJ_Size sizeOfItem, RJ_Size capacity)
{
    retHashMap->capacity = capacity;
    retHashMap->sizeOfItem = sizeOfItem;

    RJ_Size nameOfTypeLength = (RJ_Size)strlen(title);

    if (title == NULL)
    {
        title = "HashMap";
    }

    size_t titleLength = HashMap_Min(HASH_MAP_MAX_TITLE_LENGTH - 1, strlen(title));
    if (titleLength >= HASH_MAP_MAX_TITLE_LENGTH)
    {
        RJ_DebugWarning("HashMap title '%s' is longer than the maximum length of %d characters. It will be truncated.", title, HASH_MAP_MAX_TITLE_LENGTH - 1);
    }

    retHashMap->count = 0;
    if (!RJ_Allocate(char, retHashMap->data, capacity *sizeOfItem))
    {
        RJ_DebugWarning("Failed to allocate HashMap data for '%s'.", title);
        return RJ_ERROR_ALLOCATION;
    }

    RJ_DebugInfo("HashMap '%s' created with initial capacity: %u, size of item: %u", title, capacity, sizeOfItem);

    return RJ_OK;
}

void HashMap_Destroy(HashMap *map)
{
    RJ_DebugAssertNullPointerCheck(map);
    RJ_DebugAssertNullPointerCheck(map->data);

    free(map->data);
    map->data = NULL;

    map->capacity = 0;
    map->count = 0;
    map->sizeOfItem = 0;

    RJ_DebugInfo("HashMap '%s' destroyed.", map->title);
    map->title[0] = '\0';
}

bool HashMap_Contains(const HashMap *map, const char *key)
{
    RJ_DebugAssertNullPointerCheck(map);
    RJ_DebugAssertNullPointerCheck(key);

    RJ_Size hashIndex = HashMap_Hash(map, key);
    void *targetLocation = (void *)((char *)map->data + hashIndex * map->sizeOfItem);

    return targetLocation != NULL;
}

void HashMap_Register(HashMap *map, const char *key, const void *value)
{
    RJ_DebugAssertNullPointerCheck(map);
    RJ_DebugAssertNullPointerCheck(key);
    RJ_DebugAssertNullPointerCheck(value);

    void *targetLocation = (void *)((char *)map->data + HashMap_Hash(map, key) * map->sizeOfItem);

    memcpy(targetLocation, value, map->sizeOfItem);

    map->count++;
}

void *HashMap_Access(const HashMap *map, const char *key)
{
    RJ_DebugAssertNullPointerCheck(map);
    RJ_DebugAssertNullPointerCheck(key);

    RJ_DebugAssert(HashMap_Contains(map, key), "Key '%s' not found in HashMap '%s'.", key, map->title);

    return (void *)((char *)map->data + HashMap_Hash(map, key) * map->sizeOfItem);
}