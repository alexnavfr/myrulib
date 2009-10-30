#ifndef __FBPARAMS_H__
#define __FBPARAMS_H__

#include <wx/wx.h>
#include <wx/arrimpl.cpp>
#include <wx/wxsqlite3.h>
#include "FbDatabase.h"

enum FbParamKey {
	FB_CONFIG_TITLE   = 1,
	FB_CONFIG_VERSION = 2,

	FB_NEW_FOLDER	  = 101,
	FB_NEW_COMMENT	 = 102,
	FB_NEW_DOWNLOAD	= 103,

	FB_RECENT_0 = 110,
	FB_RECENT_1 = 111,
	FB_RECENT_2 = 112,
	FB_RECENT_3 = 113,
	FB_RECENT_4 = 114,
	FB_RECENT_5 = 115,

	FB_EXTERNAL_DIR	= 121,
	FB_TRANSLIT_FOLDER = 122,
	FB_TRANSLIT_FILE   = 123,
	FB_FOLDER_FORMAT   = 124,
	FB_FILE_FORMAT	 = 125,

	FB_VIEW_AUTHOR	 = 130,
	FB_VIEW_GENRES	 = 131,
	FB_VIEW_SEARCH	 = 132,
	FB_VIEW_FOLDER	 = 133,
	FB_VIEW_DOWNLD	 = 134,
	FB_MODE_AUTHOR	 = 135,
	FB_MODE_GENRES	 = 136,
	FB_MODE_SEARCH	 = 137,
	FB_MODE_FOLDER	 = 138,

	FB_FILTER_FB2	  = 140,
	FB_FILTER_LIB	  = 141,
	FB_FILTER_USR	  = 142,

	FB_LIBRUSEC_URL   = 150,
	FB_USE_PROXY	  = 151,
	FB_PROXY_ADDR	  = 152,
	FB_AUTO_DOWNLD    = 153,
};

class ParamItem
{
	public:
		ParamItem(const int param): id(param), value(0) {};
		ParamItem(wxSQLite3ResultSet & result);
	public:
		int id;
		int value;
		wxString text;
};

WX_DECLARE_OBJARRAY(ParamItem, ParamArray);

class FbParams {
	public:
		FbParams();
		static int GetValue(const int param);
		static wxString GetText(const int param);
		void LoadParams();
		void SetValue(const int param, int value);
		void SetText(const int param, wxString text);
	private:
		static int DefaultValue(int param);
		static wxString DefaultText(int param);
		static ParamArray sm_params;
	private:
		FbCommonDatabase m_database;
		static wxCriticalSection sm_queue;
};

#endif // __FBPARAMS_H__
