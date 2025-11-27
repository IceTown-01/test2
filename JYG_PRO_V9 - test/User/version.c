#include "version.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const char* version_get_string(void)
{
    return VERSION_STRING;
}

const char* version_get_full_string(void)
{
    return VERSION_STRING_FULL;
}

uint32_t version_get_number(void)
{
    return VERSION_NUMBER;
}
