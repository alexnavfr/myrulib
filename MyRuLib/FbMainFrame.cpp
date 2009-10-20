/***************************************************************
 * Name:      MyRuLibMain.cpp
 * Purpose:   Code for Application Frame
 * Author:    Kandrashin Denis (mail@kandr.ru)
 * Created:   2009-05-05
 * Copyright: Kandrashin Denis (www.lintest.ru)
 * License:
 **************************************************************/

#include "FbMainFrame.h"
#include <wx/splitter.h>
#include <wx/dirdlg.h>
#include <wx/stattext.h>
#include <wx/dcclient.h>
#include <wx/artprov.h>
#include "FbConst.h"
#include "MyRuLibApp.h"
#include "FbManager.h"
#include "SettingsDlg.h"
#include "ImpThread.h"
#include "FbFrameSearch.h"
#include "FbFrameGenres.h"
#include "FbFrameFavour.h"
#include "FbFrameInfo.h"
#include "FbMainMenu.h"
#include "VacuumThread.h"

BEGIN_EVENT_TABLE(FbMainFrame, wxAuiMDIParentFrame)
    EVT_TOOL(wxID_NEW, FbMainFrame::OnNewZip)
    EVT_MENU(wxID_OPEN, FbMainFrame::OnFolder)
    EVT_MENU(wxID_EXIT, FbMainFrame::OnExit)
    EVT_MENU(ID_MENU_SEARCH, FbMainFrame::OnMenuTitle)
    EVT_MENU(ID_FRAME_AUTHOR, FbMainFrame::OnMenuAuthor)
    EVT_MENU(ID_FRAME_GENRES, FbMainFrame::OnMenuGenres)
    EVT_MENU(ID_FRAME_FAVOUR, FbMainFrame::OnMenuFavour)
    EVT_MENU(ID_FRAME_ARCH, FbMainFrame::OnMenuNothing)
    EVT_MENU(ID_FRAME_SEQ, FbMainFrame::OnMenuNothing)
    EVT_MENU(ID_FRAME_DATE, FbMainFrame::OnMenuNothing)
    EVT_MENU(ID_MENU_DB_INFO, FbMainFrame::OnDatabaseInfo)
    EVT_MENU(ID_MENU_VACUUM, FbMainFrame::OnVacuum)
    EVT_MENU(ID_MENU_CONFIG, FbMainFrame::OnMenuNothing)
	EVT_MENU(wxID_PREFERENCES, FbMainFrame::OnSetup)
	EVT_MENU(ID_OPEN_WEB, FbMainFrame::OnOpenWeb)
	EVT_MENU(wxID_ABOUT, FbMainFrame::OnAbout)

    EVT_MENU(ID_FIND_AUTHOR, FbMainFrame::OnFindAuthor)
	EVT_TEXT_ENTER(ID_FIND_AUTHOR, FbMainFrame::OnFindAuthorEnter)
    EVT_MENU(ID_FIND_TITLE, FbMainFrame::OnFindTitle)
	EVT_TEXT_ENTER(ID_FIND_TITLE, FbMainFrame::OnFindTitleEnter)

    EVT_UPDATE_UI(ID_PROGRESS_START, FbMainFrame::OnProgressStart)
    EVT_UPDATE_UI(ID_PROGRESS_UPDATE, FbMainFrame::OnProgressUpdate)
    EVT_UPDATE_UI(ID_PROGRESS_FINISH, FbMainFrame::OnProgressFinish)

    EVT_MENU(ID_ERROR, FbMainFrame::OnError)
    EVT_MENU(ID_LOG_TEXTCTRL, FbMainFrame::OnHideLog)

    EVT_AUI_PANE_CLOSE(FbMainFrame::OnPanelClosed)
    EVT_AUINOTEBOOK_PAGE_CLOSE(wxID_ANY, FbMainFrame::OnNotebookPageClose)

    EVT_COMMAND(ID_UPDATE_FOLDER, fbEVT_BOOK_ACTION, FbMainFrame::OnUpdateFolder)
    EVT_COMMAND(ID_UPDATE_RATING, fbEVT_BOOK_ACTION, FbMainFrame::OnUpdateRating)
    EVT_COMMAND(ID_DATABASE_INFO, fbEVT_BOOK_ACTION, FbMainFrame::OnInfoCommand)
    EVT_COMMAND(ID_OPEN_AUTHOR, fbEVT_BOOK_ACTION, FbMainFrame::OnOpenAuthor)
