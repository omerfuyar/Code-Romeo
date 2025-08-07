#include "utilities/String.h"

String String_CreateCopy(char *string)
{
    DebugAssertNullPointerCheck(string);

    String createdString = {NULL, 0, true};

    createdString.length = strlen(string);
    createdString.characters = (char *)malloc((createdString.length + 1) * sizeof(char));
    DebugAssertNullPointerCheck(createdString.characters);

    StringCopy(createdString.characters, (createdString.length + 1) * sizeof(char), string);
    createdString.isOwner = true;

    return createdString;
}

String String_CreateReference(char *string)
{
    DebugAssertNullPointerCheck(string);
    String createdString;

    createdString.length = strlen(string);
    createdString.characters = string;
    return createdString;
}

void String_Destroy(String string)
{
    string.length = 0;

    if (string.isOwner)
    {
        free(string.characters);
        string.characters = NULL;
    }
}

void String_Change(String string, char *newString)
{
    string.length = strlen(newString);
    string.characters = (char *)realloc(string.characters, (string.length + 1) * sizeof(char));
    DebugAssertNullPointerCheck(string.characters);

    StringCopy(string.characters, (string.length + 1) * sizeof(char), newString);
}

void String_Concat(String string, String other)
{
    string.length += other.length;
    string.characters = (char *)realloc(string.characters, (string.length + 1) * sizeof(char));
    DebugAssertNullPointerCheck(string.characters);

    StringCopy(string.characters, (string.length + 1) * sizeof(char), other.characters);
}

int String_Compare(String string, String other)
{
    int result = strcmp(string.characters, other.characters);

    return result;
}

char String_GetChar(String string, size_t index)
{
    DebugAssert(index <= string.length, "Char index to get can not be less than the string length");

    return string.characters[index];
}
