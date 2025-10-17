#include "utilities/HashMap.h"

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
    map.nameOfType = malloc(strlen(nameOfType) + 1);
    MemoryCopy(map.nameOfType, strlen(nameOfType) + 1, nameOfType);
    map.nameOfType[strlen(nameOfType)] = '\0';
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

    const char *name = map->nameOfType;

    free(map->data);
    map->data = NULL;

    free(map->nameOfType);
    map->nameOfType = NULL;
    map->capacity = 0;
    map->count = 0;
    map->sizeOfItem = 0;

    DebugInfo("HashMap '%s' destroyed.", name);
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
