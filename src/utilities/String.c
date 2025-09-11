#include "utilities/String.h"
#include "utilities/Maths.h"

String String_CreateCopy(char *string)
{
    DebugAssertNullPointerCheck(string);

    String createdString = {0};

    createdString.length = strlen(string);
    createdString.characters = (char *)malloc((createdString.length + 1) * sizeof(char));
    DebugAssertNullPointerCheck(createdString.characters);

    MemoryCopy(createdString.characters, createdString.length * sizeof(char), string);
    createdString.characters[createdString.length] = '\0';

    createdString.isOwner = true;

    return createdString;
}

String String_CreateCopyS(char *string, size_t length)
{
    DebugAssertNullPointerCheck(string);

    String createdString = {0};

    createdString.length = length;
    createdString.characters = (char *)malloc((createdString.length + 1) * sizeof(char));
    DebugAssertNullPointerCheck(createdString.characters);

    MemoryCopy(createdString.characters, createdString.length * sizeof(char), string);
    createdString.characters[createdString.length] = '\0';

    createdString.isOwner = true;

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

    MemoryCopy(string->characters, string->length * sizeof(char), newString);
    string->characters[string->length] = '\0';
}

void String_ConcatEnd(String *string, String other)
{
    DebugAssertNullPointerCheck(string);
    DebugAssert(string->isOwner, "Cannot modify a referenced string");

    string->characters = (char *)realloc(string->characters, (string->length + other.length + 1) * sizeof(char));
    DebugAssertNullPointerCheck(string->characters);

    MemoryCopy(string->characters + string->length, other.length * sizeof(char), other.characters);
    string->length += other.length;
    string->characters[string->length] = '\0';
}

void String_ConcatBegin(String *string, String other)
{
    DebugAssertNullPointerCheck(string);
    DebugAssert(string->isOwner, "Cannot modify a referenced string");

    String buffer = String_CreateCopy(string->characters);

    string->characters = (char *)realloc(string->characters, (string->length + other.length + 1) * sizeof(char));
    DebugAssertNullPointerCheck(string->characters);

    MemoryCopy(string->characters + other.length, (string->length + 1) * sizeof(char), buffer.characters);
    MemoryCopy(string->characters, other.length * sizeof(char), other.characters);

    string->length += other.length;
    string->characters[string->length] = '\0';

    String_Destroy(&buffer);
}

int String_Compare(String string, String other)
{
    DebugAssertNullPointerCheck(string.characters);
    DebugAssertNullPointerCheck(other.characters);

    int result = strncmp(string.characters, other.characters, Min(string.length, other.length));
    if (result == 0)
    {
        if (string.length < other.length)
        {
            return -1;
        }
        else if (string.length > other.length)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

    return result;
}

void String_Tokenize(String string, String delimeter, size_t *tokenCountRet, String *tokenBufferRet, size_t maxTokenCount)
{
    size_t tokenCount = 0;
    size_t lastTokenIndex = 0;
    String *token = NULL;

    for (size_t i = 0; i < string.length && tokenCount < maxTokenCount; i++)
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

    if (lastTokenIndex < string.length && tokenCount < maxTokenCount)
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
    DebugAssertNullPointerCheck(string.characters);

    DebugAssert(index <= string.length, "Char index to get can not be less than the string length");

    return string.characters[index];
}

float String_ToFloat(String string)
{
    DebugAssertNullPointerCheck(string.characters);

    char buffer[STRING_TO_NUMERIC_CHAR_BUFFER];
    MemoryCopy(buffer, Min(sizeof(buffer), string.length), string.characters);
    buffer[string.length] = '\0';
    float result = (float)atof(buffer);

    return result;
}

int String_ToInt(String string)
{
    DebugAssertNullPointerCheck(string.characters);

    char buffer[STRING_TO_NUMERIC_CHAR_BUFFER];
    MemoryCopy(buffer, Min(sizeof(buffer), string.length), string.characters);
    buffer[string.length] = '\0';
    int result = atoi(buffer);

    return result;
}