END_EVENT_TABLE()

FbMainFrame::FbMainFrame()
{
	Create(NULL, wxID_ANY, wxT("MyRuLib - My Russian Library"));
}

FbMainFrame::~FbMainFrame()
{
	m_FrameManager.UnInit();
}

#ifndef __WXMSW__
#include "res/home.xpm"
#endif

bool FbMainFrame::Create(wxWindow * parent, wxWindowID id, const wxString & title)
{
	bool res = wxAuiMDIParentFrame::Create(parent, id, title, wxDefaultPosition, wxSize(700, 500), wxDEFAULT_FRAME_STYLE|wxFRAME_NO_WINDOW_MENU);
	if(res)	{
		CreateControls();
        #ifdef __WXMSW__
		wxIcon icon(wxT("aaaa"));
		SetIcon(icon);
        #else
        wxBitmap bitmap(home_xpm);
        wxIcon icon;
        icon.CopyFromBitmap(bitmap);
        SetIcon(icon);
        #endif
	}
	return res;
}

void FbMainFrame::CreateControls()
{
	SetMenuBar(new FbMainMenu);

	const int widths[] = {-90, -50, -50, -10};
    m_ProgressBar.Create(this, ID_PROGRESSBAR);
    m_ProgressBar.SetFieldsCount(4);
	m_ProgressBar.SetStatusWidths(4, widths);
	SetStatusBar(&m_ProgressBar);

	m_LOGTextCtrl.Create(this, ID_LOG_TEXTCTRL, wxEmptyString, wxDefaultPosition, wxSize(-1, 100), wxTE_MULTILINE|wxTE_READONLY|wxNO_BORDER|wxTE_DONTWRAP);
	FbFrameAuthor * authors = new FbFrameAuthor(this);
	authors->SelectRandomLetter();

	new FbFrameGenres(this);

	GetNotebook()->SetWindowStyleFlag(
        wxAUI_NB_TOP|
		wxAUI_NB_SCROLL_BUTTONS |
		wxAUI_NB_CLOSE_ON_ACTIVE_TAB |
		wxNO_BORDER);
	GetNotebook()->SetSelection(0);

	m_FrameManager.SetManagedWindow(this);

	m_FrameManager.AddPane(CreateToolBar(), wxAuiPaneInfo().Name(wxT("ToolBar")).Top().Show(true).ToolbarPane().Dockable(false).PaneBorder(false));
	m_FrameManager.AddPane(GetNotebook(), wxAuiPaneInfo().Name(wxT("CenterPane")).CenterPane());
	m_FrameManager.AddPane(&m_LOGTextCtrl, wxAuiPaneInfo().Bottom().Name(wxT("Log")).Caption(_("Информационные сообщения")).Show(false));
	m_FrameManager.Update();

	m_FindAuthor.SetFocus();

	Centre();
}

void FbMainFrame::OnSetup(wxCommandEvent & event)
{
    SettingsDlg::Execute(this);
}

void FbMainFrame::OnOpenWeb(wxCommandEvent & event)
{
    wxLaunchDefaultBrowser(strHomePage);
}

void FbMainFrame::OnAbout(wxCommandEvent & event)
{
    wxMessageBox(strVersionInfo + wxT("\n\nDatabase:\n") + wxGetApp().GetAppData());
}

