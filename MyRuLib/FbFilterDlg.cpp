#include "FbFilterDlg.h"
#include <wx/imaglist.h>
#include "FbBookList.h"
#include "FbLogoBitmap.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( FbFilterDlg, wxDialog )
	EVT_BUTTON( wxID_NO, FbFilterDlg::OnNoButton )
END_EVENT_TABLE()

FbFilterDlg::FbFilterDlg(FbFilterObj & filter)
	: FbDialog( NULL, wxID_ANY, _("Настройка фильтра"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerOwner;
	sbSizerOwner = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Отбор книг по принадлежности") ), wxHORIZONTAL );

	m_checkLib = new wxCheckBox( this, ID_CHECK_LIB, _("Библиотечные"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerOwner->Add( m_checkLib, 0, wxALL|wxEXPAND, 5 );

	m_checkUsr = new wxCheckBox( this, ID_CHECK_USR, _("Собственные"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerOwner->Add( m_checkUsr, 0, wxALL|wxEXPAND, 5 );

	bSizerMain->Add( sbSizerOwner, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizerList;
	bSizerList = new wxBoxSizer( wxHORIZONTAL );

	m_treeLang = CreateTree(_("Язык"));;
	bSizerList->Add( m_treeLang, 1, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_treeType = CreateTree(_("Тип файла"));;
	bSizerList->Add( m_treeType, 1, wxEXPAND|wxALL, 5 );

	bSizerMain->Add( bSizerList, 1, wxEXPAND, 5 );

	wxStdDialogButtonSizer * sdbSizer = CreateStdDialogButtonSizer( wxYES | wxNO | wxCANCEL );
	bSizerMain->Add( sdbSizer, 0, wxEXPAND|wxALL, 5 );

	this->SetSizer( bSizerMain );
	this->Layout();
}

FbTreeListCtrl * FbFilterDlg::CreateTree(const wxString & title)
{
	FbBookList * treelist = new FbBookList( this, ID_TREE_TYPE, wxTR_FULL_ROW_HIGHLIGHT | wxTR_MULTIPLE | wxSUNKEN_BORDER );
	treelist->AddColumn (title, 10, wxALIGN_LEFT);
	treelist->SetMinSize( wxSize( 100,100 ) );
	treelist->SetItemBold( treelist->AddRoot(wxT("Все"), 0), true );

	wxBitmap size = wxBitmap(checked_xpm);
	wxImageList *images;
	images = new wxImageList (size.GetWidth(), size.GetHeight(), true);
	images->Add (wxBitmap(nocheck_xpm));
	images->Add (wxBitmap(checked_xpm));
	images->Add (wxBitmap(checkout_xpm));
	treelist->AssignImageList (images);

	return treelist;
}

void FbFilterDlg::OnNoButton( wxCommandEvent& event )
{
	wxDialog::EndModal( wxID_NO );
}

bool FbFilterDlg::Execute(FbFilterObj & filter)
{
	FbFilterDlg dlg(filter);
	int res = dlg.ShowModal();

	switch ( res ) {
		case wxID_YES: {
			filter.m_enabled = true;
		} break;
		case wxID_NO: {
			filter.m_enabled = false;
		} break;
	}

	return  res != wxID_CANCEL;
}

