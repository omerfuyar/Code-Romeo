#include <string.h>
#include "utilities/String.h"

String *String_Create(char *string)
{
    DebugAssertPointerNullCheck(string);

    String *createdString = (String *)malloc(sizeof(String));

    createdString->length = strlen(string);
    createdString->characters = (char *)malloc(createdString->length * sizeof(char));

    strcpy_s(createdString->characters, createdString->length * sizeof(char), string);

    return createdString;
}

String *String_CreateSafe(char *string, size_t stringLength)
{
    DebugAssertPointerNullCheck(string);

    String *createdString = (String *)malloc(sizeof(String));

    createdString->length = stringLength;
    createdString->characters = (char *)malloc(createdString->length * sizeof(char));

    strcpy_s(createdString->characters, createdString->length * sizeof(char), string);

    return createdString;
}

void String_Destroy(String *string)
{
    DebugAssertPointerNullCheck(string);

    string->length = 0;

    free(string->characters);
    string->characters = NULL;

    free(string);
    string = NULL;
}

void String_Change(String *string, char *newString)
{
    DebugAssertPointerNullCheck(string);
    DebugAssertPointerNullCheck(newString);

    string->length = strlen(newString);
    string->characters = (char *)realloc(string->characters, string->length);

    strcpy_s(string->characters, string->length * sizeof(char), newString);
}

void String_ChangeSafe(String *string, char *newString, size_t newStringLength)
{
    DebugAssertPointerNullCheck(string);
    DebugAssertPointerNullCheck(newString);

    string->length = newStringLength;
    string->characters = (char *)realloc(string->characters, string->length);

    strcpy_s(string->characters, string->length * sizeof(char), newString);
}

char String_GetChar(String *string, size_t index)
{
    DebugAssertPointerNullCheck(string);
    DebugAssert(index <= string->length, "Char index to get can not be less than the string length");

    return string->characters[index];
}
