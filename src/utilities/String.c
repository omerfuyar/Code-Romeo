#include "utilities/String.h"

#define String_Min(a, b) ((a) < (b) ? (a) : (b))
#define String_Max(a, b) ((a) > (b) ? (a) : (b))

String String_CreateCopySafe(const char *string, RJGlobal_Size length)
{
    RJGlobal_DebugAssertNullPointerCheck(string);

    String createdString = {0};

    createdString.length = length;
    RJGlobal_DebugAssertAllocationCheck(char, createdString.characters, createdString.length + 1);

    RJGlobal_MemoryCopy(createdString.characters, createdString.length, string);
    createdString.characters[createdString.length] = '\0';

    return createdString;
}

void String_Destroy(String *string)
{
    RJGlobal_DebugAssertNullPointerCheck(string);

    string->length = 0;

    free(string->characters);
    string->characters = NULL;
}

void String_Change(String *string, StringView newString)
{
    RJGlobal_DebugAssertNullPointerCheck(string);

    string->length = newString.length;
    string->characters = (char *)realloc(string->characters, (string->length + 1));
    RJGlobal_DebugAssertNullPointerCheck(string->characters);

    RJGlobal_MemoryCopy(string->characters, string->length, newString.characters);
    string->characters[string->length] = '\0';
}

void String_ConcatEnd(String *string, StringView other)
{
    RJGlobal_DebugAssertNullPointerCheck(string);

    string->characters = (char *)realloc(string->characters, (string->length + other.length + 1));
    RJGlobal_DebugAssertNullPointerCheck(string->characters);

    RJGlobal_MemoryCopy(string->characters + string->length, other.length, other.characters);
    string->length += other.length;
    string->characters[string->length] = '\0';
}

void String_ConcatBegin(String *string, StringView other)
{
    RJGlobal_DebugAssertNullPointerCheck(string);

    String buffer = String_CreateCopySafe(string->characters, string->length);

    string->characters = (char *)realloc(string->characters, (string->length + other.length + 1));
    RJGlobal_DebugAssertNullPointerCheck(string->characters);

    RJGlobal_MemoryCopy(string->characters + other.length, (buffer.length + 1), buffer.characters);
    RJGlobal_MemoryCopy(string->characters, other.length, other.characters);

    string->length += other.length;
    string->characters[string->length] = '\0';

    String_Destroy(&buffer);
}

int String_Compare(StringView string, StringView other)
{
    RJGlobal_DebugAssertNullPointerCheck(string.characters);
    RJGlobal_DebugAssertNullPointerCheck(other.characters);

    int result = strncmp(string.characters, other.characters, String_Min(string.length, other.length));

    if (string.length != other.length && result == 0)
    {
        result = (int)string.length - (int)other.length;
    }

    return result;
}

void String_Tokenize(StringView string, StringView delimeter, RJGlobal_Size *tokenCountRet, StringView *tokenBufferRet, RJGlobal_Size String_MaxTokenCount)
{
    RJGlobal_DebugAssertNullPointerCheck(tokenBufferRet);

    RJGlobal_Size tokenCount = 0;
    RJGlobal_Size lastTokenIndex = 0;

    RJGlobal_Size index = 0;
    while (index < string.length && tokenCount < String_MaxTokenCount)
    {
        if (String_Compare(delimeter, scs(string.characters + index, delimeter.length)) == 0)
        {
            if (index > lastTokenIndex)
            {
                tokenBufferRet[tokenCount] = scs(string.characters + lastTokenIndex, index - lastTokenIndex);
                tokenCount++;
            }

            index += delimeter.length;

            while (index < string.length &&
                   index + delimeter.length <= string.length &&
                   String_Compare(delimeter, scs(string.characters + index, delimeter.length)) == 0)
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

char String_GetChar(StringView string, RJGlobal_Size index)
{
    RJGlobal_DebugAssertNullPointerCheck(string.characters);

    RJGlobal_DebugAssert(index <= string.length, "Char index to get can not be less than the string length");

    return string.characters[index];
}

float String_ToFloat(StringView string)
{
    RJGlobal_DebugAssertNullPointerCheck(string.characters);

    char buffer[STRING_NUMERIC_CHAR_BUFFER];
    RJGlobal_Size copyLength = String_Min(sizeof(buffer) - 1, string.length);
    RJGlobal_MemoryCopy(buffer, copyLength, string.characters);
    buffer[copyLength] = '\0';
    float result = (float)atof(buffer);

    return result;
}

int String_ToInt(StringView string)
{
    RJGlobal_DebugAssertNullPointerCheck(string.characters);

    char buffer[STRING_NUMERIC_CHAR_BUFFER];
    RJGlobal_Size copyLength = String_Min(sizeof(buffer) - 1, string.length);
    RJGlobal_MemoryCopy(buffer, copyLength, string.characters);
    buffer[copyLength] = '\0';
    int result = atoi(buffer);

    return result;
}
