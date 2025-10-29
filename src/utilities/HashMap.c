#include "utilities/HashMap.h"
#include "utilities/Maths.h"

#pragma region Source Only

size_t HashMap_Hash(const char *key, size_t capacity)
{
    size_t strLength = strlen(key);

    size_t sum = 0;
    size_t mul = 1;

    for (size_t i = 0; i < strLength; i++)
    {
        mul = (i % 4 == 0) ? 1 : mul * 256;
        sum += (size_t)key[i] * mul;
    }

    return sum % capacity;
}

#pragma endregion Source Only

HashMap HashMap_Create(const char *nameOfType, size_t sizeOfItem, size_t capacity)
{
    HashMap map;
    map.capacity = capacity;
    map.sizeOfItem = sizeOfItem;

    size_t nameOfTypeLength = strlen(nameOfType);

    map.nameOfType = (char *)malloc(nameOfTypeLength + 1);
    DebugAssertNullPointerCheck(map.nameOfType);
    MemoryCopy(map.nameOfType, nameOfTypeLength + 1, nameOfType);
    map.nameOfType[nameOfTypeLength] = '\0';

    map.count = 0;
    map.data = (void *)malloc(capacity * sizeOfItem);
    DebugAssertNullPointerCheck(map.data);

    MemorySet(map.data, map.sizeOfItem * map.capacity, 0);

    DebugInfo("HashMap '%s' created with initial capacity: %zu, size of item: %zu", nameOfType, capacity, sizeOfItem);

    return map;
}

void HashMap_Destroy(HashMap *map)
{
    DebugAssertNullPointerCheck(map);
    DebugAssertNullPointerCheck(map->data);

    size_t nameOfTypeLength = strlen(map->nameOfType);

    char tempTitle[RJ_TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, Min(RJ_TEMP_BUFFER_SIZE - 1, nameOfTypeLength), map->nameOfType);
    tempTitle[Min(RJ_TEMP_BUFFER_SIZE - 1, nameOfTypeLength)] = '\0';

    free(map->nameOfType);
    map->nameOfType = NULL;

    free(map->data);
    map->data = NULL;

    map->capacity = 0;
    map->count = 0;
    map->sizeOfItem = 0;

    DebugInfo("HashMap '%s' destroyed.", tempTitle);
}

void HashMap_Register(HashMap *map, const char *key, const void *value)
{
    DebugAssertNullPointerCheck(map);

    void *targetLocation = (void *)((char *)map->data + HashMap_Hash(key, map->capacity) * map->sizeOfItem);

    MemoryCopy(targetLocation, map->sizeOfItem, value);

    map->count++;
}

void *HashMap_Access(const HashMap *map, const char *key)
{
    return (void *)((char *)map->data + HashMap_Hash(key, map->capacity) * map->sizeOfItem);
}
