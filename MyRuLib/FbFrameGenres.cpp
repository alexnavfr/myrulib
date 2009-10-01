#include "FbFrameGenres.h"
#include <wx/artprov.h>
#include "FbConst.h"
#include "FbDatabase.h"
#include "FbManager.h"
#include "BooksPanel.h"
#include "ExternalDlg.h"
#include "FbGenres.h"

BEGIN_EVENT_TABLE(FbFrameGenres, FbFrameBase)
    EVT_MENU(wxID_SAVE, FbFrameGenres::OnExternal)
    EVT_COMMAND(ID_EMPTY_BOOKS, fbEVT_BOOK_ACTION, FbFrameGenres::OnEmptyBooks)
    EVT_FB_BOOK(ID_APPEND_BOOK, FbFrameGenres::OnAppendBook)
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

	long substyle = wxTR_NO_LINES | wxTR_HIDE_ROOT | wxTR_FULL_ROW_HIGHLIGHT | wxTR_COLUMN_LINES | wxTR_MULTIPLE | wxSUNKEN_BORDER;
	m_BooksPanel.Create(splitter, wxID_ANY, wxDefaultPosition, wxSize(500, 400), wxNO_BORDER, substyle);
	splitter->SplitVertically(m_GenresList, &m_BooksPanel, 160);

    m_BooksPanel.CreateColumns(FB2_MODE_LIST);

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

void FbFrameGenres::OnExternal(wxCommandEvent& event)
{
    ExternalDlg::Execute(this, m_BooksPanel.m_BookList);
}

class GenresThread: public wxThread
{
    public:
        GenresThread(FbFrameGenres * frame, const int code)
			:m_frame(frame), m_code(code) {};
        virtual void *Entry();
    private:
		static wxCriticalSection sm_queue;
		FbFrameGenres * m_frame;
		BooksPanel * m_books;
        int m_code;
};

class

wxCriticalSection GenresThread::sm_queue;

void * GenresThread::Entry()
{
	wxCriticalSectionLocker locker(sm_queue);

	if (FbFrameGenres::GetCode() != m_code) return NULL;

	wxCommandEvent event(fbEVT_BOOK_ACTION, ID_EMPTY_BOOKS);
	event.SetInt(m_code);
	wxPostEvent(m_frame, event);

	wxString sql = wxT("\
        SELECT books.id, books.title, books.file_name, books.file_type, books.file_size, authors.full_name, 0 as number \
        FROM books LEFT JOIN authors ON books.id_author = authors.id \
        WHERE GENRE(books.genres, ?) \
        ORDER BY books.title, books.id, authors.full_name\
    ");

	FbCommonDatabase database;
	FbGenreFunction function;
    database.CreateFunction(wxT("GENRE"), 2, function);
    wxSQLite3Statement stmt = database.PrepareStatement(sql);
    stmt.Bind(1, wxString::Format(wxT("%02X"), m_code));
    wxSQLite3ResultSet result = stmt.ExecuteQuery();
    result.NextRow();

    while (!result.Eof()) {
		if (FbFrameGenres::GetCode() != m_code) return NULL;
        BookTreeItemData data(result);
        wxString full_name = result.GetString(wxT("full_name"));
        do {
            result.NextRow();
            if ( data.GetId() != result.GetInt(wxT("id")) ) break;
            full_name = full_name + wxT(", ") + result.GetString(wxT("full_name"));
        } while (!result.Eof());

		FbBookEvent event(fbEVT_BOOK_ACTION, ID_APPEND_BOOK, &data);
		event.SetString(full_name);
		event.SetInt(m_code);
		wxPostEvent(m_frame, event);
    }
    return NULL;
}

wxCriticalSection FbFrameGenres::sm_queue;

int FbFrameGenres::sm_code = 0;

int FbFrameGenres::GetCode()
{
	wxCriticalSectionLocker locker(sm_queue);
	return sm_code;
}

void FbFrameGenres::SetCode(const int code)
{
	wxCriticalSectionLocker locker(sm_queue);
	sm_code = code;
}

void FbFrameGenres::OnGenreSelected(wxTreeEvent & event)
{
	wxTreeItemId selected = event.GetItem();
	if (selected.IsOk()) {
        m_BooksPanel.EmptyBooks();
		FbGenreData * data = (FbGenreData*) m_GenresList->GetItemData(selected);
		if (data) {
			SetCode(data->GetCode());
			wxThread * thread = new GenresThread(this, data->GetCode());
			if ( thread->Create() == wxTHREAD_NO_ERROR ) thread->Run();
		} else {
			SetCode(0);
		}
	}
}

void FbFrameGenres::OnEmptyBooks(wxCommandEvent& event)
{
	if ( sm_code == event.GetInt() ) m_BooksPanel.EmptyBooks();
}

void FbFrameGenres::OnAppendBook(FbBookEvent& event)
{
	if ( sm_code == event.GetInt() ) {
        m_BooksPanel.AppendBook( new BookTreeItemData(event.m_data), event.GetString() );
	}
}
