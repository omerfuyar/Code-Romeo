#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
enum
{
    otherFind_SUCCESS, //! must be here
    otherFind_NULL,
    otherFind_NOTFOUND
} otherFind(size_t *ret, char **stringArray, size_t stringArrayLen, char *pattern)
*/

enum
{
    otherFind_NULL = 1, // or define special codes
    otherFind_NOTFOUND = 2
} otherFind(size_t *ret, char **stringArray, size_t stringArrayLen, char *pattern)
{
    if (!stringArray || !pattern || !ret)
    {
        return otherFind_NULL;
    }

    for (size_t i = 0; i < stringArrayLen; i++)
    {
        if (!strcmp(stringArray[i], pattern))
        {
            *ret = i;
            return 0;
        }
    }

    return otherFind_NOTFOUND;
}

void proccessBasicRetrun(unsigned int ret, size_t value)
{
    if (ret)
    {
        printf("error '%d' received from find.\n", ret);
    }
    else
    {
        printf("element found in index '%zu'.\n", value);
    }
}

int main(int argc, char **argv)
{
    size_t findReturn = 0;

    proccessBasicRetrun(otherFind(&findReturn, argv, argc, argv[0]), findReturn); // success
    proccessBasicRetrun(otherFind(&findReturn, NULL, argc, ""), findReturn);      // error NULL
    proccessBasicRetrun(otherFind(&findReturn, argv, argc, "31"), findReturn);    // error NOTFOUND
}
