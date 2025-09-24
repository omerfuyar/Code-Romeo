#pragma once

#include "Global.h"

#define STRING_TO_NUMERIC_CHAR_BUFFER 32

/// @brief Standard string type for the entire project. Can be used with helper functions for heap or by itself for stack. Owner of its memory.
typedef struct String
{
    char *characters;
    size_t length;
} String;

/// @brief Standard view string for entire project. Used in parameters to indicate the function is not changing the string data and for other reasons. Does not owns the memory, just points it.
typedef struct StringView
{
    const char *characters;
    size_t length;
} StringView;

/// @brief Creates a new String object from a char array safely. Allocates its own memory and copies the string. Can be used with dynamic strings.
/// @param string Any char pointer.
/// @param length Length of the given string.
/// @return Newly created String object holding a pointer to copy of the original string.
String String_CreateCopyS(const char *string, size_t length);

/// @brief Create a owner copy of the given string it can be a view or owner.
/// @param stringToCopy String to copy. Not a pointer.
#define scc(stringToCopy) \
    String_CreateCopyS(stringToCopy.characters, stringToCopy.length)

/// @brief Create view from string literal.
/// @param stringLiteral The literal string to create a view of.
#define scl(stringLiteral) \
    (StringView) { .characters = stringLiteral, .length = strlen(stringLiteral) }

/// @brief Create a view of given string object.
/// @param stringToCreateView String object to create a view of. Not a pointer.
#define scv(stringToCreateView) \
    (StringView) { .characters = stringToCreateView.characters, .length = stringToCreateView.length }

/// @brief Destroys a String object and frees its memory if it is a copy.
/// @param string Pointer to the String object to destroy.
void String_Destroy(String *string);

/// @brief Changes the contents of a String object.
/// @param string Pointer to the String object to change.
/// @param newString Null terminated C-style string to copy.
/// @param newLength Length of the new string.
void String_Change(String *string, StringView newString);

/// @brief Concatenates second String object to the end of the first String object. Changes the first String object to hold the concatenated result.
/// @param string Pointer to the String object to concatenate to.
/// @param other Pointer to the String object to concatenate from.
void String_ConcatEnd(String *string, StringView other);

/// @brief Concatenates second String object to the beginning of the first String object. Changes the first String object to hold the concatenated result.
/// @param string Pointer to the String object to concatenate to.
/// @param other Pointer to the String object to concatenate from.
void String_ConcatBegin(String *string, StringView other);

/// @brief Compares two String objects by subtracting their character arrays.
/// @param string Pointer to the first String object.
/// @param other Pointer to the second String object.
/// @return Negative if string < other, positive if string > other, zero if they are equal.
int String_Compare(StringView string, StringView other);

/// @brief Tokenizes a string into an array of string views. Returned buffer uses the memory of the first string parameter.
/// @param string The string to tokenize.
/// @param delimeter The delimiter to use for tokenization.
/// @param tokenCountRet A pointer to a size_t variable to store the number of tokens. Leave NULL if not needed.
/// @param tokenBufferRet A pointer to a StringView buffer to store the tokenized string views.
/// @param maxTokenCount Max number of tokens to return. The size of the tokenCounts buffer.
void String_Tokenize(StringView string, StringView delimeter, size_t *tokenCountRet, StringView *tokenBufferRet, size_t maxTokenCount);

/// @brief Gets a character from a String object.
/// @param string Pointer to the String object to get the character from.
/// @param index Index of the character to get.
/// @return Character at the specified index.
char String_GetChar(StringView string, size_t index);

/// @brief Converts a String object to a float.
/// @param string Pointer to the String object to convert.
/// @return Converted float value. 0.0f if string doesn't contain any numbers.
float String_ToFloat(StringView string);

/// @brief Converts a String object to an int.
/// @param string Pointer to the String object to convert.
/// @return Converted int value. 0 if string doesn't contain any numbers.
int String_ToInt(StringView string);
