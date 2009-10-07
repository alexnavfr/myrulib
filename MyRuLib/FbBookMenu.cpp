#include "FbBookMenu.h"
#include "FbConst.h"
#include "FbDatabase.h"
#include "MyRuLibApp.h"

WX_DEFINE_OBJARRAY(FbFolderArray);

FbFolderArray FbBookMenu::sm_folders;

FbBookMenu::FbBookMenu(int id, int iFolder)
	: m_id(id)
{
    if (sm_folders.Count() == 0) LoadFolders();

	wxMenu * submenu = new wxMenu;
	for (size_t i=0; i<sm_folders.Count(); i++) {
		int id = sm_folders[i].id;
		if (sm_folders[i].folder == iFolder) continue;
		submenu->Append(id, sm_folders[i].name);
	}

	Append(ID_OPEN_BOOK, _("Открыть книгу\tEnter"));
    AppendSeparator();
	Append(wxID_SELECTALL, _("Выделить все\tCtrl+A"));
	Append(ID_UNSELECTALL, _("Отменить выделение"));
    AppendSeparator();
    if (iFolder == fbNO_FOLDER) {
        Append(ID_FAVORITES_ADD, _T("Добавить в избранное"));
    } else {
        Append(ID_FAVORITES_DEL, _T("Удалить закладку"));
		if (iFolder) Append(ID_FAVORITES_ADD, _T("Добавить в избранное"));
    }
	Append(wxID_ANY, _("Добавить в папку"), submenu);
}

void FbBookMenu::LoadFolders()
{
    wxString sql = wxT("SELECT id, value FROM folders ORDER BY value");
    wxSQLite3ResultSet result = wxGetApp().GetConfigDatabase().ExecuteQuery(sql);
    int id = ID_FAVORITES_ADD;
    while (result.NextRow()) {
        FbFolderItem * item = new FbFolderItem;
        item->id = ++id;
        item->folder = result.GetInt(0);
        item->name = result.GetString(1);
        sm_folders.Add(item);
    }
}

int FbBookMenu::GetFolder(const int id)
{
	for (size_t i=0; i<sm_folders.Count(); i++)
		if ( sm_folders[i].id == id ) return sm_folders[i].folder;
	return 0;
}

void FbBookMenu::Connect(wxWindow * frame, wxObjectEventFunction func)
{
    if (sm_folders.Count() == 0) LoadFolders();
	for (size_t i=0; i<sm_folders.Count(); i++)
		frame->Connect(sm_folders[i].id, wxEVT_COMMAND_MENU_SELECTED, func);
}