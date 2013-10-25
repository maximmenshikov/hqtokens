// HqTokens.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "regext.h"
#include <vector>
#include "..\..\common\PacmanClient.h"
#include "..\..\common\APIReady.h"

/*  FUNCTIONALITY:
	1) Crawls over all the token list and check if certain one exists in registry-based database.
	2) If it exists, then HqTokens replaces its token and/or application icon with a crisp one.
*/

#define MAXTIMEOUT 120000
typedef std::vector<wchar_t *> wvector;

bool WaitForApis()
{
	if (WaitForAPIReady(SH_FILESYS_APIS, MAXTIMEOUT) != WAIT_OBJECT_0 ||
        WaitForAPIReady(SH_SHELL, MAXTIMEOUT) != WAIT_OBJECT_0 ||
		WaitForAPIReady(SH_DEVMGR_APIS, MAXTIMEOUT) != WAIT_OBJECT_0 ||
		WaitForAPIReady(SH_SERVICES, MAXTIMEOUT) != WAIT_OBJECT_0 ||
		WaitForAPIReady(SH_COMM, MAXTIMEOUT) != WAIT_OBJECT_0 || 
		WaitForAPIReady(SH_WMGR, MAXTIMEOUT) != WAIT_OBJECT_0)
		return false;
	return true;
}

wvector* GetTokens(wchar_t *str)
{
	wvector *vector = new wvector;
	int len = wcslen(str);
	if (len > 0)
	{
		wchar_t *cur = str;
		vector->push_back(cur);
		for (int i = 0; i < len; ++i)
		{
			if (str[i] == L'|')
			{
				str[i] = L'\0';
				i++;
				vector->push_back(&str[i]);
			}
		}
	}
	return vector;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (WaitForApis() == false)
	{
		return -1;
	}
	IApplicationInfoEnumerator *appInfoEnumerator = NULL;
	GetAllApplications(&appInfoEnumerator);
	if (appInfoEnumerator)
	{
		ITokenManager *tokenManager = NULL;
		GetTokenManager(&tokenManager);
		if (tokenManager)
		{
			IApplicationInfo *cur = NULL;
			while (appInfoEnumerator->get_Next(&cur) == S_OK)
			{
				if (cur == NULL)
					break;
				unsigned long long appId = cur->get_AppID();
				ITokenIDEnumerator *tokenIds = NULL;
				tokenManager->GetTokenIDEnum(appId,  &tokenIds);
				if (tokenIds)
				{
					LPWSTR tokenID = NULL;
					while (tokenIds->get_Next(&tokenID) == S_OK)
					{
						wchar_t tokenString[1000];
						if (RegistryGetString(HKEY_LOCAL_MACHINE, L"Software\\OEM\\Tokens", tokenID, tokenString, 1000) == S_OK)
						{
							wchar_t *lines[3] = {NULL, NULL, NULL};
							wvector *vect = GetTokens(tokenString);
							if (vect)
							{
								int size = vect->size();
								for (int i = 0; i < size; ++i)
								{
									lines[i] = *(vect->begin() + i);
								}
								delete vect;
							}
							wchar_t *tokenXml = lines[0];
							wchar_t *tokenApplicationIcon = lines[1];
							IToken *token = NULL;
							tokenManager->GetToken(cur->get_AppID(), tokenID, &token);
							if (token)
							{
								if (tokenXml && wcslen(tokenXml))
								{
									FILE *f = _wfopen(tokenXml, L"rb");
									if (f)
									{
										fseek(f, -1, SEEK_END);
										int fsize = ftell(f);
										fseek(f, 0, SEEK_SET);
										char *xml = new char[fsize];
										if (xml)
										{
											fread(xml, 1, fsize, f);
											
											HRESULT hr = tokenManager->UpdateTokenFromXML(appId, (PBYTE)xml, fsize);
											delete[] xml;
										}
										fclose(f);
									}
								}
								if (tokenApplicationIcon && wcslen(tokenApplicationIcon))
								{
									PMUpdateAppIconPath(*cur->get_ProductID(), tokenApplicationIcon);
								}
							}
						}
					}
				}
			}
			delete tokenManager;
		}
		delete appInfoEnumerator;
	}
	return 0;
}

