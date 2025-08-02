#pragma once
#include <Global.h>

/// @brief Standard string type for the entire project. Meant to be safe and easy to use. Can be used with helper functions for heap or by itself for stack.
typedef struct String
{
    char *characters;
    size_t length;
} String;

/// @brief
/// @param string
/// @return
String *String_Create(char *string);

/// @brief
/// @param string
/// @param stringLength
/// @return
String *String_CreateSafe(char *string, size_t stringLength);

/// @brief
/// @param string
void String_Destroy(String *string);

/// @brief
/// @param string
/// @param newString
void String_Change(String *string, char *newString);

/// @brief
/// @param string
/// @param newString
/// @param newStringLength
void String_ChangeSafe(String *string, char *newString, size_t newStringLength);

/// @brief
/// @param string
/// @param index
/// @return
char String_GetChar(String *string, size_t index);
