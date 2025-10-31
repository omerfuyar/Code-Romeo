#include "utilities/String.h"

#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Max(a, b) ((a) > (b) ? (a) : (b))

String String_CreateCopyS(const char *string, size_t length)
{
    DebugAssertNullPointerCheck(string);

    String createdString = {0};

    createdString.length = length;
    createdString.characters = (char *)malloc((createdString.length + 1));
    DebugAssertNullPointerCheck(createdString.characters);

    MemoryCopy(createdString.characters, createdString.length, string);
    createdString.characters[createdString.length] = '\0';

    return createdString;
}

void String_Destroy(String *string)
{
    DebugAssertNullPointerCheck(string);

    string->length = 0;

    free(string->characters);
    string->characters = NULL;
}

void String_Change(String *string, StringView newString)
{
    DebugAssertNullPointerCheck(string);

    string->length = newString.length;
    string->characters = (char *)realloc(string->characters, (string->length + 1));
    DebugAssertNullPointerCheck(string->characters);

    MemoryCopy(string->characters, string->length, newString.characters);
    string->characters[string->length] = '\0';
}

void String_ConcatEnd(String *string, StringView other)
{
    DebugAssertNullPointerCheck(string);

    string->characters = (char *)realloc(string->characters, (string->length + other.length + 1));
    DebugAssertNullPointerCheck(string->characters);

    MemoryCopy(string->characters + string->length, other.length, other.characters);
    string->length += other.length;
    string->characters[string->length] = '\0';
}

void String_ConcatBegin(String *string, StringView other)
{
    DebugAssertNullPointerCheck(string);

    String buffer = String_CreateCopyS(string->characters, string->length);

    string->characters = (char *)realloc(string->characters, (string->length + other.length + 1));
    DebugAssertNullPointerCheck(string->characters);

    MemoryCopy(string->characters + other.length, (buffer.length + 1), buffer.characters);
    MemoryCopy(string->characters, other.length, other.characters);

    string->length += other.length;
    string->characters[string->length] = '\0';

    String_Destroy(&buffer);
}

int String_Compare(StringView string, StringView other)
{
    DebugAssertNullPointerCheck(string.characters);
    DebugAssertNullPointerCheck(other.characters);

    int result = strncmp(string.characters, other.characters, Min(string.length, other.length));

    if (string.length != other.length && result == 0)
    {
        result = (int)string.length - (int)other.length;
    }

    return result;
}

void String_Tokenize(StringView string, StringView delimeter, size_t *tokenCountRet, StringView *tokenBufferRet, size_t maxTokenCount)
{
    DebugAssertNullPointerCheck(tokenBufferRet);

    size_t tokenCount = 0;
    size_t lastTokenIndex = 0;

    size_t index = 0;
    while (index < string.length && tokenCount < maxTokenCount)
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

    if (lastTokenIndex < string.length && tokenCount < maxTokenCount)
    {
        tokenBufferRet[tokenCount] = scs(string.characters + lastTokenIndex, string.length - lastTokenIndex);
        tokenCount++;
    }

    if (tokenCountRet != NULL)
    {
        *tokenCountRet = tokenCount;
    }
}

char String_GetChar(StringView string, size_t index)
{
    DebugAssertNullPointerCheck(string.characters);

    DebugAssert(index <= string.length, "Char index to get can not be less than the string length");

    return string.characters[index];
}

float String_ToFloat(StringView string)
{
    DebugAssertNullPointerCheck(string.characters);

    char buffer[STRING_TO_NUMERIC_CHAR_BUFFER];
    size_t copyLength = Min(sizeof(buffer) - 1, string.length);
    MemoryCopy(buffer, copyLength, string.characters);
    buffer[copyLength] = '\0';
    float result = (float)atof(buffer);

    return result;
}

int String_ToInt(StringView string)
{
    DebugAssertNullPointerCheck(string.characters);

    char buffer[STRING_TO_NUMERIC_CHAR_BUFFER];
    size_t copyLength = Min(sizeof(buffer) - 1, string.length);
    MemoryCopy(buffer, copyLength, string.characters);
    buffer[copyLength] = '\0';
    int result = atoi(buffer);

    return result;
}
