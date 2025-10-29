#pragma once

#include "Global.h"

/// @brief A dynamic hash map implementation. Can be used with any type. Copies passed items to its own property. Shouldn't be used without helper functions.
typedef struct HashMap
{
    void *data;
    size_t capacity;
    size_t count;
    size_t sizeOfItem;
    char *nameOfType;
} HashMap;

/// @brief Creator function for HashMap.
/// @param nameOfType Name of the type stored in the hash map.
/// @param sizeOfItem Size of the item type to store.
/// @param capacity Initial capacity of the hash map.
/// @return The created HashMap struct.
HashMap HashMap_Create(const char *nameOfType, size_t sizeOfItem, size_t capacity);

/// @brief Destroyer function for HashMap.
/// @param hashMap Pointer to the HashMap to destroy.
void HashMap_Destroy(HashMap *hashMap);

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