wxAuiToolBar * FbMainFrame::CreateToolBar()
{
	wxAuiToolBar * toolbar = new wxAuiToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE);
	wxAuiToolBarArt * art = new wxAuiDefaultToolBarArt;
	art->SetElementSize(wxAUI_TBART_GRIPPER_SIZE, 0);
	toolbar->SetArtProvider(art);

    wxWindowDC dc(toolbar);
    int text_width = 0, text_height = 0;
    dc.GetTextExtent(wxT("Автор:"), &text_width, &text_height);

	toolbar->AddTool(wxID_NEW, _("Импорт файла"), wxArtProvider::GetBitmap(wxART_NEW), _("Добавить в библиотеку новые файлы"));
	toolbar->AddTool(wxID_OPEN, _("Импорт папки"), wxArtProvider::GetBitmap(wxART_FILE_OPEN), _("Добавить в библиотеку директорию"));
	toolbar->AddSeparator();
	toolbar->AddLabel(wxID_ANY, _("Автор:"), text_width);
	m_FindAuthor.Create(toolbar, ID_FIND_AUTHOR, wxEmptyString, wxDefaultPosition, wxSize(180, -1), wxTE_PROCESS_ENTER);
	toolbar->AddControl( &m_FindAuthor );
	toolbar->AddTool(ID_FIND_AUTHOR, _("Найти"), wxArtProvider::GetBitmap(wxART_FIND), _("Поиск автора"));
	toolbar->AddSeparator();
	toolbar->AddLabel(wxID_ANY, _("Книга:"), text_width);
	m_FindTitle.Create(toolbar, ID_FIND_TITLE, wxEmptyString, wxDefaultPosition, wxSize(180, -1), wxTE_PROCESS_ENTER);
	toolbar->AddControl( &m_FindTitle );
	toolbar->AddTool(ID_FIND_TITLE, _("Найти"), wxArtProvider::GetBitmap(wxART_FIND), _("Поиск книги по заголовку"));
	toolbar->AddSeparator();
	toolbar->AddTool(wxID_SAVE, _("Экспорт"), wxArtProvider::GetBitmap(wxART_FILE_SAVE), _("Запись на внешнее устройство"));
	toolbar->Realize();

	return toolbar;
}

void FbMainFrame::OnExit(wxCommandEvent & event)
{
	wxUnusedVar(event);
	Close();
}

void FbMainFrame::OnNewZip( wxCommandEvent& event ){

    wxFileDialog dlg (
		this,
		_("Выберите zip-файл для добавления в библиотеку…"),
		wxEmptyString,
		wxEmptyString,
		_("Электронные книги и архивы (*.fb2; *.zip)|*.zip;*.Zip;*.ZIP;*.fb2;*.Fb2;*.FB2|Электронные книги FB2 (*.fb2)|*.fb2;*.Fb2;*.FB2|Архивы ZIP (*.zip)|*.zip;*.Zip;*.ZIP|Все файлы (*.*)|*.*"),
		wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST,
		wxDefaultPosition
    );

	if (dlg.ShowModal() == wxID_OK) {
		wxArrayString paths;
		dlg.GetPaths(paths);

        ZipImportThread *thread = new ZipImportThread(paths);
        thread->m_info = _("Обработка файла:");
        if ( thread->Create() != wxTHREAD_NO_ERROR ) {
            wxLogError(wxT("Can't create thread!"));
            return;
        }
        thread->Run();
	}
}

void FbMainFrame::OnFolder( wxCommandEvent& event ) {

    wxDirDialog dlg (
		this,
		_("Выберите папку для рекурсивного импорта файлов в библиотеку…"),
		wxEmptyString,
		wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST,
		wxDefaultPosition
    );

	if (dlg.ShowModal() == wxID_OK) {
        DirImportThread *thread = new DirImportThread(dlg.GetPath());
        thread->m_info = wxT("Обработка папки:");

        if ( thread->Create() != wxTHREAD_NO_ERROR ) {
            wxLogError(wxT("Can't create thread!"));
            return;
        }
        thread->Run();
	}
}

