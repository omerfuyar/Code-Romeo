#pragma once

#include "Global.h"

#define STRING_TO_NUMERIC_CHAR_BUFFER 32

/// @brief Standard string type for the entire project. Meant to be safe and easy to use. Can be used with helper functions for heap or by itself for stack.
typedef struct String
{
    char *characters;
    size_t length;
    bool isOwner;
} String;

/// @brief Creates a new String object from a char array. Allocates its own memory and copies the string. Can be used with dynamic strings.
/// @param string Null terminated C-style string.
/// @param length Length of the original string.
/// @return Newly created String object holding a pointer to copy of the original string.
String String_CreateCopy(char *string, size_t length);

/// @brief Creates a new String object from a char array. The created String object will use the original string's memory. Can be used with string literals.
/// @param string Null terminated C-style string.
/// @param length Length of the original string.
/// @return Newly created String object holding a pointer to reference of the original string.
String String_CreateReference(char *string, size_t length);

/// @brief Creates a new String object from a string literal. The created String object will use the original string's memory.
/// @param string Null terminated C-style string literal.
/// @return Newly created String object holding a pointer to reference of the original string.
String String_CreateLiteral(char *string);

#define scl(string) String_CreateLiteral(string)

/// @brief Destroys a String object and frees its memory if it is a copy.
/// @param string Pointer to the String object to destroy.
void String_Destroy(String *string);

/// @brief Changes the contents of a String object.
/// @param string Pointer to the String object to change.
/// @param newString Null terminated C-style string to copy.
/// @param newLength Length of the new string.
void String_Change(String *string, char *newString, size_t newLength);

/// @brief Concatenates second String object to the end of the first String object. Changes the first String object to hold the concatenated result.
/// @param string Pointer to the String object to concatenate to.
/// @param other Pointer to the String object to concatenate from.
void String_ConcatEnd(String *string, String other);

/// @brief Concatenates second String object to the beginning of the first String object. Changes the first String object to hold the concatenated result.
/// @param string Pointer to the String object to concatenate to.
/// @param other Pointer to the String object to concatenate from.
void String_ConcatBegin(String *string, String other);

/// @brief Compares two String objects by subtracting their character arrays.
/// @param string Pointer to the first String object.
/// @param other Pointer to the second String object.
/// @return Negative if string < other, positive if string > other, zero if they are equal.
int String_Compare(String string, String other);

/// @brief Tokenizes a string into an array of strings.
/// @param string The string to tokenize.
/// @param delimeter The delimiter to use for tokenization.
/// @param tokenCountRet A pointer to a size_t variable to store the number of tokens. Leave NULL if not needed.
/// @param tokenBufferRet A pointer to a String buffer to store the tokenized strings.
/// @param maxTokenCount Max number of tokens to return. The size of the tokenCounts buffer.
void String_Tokenize(String string, String delimeter, size_t *tokenCountRet, String *tokenBufferRet, size_t maxTokenCount);

/// @brief Gets a character from a String object.
/// @param string Pointer to the String object to get the character from.
/// @param index Index of the character to get.
/// @return Character at the specified index.
char String_GetChar(String string, size_t index);

/// @brief Converts a String object to a float.
/// @param string Pointer to the String object to convert.
/// @return Converted float value.
float String_ToFloat(String string);

/// @brief Converts a String object to an int.
/// @param string Pointer to the String object to convert.
/// @return Converted int value.
int String_ToInt(String string);
