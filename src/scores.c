#include "main.h"
#include "cJSON.h"

int jsonGetInt(cJSON* d, const char* key)
{
	cJSON* val = cJSON_GetObjectItem(d, key);
	return val ? (int)val->valuedouble : 0;
}

bool jsonGetBool(cJSON* d, const char* key)
{
	cJSON* val = cJSON_GetObjectItem(d, key);
	return val ? cJSON_IsTrue(val) : false;
}

const char* jsonGetStr(cJSON* d, const char* key)
{
	cJSON* val = cJSON_GetObjectItem(d, key);
	return val ? val->valuestring : "";
}

int getSavegameVersion(const char* checkstr)
{
	const int maxlen = (int)strlen(checkstr);
	int versionNumber = 300;
	char versionStr[4] = "000";
	int i = 0;
	for ( int j = 0; checkstr[j] != '\0' && j < maxlen; ++j )
	{
		if ( checkstr[j] >= '0' && checkstr[j] <= '9' )
		{
			versionStr[i] = checkstr[j];
			++i;
			if ( i == 3 )
			{
				versionStr[i] = '\0';
				break;
			}
		}
	}
	versionNumber = atoi(versionStr);
	if ( versionNumber < 200 || versionNumber > 999 )
	{
		return -1;
	}
	return versionNumber;
}
