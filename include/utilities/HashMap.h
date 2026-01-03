#pragma once

#include "RJGlobal.h"

#define HASH_MAP_MAX_TITLE_LENGTH (RJ_TEMP_BUFFER_SIZE / 8)

#pragma region Typedefs

/// @brief A dynamic hash map implementation. Can be used with any type. Copies passed items to its own property. Shouldn't be used without helper functions.
typedef struct HashMap
{
    void *data;
    RJ_Size capacity;
    RJ_Size count;
    RJ_Size sizeOfItem;
    char title[HASH_MAP_MAX_TITLE_LENGTH];
} HashMap;

#pragma endregion Typedefs

/// @brief Creator function for HashMap.
/// @param retHashMap Pointer to the HashMap to initialize.
/// @param title Name of the type stored in the hash map.
/// @param sizeOfItem Size of the item type to store.
/// @param capacity Initial capacity of the hash map.
/// @return RJ_OK on success, or RJ_ERROR_ALLOCATION if internal allocation failsRJ_ERROR_.
RJ_ResultDef HashMap_Create(HashMap *retHashMap, const char *title, RJ_Size sizeOfItem, RJ_Size capacity);

/// @brief Destroyer function for HashMap.
/// @param hashMap Pointer to the HashMap to destroy.
void HashMap_Destroy(HashMap *hashMap);

/// @brief Contains function for HashMap. Checks if a key exists in the map.
/// @param map Pointer to the HashMap to check.
/// @param key Key string to look for.
/// @return True if the key exists in the map, false otherwise.
bool HashMap_Contains(const HashMap *map, const char *key);

/// @brief Register function for HashMap. Adds or updates a key-value pair.
/// @param hashMap Pointer to the HashMap to register item in.
/// @param key Key string to associate with the value.
/// @param value Pointer to the value to store.
void HashMap_Register(HashMap *hashMap, const char *key, const void *value);

/// @brief Access function for HashMap. Retrieves a value by key.
/// @param hashMap Pointer to the HashMap to access item from.
/// @param key Key string to look up.
/// @return Pointer to the value associated with the key. Should be cast before dereferencing.
void *HashMap_Access(const HashMap *hashMap, const char *key);
