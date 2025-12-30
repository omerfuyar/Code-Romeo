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

HashMap HashMap_Create(const char *nameOfType, RJ_Size sizeOfItem, RJ_Size capacity)
{
    HashMap map;
    map.capacity = capacity;
    map.sizeOfItem = sizeOfItem;

    RJ_Size nameOfTypeLength = (RJ_Size)strlen(nameOfType);

    RJ_DebugAssertAllocationCheck(char, map.nameOfType, nameOfTypeLength + 1);
    memcpy(map.nameOfType, nameOfType, nameOfTypeLength + 1);
    map.nameOfType[nameOfTypeLength] = '\0';

    map.count = 0;
    RJ_DebugAssertAllocationCheck(char, map.data, capacity *sizeOfItem);

    RJ_DebugInfo("HashMap '%s' created with initial capacity: %u, size of item: %u", nameOfType, capacity, sizeOfItem);

    return map;
}

void HashMap_Destroy(HashMap *map)
{
    RJ_DebugAssertNullPointerCheck(map);
    RJ_DebugAssertNullPointerCheck(map->data);

    RJ_Size nameOfTypeLength = (RJ_Size)strlen(map->nameOfType);

    char tempTitle[RJ_TEMP_BUFFER_SIZE] = {0};
    memcpy(tempTitle, map->nameOfType, HashMap_Min(RJ_TEMP_BUFFER_SIZE - 1, nameOfTypeLength));
    tempTitle[HashMap_Min(RJ_TEMP_BUFFER_SIZE - 1, nameOfTypeLength)] = '\0';

    free(map->nameOfType);
    map->nameOfType = NULL;

    free(map->data);
    map->data = NULL;

    map->capacity = 0;
    map->count = 0;
    map->sizeOfItem = 0;

    RJ_DebugInfo("HashMap '%s' destroyed.", tempTitle);
}

void HashMap_Register(HashMap *map, const char *key, const void *value)
{
    RJ_DebugAssertNullPointerCheck(map);

    void *targetLocation = (void *)((char *)map->data + HashMap_Hash(map, key) * map->sizeOfItem);

    memcpy(targetLocation, value, map->sizeOfItem);

    map->count++;
}

void *HashMap_Access(const HashMap *map, const char *key)
{
    return (void *)((char *)map->data + HashMap_Hash(map, key) * map->sizeOfItem);
}