void FbMainFrame::OnProgressStart(wxUpdateUIEvent& event)
{
	m_ProgressBar.SetRange(event.GetInt());
	m_ProgressBar.SetStatusText(event.GetText(), 0);
	m_ProgressBar.SetStatusText(wxEmptyString, 2);
}

void FbMainFrame::OnProgressUpdate(wxUpdateUIEvent& event)
{
	m_ProgressBar.SetProgress(event.GetInt());
	m_ProgressBar.SetStatusText(event.GetText(), 0);
	m_ProgressBar.SetStatusText(event.GetString(), 2);
}

void FbMainFrame::OnProgressFinish(wxUpdateUIEvent& event)
{
	m_StatusText = wxEmptyString;
	m_ProgressBar.SetProgress(0);
	m_ProgressBar.SetStatusText(wxEmptyString, 0);
	m_ProgressBar.SetStatusText(wxEmptyString, 2);
}

void FbMainFrame::OnError(wxCommandEvent& event)
{
    m_LOGTextCtrl.AppendText(event.GetString() + wxT("\n"));
    ShowPane(wxT("Log"));
}

void FbMainFrame::TogglePaneVisibility(const wxString &pane_name, bool show)
{
	wxAuiPaneInfoArray& all_panes = m_FrameManager.GetAllPanes();
	size_t count = all_panes.GetCount();
	for (size_t i = 0; i < count; ++i) {
		if(all_panes.Item(i).name == pane_name) {
			bool show = ! all_panes.Item(i).IsShown();
			all_panes.Item(i).Show(show);
            m_FrameManager.Update();
            break;
		}
	}
}

void FbMainFrame::ShowPane(const wxString &pane_name)
{
	wxAuiPaneInfoArray& all_panes = m_FrameManager.GetAllPanes();
	size_t count = all_panes.GetCount();
	for (size_t i = 0; i < count; ++i) {
		if(all_panes.Item(i).name == pane_name) {
			all_panes.Item(i).Show(true);
            m_FrameManager.Update();
            break;
		}
	}
}

void FbMainFrame::OnPanelClosed(wxAuiManagerEvent& event)
{
/*
    if (event.pane->name == wxT("Log")) {
        m_LOGTextCtrl.Clear();
    }
*/
}

void FbMainFrame::OnNotebookPageClose(wxAuiNotebookEvent& evt)
{
    wxAuiNotebook* ctrl = (wxAuiNotebook*)evt.GetEventObject();
    if (ctrl->GetPage(evt.GetSelection())->IsKindOf(CLASSINFO(FbFrameAuthor)))
    {
        int res = wxMessageBox(wxT("Are you sure you want to close/hide this notebook page?"),
                       wxT("wxAUI"),
                       wxYES_NO,
                       this);
        if (res != wxYES)
            evt.Veto();
    }
}

void FbMainFrame::OnHideLog(wxCommandEvent& event)
{
    TogglePaneVisibility(wxT("Log"), false);
}

void FbMainFrame::OnFindTitle(wxCommandEvent & event)
{
    FindTitle(m_FindTitle.GetValue());
}

void FbMainFrame::OnFindTitleEnter(wxCommandEvent& event)
{
    FindTitle(event.GetString());
}

void FbMainFrame::FindTitle(const wxString &text)
{
	FbFrameSearch::Execute(this, text);
}

void FbMainFrame::OnFindAuthor(wxCommandEvent& event)
{
    FindAuthor(m_FindAuthor.GetValue());
}

void FbMainFrame::OnFindAuthorEnter(wxCommandEvent& event)
{
    FindAuthor(event.GetString());
}

void FbMainFrame::FindAuthor(const wxString &text)
{
    FbFrameAuthor * authors = wxDynamicCast(FindFrameById(ID_FRAME_AUTHOR, true), FbFrameAuthor);
	if (!authors) {
	    authors = new FbFrameAuthor(this);
		if ( text.IsEmpty() ) authors->SelectRandomLetter();
        GetNotebook()->SetSelection( GetNotebook()->GetPageCount() - 1 );
        authors->Update();
	}
    if ( !text.IsEmpty() ) authors->FindAuthor(text);

    authors->ActivateAuthors();
}

