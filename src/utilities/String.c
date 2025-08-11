#include "utilities/String.h"

String String_CreateCopy(char *string, size_t length)
{
    DebugAssertNullPointerCheck(string);

    String createdString = {0};

    createdString.length = length;
    createdString.characters = (char *)malloc((createdString.length + 1) * sizeof(char));
    DebugAssertNullPointerCheck(createdString.characters);

    MemoryCopy(createdString.characters, (createdString.length + 1) * sizeof(char), string);
    createdString.characters[createdString.length] = '\0';
    createdString.isOwner = true;

    return createdString;
}

String String_CreateReference(char *string, size_t length)
{
    DebugAssertNullPointerCheck(string);

    String createdString;

    createdString.length = length;
    createdString.characters = string;

    createdString.isOwner = false;

    return createdString;
}

String String_CreateLiteral(char *string)
{
    DebugAssertNullPointerCheck(string);

    String createdString;

    createdString.length = strlen(string);
    createdString.characters = string;

    createdString.isOwner = false;

    return createdString;
}

void String_Destroy(String *string)
{
    DebugAssertNullPointerCheck(string);

    string->length = 0;

    if (string->isOwner)
    {
        free(string->characters);
        string->characters = NULL;
    }
}

void String_Change(String *string, char *newString, size_t newLength)
{
    DebugAssertNullPointerCheck(string);
    DebugAssert(string->isOwner, "Cannot modify a referenced string");

    string->length = newLength;
    string->characters = (char *)realloc(string->characters, (string->length + 1) * sizeof(char));
    DebugAssertNullPointerCheck(string->characters);

    MemoryCopy(string->characters, (string->length + 1) * sizeof(char), newString);
    string->characters[string->length] = '\0';
}

void String_ConcatEnd(String *string, String other)
{
    DebugAssertNullPointerCheck(string);
    DebugAssert(string->isOwner, "Cannot modify a referenced string");

    size_t oldLength = string->length;
    string->length += other.length;
    string->characters = (char *)realloc(string->characters, (string->length + 1) * sizeof(char));
    DebugAssertNullPointerCheck(string->characters);

    MemoryCopy(string->characters + oldLength, (other.length + 1) * sizeof(char), other.characters);
    string->characters[string->length] = '\0';
}

void String_ConcatBegin(String *string, String other)
{
    DebugAssertNullPointerCheck(string);
    DebugAssert(string->isOwner, "Cannot modify a referenced string");

    char *newLocation = (char *)realloc(string->characters, (string->length + 1) * sizeof(char));
    DebugAssertNullPointerCheck(newLocation);

    MemoryCopy(newLocation + other.length, (string->length + 1) * sizeof(char), string->characters);

    string->characters = newLocation;
    string->length += other.length;
    string->characters[string->length] = '\0';

    MemoryCopy(string->characters, other.length * sizeof(char), other.characters);
}

int String_Compare(String string, String other)
{
    int result = strncmp(string.characters, other.characters, min(string.length, other.length));

    return result;
}

void String_Tokenize(String string, String delimeter, size_t *tokenCountRet, String *tokenBufferRet, size_t tokenBufferSize)
{
    size_t tokenCount = 0;
    size_t lastTokenIndex = 0;
    String *token = NULL;

    for (size_t i = 0; i < string.length && tokenCount < tokenBufferSize; i++)
    {
        if (strncmp(string.characters + i, delimeter.characters, delimeter.length) == 0)
        {
            token = tokenBufferRet + tokenCount;

            token->characters = string.characters + lastTokenIndex;
            token->length = i - lastTokenIndex;
            token->isOwner = false;

            lastTokenIndex = i + delimeter.length;
            i = lastTokenIndex - 1;

            // DebugInfo("Token %zu: '%.*s'", tokenCount, (int)token->length, token->characters);

            tokenCount++;
        }
    }

    if (lastTokenIndex < string.length && tokenCount < tokenBufferSize)
    {
        token = tokenBufferRet + tokenCount;
        token->characters = string.characters + lastTokenIndex;
        token->length = string.length - lastTokenIndex;
        token->isOwner = false;

        // DebugInfo("Token %zu: '%.*s'", tokenCount, (int)token->length, token->characters);

        tokenCount++;
    }

    if (tokenCountRet != NULL)
    {
        *tokenCountRet = tokenCount;
    }
}

char String_GetChar(String string, size_t index)
{
    DebugAssert(index <= string.length, "Char index to get can not be less than the string length");

    return string.characters[index];
}

float String_ToFloat(String string)
{
    char buffer[STRING_TO_NUMERIC_CHAR_BUFFER];
    MemoryCopy(buffer, string.length, string.characters);
    float result = (float)atof(buffer);
    return result;
}

int String_ToInt(String string)
{
    char buffer[STRING_TO_NUMERIC_CHAR_BUFFER];
    MemoryCopy(buffer, string.length, string.characters);
    int result = atoi(buffer);
    return result;
}
