#include "utilities/String.h"

#define String_Min(a, b) ((a) < (b) ? (a) : (b))
#define String_Max(a, b) ((a) > (b) ? (a) : (b))

String String_CreateCopySafe(const char *string, RJ_Size length)
{
    RJ_DebugAssertNullPointerCheck(string);

    String createdString = {0};

    createdString.length = length;
    RJ_DebugAssertAllocationCheck(char, createdString.characters, createdString.length + 1);

    memcpy(createdString.characters, string, createdString.length);
    createdString.characters[createdString.length] = '\0';

    return createdString;
}

void String_Destroy(String *string)
{
    RJ_DebugAssertNullPointerCheck(string);

    string->length = 0;

    free(string->characters);
    string->characters = NULL;
}

void String_Change(String *string, StringView newString)
{
    RJ_DebugAssertNullPointerCheck(string);

    string->length = newString.length;
    string->characters = (char *)realloc(string->characters, (string->length + 1));
    RJ_DebugAssertNullPointerCheck(string->characters);

    memcpy(string->characters, newString.characters, string->length);
    string->characters[string->length] = '\0';
}

void String_ConcatEnd(String *string, StringView other)
{
    RJ_DebugAssertNullPointerCheck(string);

    string->characters = (char *)realloc(string->characters, (string->length + other.length + 1));
    RJ_DebugAssertNullPointerCheck(string->characters);

    memcpy(string->characters + string->length, other.characters, other.length);
    string->length += other.length;
    string->characters[string->length] = '\0';
}

void String_ConcatBegin(String *string, StringView other)
{
    RJ_DebugAssertNullPointerCheck(string);

    String buffer = String_CreateCopySafe(string->characters, string->length);

    string->characters = (char *)realloc(string->characters, (string->length + other.length + 1));
    RJ_DebugAssertNullPointerCheck(string->characters);

    memcpy(string->characters + other.length, buffer.characters, buffer.length + 1);
    memcpy(string->characters, other.characters, other.length);

    string->length += other.length;
    string->characters[string->length] = '\0';

    String_Destroy(&buffer);
}

int String_Compare(StringView string, StringView other)
{
    RJ_DebugAssertNullPointerCheck(string.characters);
    RJ_DebugAssertNullPointerCheck(other.characters);

    int result = strncmp(string.characters, other.characters, String_Min(string.length, other.length));

    if (string.length != other.length && result == 0)
    {
        result = (int)string.length - (int)other.length;
    }

    return result;
}

bool String_AreSame(StringView string, StringView other)
{
    RJ_DebugAssertNullPointerCheck(string.characters);
    RJ_DebugAssertNullPointerCheck(other.characters);

    if (string.length != other.length)
    {
        return false;
    }

    for (RJ_Size i = 0; i < string.length; i++)
    {
        if (string.characters[i] != other.characters[i])
        {
            return false;
        }
    }

    return true;
}

void String_Tokenize(StringView string, StringView delimeter, RJ_Size *tokenCountRet, StringView *tokenBufferRet, RJ_Size String_MaxTokenCount)
{
    RJ_DebugAssertNullPointerCheck(tokenBufferRet);

    RJ_Size tokenCount = 0;
    RJ_Size lastTokenIndex = 0;

    RJ_Size index = 0;
    while (index < string.length && tokenCount < String_MaxTokenCount)
    {
        if (String_AreSame(delimeter, scs(string.characters + index, delimeter.length)))
        {
            if (index > lastTokenIndex)
            {
                tokenBufferRet[tokenCount] = scs(string.characters + lastTokenIndex, index - lastTokenIndex);
                tokenCount++;
            }

            index += delimeter.length;

            while (index < string.length &&
                   index + delimeter.length <= string.length &&
                   String_AreSame(delimeter, scs(string.characters + index, delimeter.length)))
            {
                index += delimeter.length;
            }

            lastTokenIndex = index;
        }
        else
        {
            index++;
        }
    }

    if (lastTokenIndex < string.length && tokenCount < String_MaxTokenCount)
    {
        tokenBufferRet[tokenCount] = scs(string.characters + lastTokenIndex, string.length - lastTokenIndex);
        tokenCount++;
    }

    if (tokenCountRet != NULL)
    {
        *tokenCountRet = tokenCount;
    }
}

char String_GetChar(StringView string, RJ_Size index)
{
    RJ_DebugAssertNullPointerCheck(string.characters);

    RJ_DebugAssert(index <= string.length, "Char index to get can not be less than the string length");

    return string.characters[index];
}

float String_ToFloat(StringView string)
{
    RJ_DebugAssertNullPointerCheck(string.characters);

    char buffer[STRING_NUMERIC_CHAR_BUFFER] = {0};
    RJ_Size copyLength = String_Min(sizeof(buffer) - 1, string.length);
    memcpy(buffer, string.characters, copyLength);
    buffer[copyLength] = '\0';
    float result = (float)atof(buffer);

    return result;
}

int String_ToInt(StringView string)
{
    RJ_DebugAssertNullPointerCheck(string.characters);

    char buffer[STRING_NUMERIC_CHAR_BUFFER] = {0};
    RJ_Size copyLength = String_Min(sizeof(buffer) - 1, string.length);
    memcpy(buffer, string.characters, copyLength);
    buffer[copyLength] = '\0';
    int result = atoi(buffer);

    return result;
}
