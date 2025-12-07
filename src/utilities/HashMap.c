#include "utilities/HashMap.h"

#define HashMap_Min(a, b) ((a) < (b) ? (a) : (b))
#define HashMap_Max(a, b) ((a) > (b) ? (a) : (b))

#pragma region Source Only

/// @brief Hash function for HashMap.
/// @param map The HashMap to hash for.
/// @param key The key to hash.
/// @return The hash value.
RJGlobal_Size HashMap_Hash(const HashMap *map, const char *key)
{
    RJGlobal_Size strLength = RJGlobal_StringLength(key);

    RJGlobal_Size sum = 0;
    RJGlobal_Size mul = 1;

    for (RJGlobal_Size i = 0; i < strLength; i++)
    {
        mul = (i % 4 == 0) ? 1 : mul * 256;
        sum += (RJGlobal_Size)key[i] * mul;
    }

    return sum % map->capacity;
}

#pragma endregion Source Only

HashMap HashMap_Create(const char *nameOfType, RJGlobal_Size sizeOfItem, RJGlobal_Size capacity)
{
    HashMap map;
    map.capacity = capacity;
    map.sizeOfItem = sizeOfItem;

    RJGlobal_Size nameOfTypeLength = RJGlobal_StringLength(nameOfType);

    RJGlobal_DebugAssertAllocationCheck(char, map.nameOfType, nameOfTypeLength + 1);
    RJGlobal_MemoryCopy(map.nameOfType, nameOfTypeLength + 1, nameOfType);
    map.nameOfType[nameOfTypeLength] = '\0';

    map.count = 0;
    RJGlobal_DebugAssertAllocationCheck(char, map.data, capacity *sizeOfItem);

    RJGlobal_DebugInfo("HashMap '%s' created with initial capacity: %u, size of item: %u", nameOfType, capacity, sizeOfItem);

    return map;
}

void HashMap_Destroy(HashMap *map)
{
    RJGlobal_DebugAssertNullPointerCheck(map);
    RJGlobal_DebugAssertNullPointerCheck(map->data);

    RJGlobal_Size nameOfTypeLength = RJGlobal_StringLength(map->nameOfType);

    char tempTitle[RJGLOBAL_TEMP_BUFFER_SIZE];
    RJGlobal_MemoryCopy(tempTitle, HashMap_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, nameOfTypeLength), map->nameOfType);
    tempTitle[HashMap_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, nameOfTypeLength)] = '\0';

    free(map->nameOfType);
    map->nameOfType = NULL;

    free(map->data);
    map->data = NULL;

    map->capacity = 0;
    map->count = 0;
    map->sizeOfItem = 0;

    RJGlobal_DebugInfo("HashMap '%s' destroyed.", tempTitle);
}

void HashMap_Register(HashMap *map, const char *key, const void *value)
{
    RJGlobal_DebugAssertNullPointerCheck(map);

    void *targetLocation = (void *)((char *)map->data + HashMap_Hash(map, key) * map->sizeOfItem);

    RJGlobal_MemoryCopy(targetLocation, map->sizeOfItem, value);

    map->count++;
}

void *HashMap_Access(const HashMap *map, const char *key)
{
    return (void *)((char *)map->data + HashMap_Hash(map, key) * map->sizeOfItem);
}
