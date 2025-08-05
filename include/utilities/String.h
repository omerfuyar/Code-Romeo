#pragma once

#include "Global.h"

/// @brief Standard string type for the entire project. Meant to be safe and easy to use. Can be used with helper functions for heap or by itself for stack.
typedef struct String
{
    char *characters;
    size_t length;
    bool isOwner;
} String;

/// @brief Creates a new String object from a char array. Allocates its own memory and copies the string. Can be used with dynamic strings.
/// @param string Null terminated C-style string.
/// @return Newly created String object holding a pointer to copy of the original string.
String String_CreateCopy(char *string);

/// @brief Creates a new String object from a char array. The created String object will use the original string's memory. Can be used with string literals.
/// @param string Null terminated C-style string.
/// @return Newly created String object holding a pointer to reference of the original string.
String String_CreateReference(char *string);

/// @brief Destroys a String object and frees its memory if it is a copy.
/// @param string Pointer to the String object to destroy.
void String_Destroy(String string);

/// @brief Changes the contents of a String object.s
/// @param string Pointer to the String object to change.
/// @param newString Null terminated C-style string to copy.
void String_Change(String string, char *newString);

/// @brief Concatenates two String objects. Changes the first String object to hold the concatenated result.
/// @param string Pointer to the String object to concatenate to.
/// @param other Pointer to the String object to concatenate from.
void String_Concat(String string, String other);

/// @brief Compares two String objects by subtracting their character arrays.
/// @param string Pointer to the first String object.
/// @param other Pointer to the second String object.
/// @return Negative if string < other, positive if string > other, zero if they are equal.
int String_Compare(String string, String other);

/// @brief Gets a character from a String object.
/// @param string Pointer to the String object to get the character from.
/// @param index Index of the character to get.
/// @return Character at the specified index.
char String_GetChar(String string, size_t index);