void FbMainFrame::OnMenuAuthor(wxCommandEvent& event)
{
	FindAuthor(wxEmptyString);
}

void FbMainFrame::OnMenuTitle(wxCommandEvent& event)
{
	wxString text = wxGetTextFromUser(_("Введите строку для поиска:"), _("Поиск по заголовку"));
	if (text.IsEmpty()) return;
	FindTitle(text);
}

void FbMainFrame::OnMenuGenres(wxCommandEvent & event)
{
    FbFrameGenres * frame = wxDynamicCast(FindFrameById(ID_FRAME_GENRES, true), FbFrameGenres);
	if (!frame) {
	    frame = new FbFrameGenres(this);
        GetNotebook()->SetSelection( GetNotebook()->GetPageCount() - 1 );
        frame->Update();
	}
}

void FbMainFrame::OnMenuFavour(wxCommandEvent & event)
{
    FbFrameFavour * frame = wxDynamicCast(FindFrameById(ID_FRAME_FAVOUR, true), FbFrameFavour);
	if (!frame) {
	    frame = new FbFrameFavour(this);
        GetNotebook()->SetSelection( GetNotebook()->GetPageCount() - 1 );
        frame->Update();
	}
}

wxWindow * FbMainFrame::FindFrameById(const int id, bool bActivate)
{
	size_t count = GetNotebook()->GetPageCount();
	for (size_t i = 0; i < count; ++i) {
        if (GetNotebook()->GetPage(i)->GetId() == id) {
            wxWindow * result = GetNotebook()->GetPage(i);
            if (bActivate) GetNotebook()->SetSelection(i);
            return result;
		}
	}
	return NULL;
}


void FbMainFrame::OnMenuNothing(wxCommandEvent& event)
{
    wxMessageBox(_("Функционал не реализован в данной версии."));
}

void FbMainFrame::OnDatabaseInfo(wxCommandEvent & event)
{
	FbFrameInfo::Execute();
}

void FbMainFrame::OnVacuum(wxCommandEvent & event)
{
    wxString msg = _("Выполнить реструктуризацию базы данных?");
    if (wxMessageBox(msg, _("Подтверждение"), wxOK | wxCANCEL, this) != wxOK) return;

    VacuumThread::Execute();
}

void FbMainFrame::OnUpdateFolder(wxCommandEvent & event)
{
    FbFrameFavour * frame = wxDynamicCast(FindFrameById(ID_FRAME_FAVOUR, false), FbFrameFavour);
	if (frame) frame->UpdateFolder(event.GetInt(), FT_FOLDER);
}

void FbMainFrame::OnUpdateRating(wxCommandEvent & event)
{
    FbFrameFavour * frame = wxDynamicCast(FindFrameById(ID_FRAME_FAVOUR, false), FbFrameFavour);
	if (frame) frame->UpdateFolder(event.GetInt(), FT_RATING);
}

void FbMainFrame::OnOpenAuthor(wxCommandEvent & event)
{
    FbFrameAuthor * frame = wxDynamicCast(FindFrameById(ID_FRAME_AUTHOR, true), FbFrameAuthor);
	if (!frame) {
	    frame = new FbFrameAuthor(this);
        GetNotebook()->SetSelection( GetNotebook()->GetPageCount() - 1 );
        frame->Update();
	}
	frame->OpenAuthor(event.GetInt());
}

void FbMainFrame::OnInfoCommand(wxCommandEvent & event)
{
    FbFrameInfo * frame = wxDynamicCast(FindFrameById(ID_FRAME_INFO, true), FbFrameInfo);
	if (!frame) {
	    frame = new FbFrameInfo(this);
        GetNotebook()->SetSelection( GetNotebook()->GetPageCount() - 1 );
        frame->Update();
	}
	frame->Load(event.GetString());
}
