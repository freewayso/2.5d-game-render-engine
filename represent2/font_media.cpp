/* -------------------------------------------------------------------------
//	文件名		：	font_media_t.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	font_media_t的定义
//
// -----------------------------------------------------------------------*/
#include "font_media.h"

//media - font
KFontConfig g_sFontConfig;

BOOL InitFontCreator(void* pArg)
{
	FontInitParam* param = (FontInitParam*)pArg;
	g_sFontConfig.hWnd = param->hWnd;
	g_sFontConfig.bSingleByteCharSet = param->bSingleByteCharSet;
	return true;
}

void ShutdownFontCreator() {}

KMedia* CreateFont( LPCSTR pszIdentifier, void* pArg )
{
	DWORD dwType = *(DWORD*)pArg;
	if( dwType == 0 )	//sys font
	{
		//identifier format: "charset,typeface,height,weight,italic,antialiased,border_width"
		char pszIdentifierTmp[256];
		strcpy( pszIdentifierTmp, pszIdentifier );

		char* pszComma1 = strchr( pszIdentifierTmp, ',' );
		char* pszComma2 = strchr( &pszComma1[1], ',' );
		if( !pszComma1 || !pszComma2 )
			return new FontMedia( "", 0, 0, 0, 0, 0, 0 );
		char* pszComma3 = (pszComma2 && pszComma2[1]) ? strchr(&pszComma2[1], ',') : 0;
		char* pszComma4 = (pszComma3 && pszComma3[1]) ? strchr(&pszComma3[1], ',') : 0;
		char* pszComma5 = (pszComma4 && pszComma4[1]) ? strchr(&pszComma4[1], ',') : 0;
		char* pszComma6 = (pszComma5 && pszComma5[1]) ? strchr(&pszComma5[1], ',') : 0;
		*pszComma1 = 0;
		*pszComma2 = 0;
		if( pszComma3 ) 
			*pszComma3 = 0;
		if( pszComma4 )
			*pszComma4 = 0;
		if( pszComma5 )
			*pszComma5 = 0;
		if( pszComma6 )
			*pszComma6 = 0;
		INT nHeight = atoi( &pszComma2[1] );
		DWORD dwCharset = (DWORD)atoi( pszIdentifierTmp );
		DWORD dwWeight = pszComma3 ? (DWORD)atoi(&pszComma3[1]) : FW_NORMAL;
		BOOL bItalic = pszComma4 ? (atoi(&pszComma4[1])!=0) : false;
		BOOL bAntialiased = pszComma5 ? (atoi(&pszComma5[1])!=0) : false;
		INT nBorderWidth = pszComma6 ? atoi(&pszComma6[1]) : 0;
		return new FontMedia( &pszComma1[1], nHeight, dwCharset, dwWeight, bItalic, bAntialiased, nBorderWidth );
	}
	else
	{
		return new KUserFontMedia( pszIdentifier, dwType != 1 );	//1 for gb2312, else for big5
	}
}
