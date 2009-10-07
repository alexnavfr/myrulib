#include "FbFrameGenres.h"
#include <wx/artprov.h>
#include "FbConst.h"
#include "FbDatabase.h"
#include "FbManager.h"
#include "BooksPanel.h"
#include "FbGenres.h"
#include "FbFrameBaseThread.h"

BEGIN_EVENT_TABLE(FbFrameGenres, FbFrameBase)
    EVT_MENU(ID_MODE_TREE, FbFrameGenres::OnChangeMode)
    EVT_MENU(ID_MODE_LIST, FbFrameGenres::OnChangeMode)
    EVT_TREE_SEL_CHANGED(ID_GENRES_TREE, FbFrameGenres::OnGenreSelected)
END_EVENT_TABLE()

FbFrameGenres::FbFrameGenres(wxAuiMDIParentFrame * parent, wxWindowID id,const wxString & title)
    :FbFrameBase(parent, id, title)
{
    CreateControls();
}

void FbFrameGenres::CreateControls()
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	SetMenuBar(CreateMenuBar());

	wxToolBar * toolbar = CreateToolBar(wxTB_FLAT|wxTB_NODIVIDER|wxTB_HORZ_TEXT, wxID_ANY, GetTitle());
	bSizer1->Add( toolbar, 0, wxGROW);

	wxSplitterWindow * splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxSize(500, 400), wxSP_NOBORDER);
	splitter->SetMinimumPaneSize(50);
	splitter->SetSashGravity(0.33);
	bSizer1->Add(splitter, 1, wxEXPAND);

	long style = wxTR_HIDE_ROOT | wxTR_FULL_ROW_HIGHLIGHT | wxSUNKEN_BORDER | wxTR_NO_BUTTONS;
	m_GenresList = new FbTreeListCtrl(splitter, ID_GENRES_TREE, style);
    m_GenresList->AddColumn (_("Список жанров"), 100, wxALIGN_LEFT);
	m_GenresList->SetFocus();
	FbGenres::FillControl(m_GenresList);

	long substyle = wxTR_HIDE_ROOT | wxTR_FULL_ROW_HIGHLIGHT | wxTR_COLUMN_LINES | wxTR_MULTIPLE | wxSUNKEN_BORDER;
	m_BooksPanel.Create(splitter, wxID_ANY, wxDefaultPosition, wxSize(500, 400), wxNO_BORDER, substyle);
	splitter->SplitVertically(m_GenresList, &m_BooksPanel, 160);

    m_BooksPanel.CreateColumns(GetListMode(FB_MODE_GENRES));

	SetSizer( bSizer1 );
	Layout();
}

wxToolBar * FbFrameGenres::CreateToolBar(long style, wxWindowID winid, const wxString& name)
{
    wxToolBar * toolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, style, name);
	toolbar->AddTool(wxID_SAVE, _("Экспорт"), wxArtProvider::GetBitmap(wxART_FILE_SAVE), _("Запись на внешнее устройство"));
	toolbar->Realize();
    return toolbar;
}

class FrameGenresThread: public FbFrameBaseThread
{
    public:
        FrameGenresThread(FbFrameGenres * frame, FbListMode mode, const int code)
			:FbFrameBaseThread(frame, mode), m_code(code), m_number(sm_skiper.NewNumber()) {};
        virtual void *Entry();
    private:
		static FbThreadSkiper sm_skiper;
        int m_code;
        int m_number;
};

FbThreadSkiper FrameGenresThread::sm_skiper;

void * FrameGenresThread::Entry()
{
	wxCriticalSectionLocker locker(sm_queue);

	if (sm_skiper.Skipped(m_number)) return NULL;
	EmptyBooks();

	wxString condition = wxString::Format(wxT("GENRE(books.genres, ?)"), m_code);
	wxString sql = GetSQL(condition);

	FbCommonDatabase database;
	FbGenreFunction function;
    database.CreateFunction(wxT("GENRE"), 2, function);
    wxSQLite3Statement stmt = database.PrepareStatement(sql);
    stmt.Bind(1, wxString::Format(wxT("%02X"), m_code));
    wxSQLite3ResultSet result = stmt.ExecuteQuery();

	if (sm_skiper.Skipped(m_number)) return NULL;
    FillBooks(result);

	return NULL;
}

void FbFrameGenres::OnGenreSelected(wxTreeEvent & event)
{
	wxTreeItemId selected = event.GetItem();
	if (selected.IsOk()) {
        m_BooksPanel.EmptyBooks();
		FbGenreData * data = (FbGenreData*) m_GenresList->GetItemData(selected);
		if (data) {
			wxThread * thread = new FrameGenresThread(this, m_BooksPanel.GetListMode(), data->GetCode());
			if ( thread->Create() == wxTHREAD_NO_ERROR ) thread->Run();
		}
	}
}

void FbFrameGenres::OnChangeMode(wxCommandEvent& event)
{
	FbListMode mode = event.GetId() == ID_MODE_TREE ? FB2_MODE_TREE : FB2_MODE_LIST;
	SetListMode(FB_MODE_GENRES, mode);

	m_BooksPanel.CreateColumns(mode);

	int code = 0;
	wxTreeItemId selected = m_GenresList->GetSelection();
	if (selected.IsOk()) {
		FbGenreData * data = (FbGenreData*) m_GenresList->GetItemData(selected);
		if (data) code = data->GetCode();
	}

	if ( code ) {
		wxThread * thread = new FrameGenresThread(this, m_BooksPanel.GetListMode(), code);
		if ( thread->Create() == wxTHREAD_NO_ERROR ) thread->Run();
	}
}