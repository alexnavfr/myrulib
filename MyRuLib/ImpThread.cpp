#include <wx/zipstrm.h>
#include <wx/dir.h>
#include "ImpThread.h"
#include "FbParams.h"
#include "FbManager.h"
#include "FbGenres.h"
#include "MyRuLibApp.h"
#include "ZipReader.h"
#include "sha1/sha1.h"
#include "wx/base64.h"
#include "db/Files.h"

extern wxString strAlphabet;
extern wxString strRusJO;
extern wxString strRusJE;

extern "C" {
static void StartElementHnd(void *userData, const XML_Char *name, const XML_Char **atts)
{
    ImportParsingContext *ctx = (ImportParsingContext*)userData;
    wxString node_name = ctx->CharToLower(name);
    wxString path = ctx->Path();

	if (path == wxT("/fictionbook/description/title-info")) {
	    if (node_name == wxT("author")) {
            ctx->author = new AuthorItem;
	        ctx->authors.Add(ctx->author);
	    } else if (node_name == wxT("sequence")) {
	        SequenceItem * seqitem = new SequenceItem;
            ctx->sequences.Add(seqitem);
            const XML_Char **a = atts;
            while (*a) {
                wxString attr = ctx->CharToLower(a[0]).Trim(false).Trim(true); a++;
                wxString text = ctx->CharToString(a[0]).Trim(false).Trim(true); a++;
                if (attr == wxT("name")) {
                    seqitem->seqname = text;
                } else if (attr == wxT("number")) {
                    text.ToLong(&seqitem->number);
                }
            }
	    }
	}
	ctx->AppendTag(node_name);
    ctx->text.Empty();
}
}

extern "C" {
static void EndElementHnd(void *userData, const XML_Char* name)
{
    ImportParsingContext *ctx = (ImportParsingContext*)userData;
    wxString node_name = ctx->CharToLower(name);
	ctx->RemoveTag(node_name);
    wxString path = ctx->Path();

	if (path == wxT("/fictionbook/description/title-info")) {
        ctx->text.Trim(false).Trim(true);
	    if (node_name == wxT("book-title")) {
	        ctx->title = ctx->text;
	    } else if (node_name == wxT("genre")) {
            ctx->genres += FbGenres::Char(ctx->text);
	    }
	} else if (path == wxT("/fictionbook/description/title-info/author")) {
        ctx->text.Trim(false).Trim(true);
        if (node_name == wxT("first-name"))
            ctx->author->first = ctx->text;
        if (node_name == wxT("middle-name"))
            ctx->author->middle = ctx->text;
        if (node_name == wxT("last-name"))
            ctx->author->last = ctx->text;
	} else if (path == wxT("/fictionbook/description")) {
	    if (node_name == wxT("title-info")) {
	        ctx->Stop();
	    }
	}
}
}

extern "C" {
static void TextHnd(void *userData, const XML_Char *s, int len)
{
    ImportParsingContext *ctx = (ImportParsingContext*)userData;

    wxString str = ctx->CharToString(s, len);

    if (!ctx->IsWhiteOnly(str)) ctx->text += str;
}
}

void ImportThread::OnExit()
{
}

bool ImportThread::LoadXml(wxInputStream& stream, ImportParsingContext &ctx)
{
    const size_t BUFSIZE = 1024;
    unsigned char buf[BUFSIZE];
    bool done;

    sha1_context sha1;

    XML_SetElementHandler(ctx.GetParser(), StartElementHnd, EndElementHnd);
    XML_SetCharacterDataHandler(ctx.GetParser(), TextHnd);

    sha1_starts( &sha1 );

    bool ok = true;
    bool skip = ctx.sha1only;

    do {
        size_t len = stream.Read(buf, BUFSIZE).LastRead();
        done = (len < BUFSIZE);

		sha1_update( &sha1, buf, (int) len );

		if (!skip) {
			if ( !XML_Parse(ctx.GetParser(), (char *)buf, len, done) ) {
				XML_Error error_code = XML_GetErrorCode(ctx.GetParser());
				if ( error_code == XML_ERROR_ABORTED ) {
					skip = true;
				} else {
					wxString error(XML_ErrorString(error_code), *wxConvCurrent);
					wxLogError(_("XML parsing error: '%s' at line %d"), error.c_str(), XML_GetCurrentLineNumber(ctx.GetParser()));
					skip = true;
					ok = false;
					break;
				}
			}
        }
    } while (!done);

    unsigned char output[20];
    sha1_finish( &sha1, output );
	ctx.sha1sum = wxBase64Encode(output, 20).Left(27);

    for (size_t i=0; i<ctx.authors.Count(); i++)
		ctx.authors[i].Convert();

	if (ctx.authors.Count() == 0)
		ctx.authors.Add(new AuthorItem);

    for (size_t i=0; i<ctx.sequences.Count(); i++)
        ctx.sequences[i].Convert();

    return ok;
}

void ImportThread::AppendBook(ImportParsingContext &info, const wxString &name, const wxFileOffset size, int id_archive)
{
	long id_book = 0;
	{
		wxCriticalSectionLocker enter(wxGetApp().m_DbSection);
		id_book = - BookInfo::NewId(DB_NEW_BOOK);
	}

	for (size_t i = 0; i<info.authors.Count(); i++) {
		wxCriticalSectionLocker enter(wxGetApp().m_DbSection);
		Books books(wxGetApp().GetDatabase());
		Bookseq bookseq(wxGetApp().GetDatabase());

		BooksRow * row = books.New();
		row->id = id_book;
		row->id_author = info.authors[i].id;
		row->title = info.title;
		row->genres = info.genres;
		row->file_size = size;
		row->file_name = name;
		row->file_type = wxFileName(name).GetExt().Lower();
		row->id_archive = id_archive;
		row->sha1sum = info.sha1sum;
		row->Save();

		for (size_t j = 0; j<info.sequences.Count(); j++) {
			BookseqRow * seqRow = bookseq.New();
			seqRow->id_book = id_book;
			seqRow->id_seq = info.sequences[j].id;
			seqRow->id_author = info.authors[i].id;
			seqRow->number = info.sequences[j].number;
			seqRow->Save();
		}
	}
}

int ImportThread::FindBySHA1(const wxString &sha1sum)
{
	wxCriticalSectionLocker enter(wxGetApp().m_DbSection);
	DatabaseLayer * database = wxGetApp().GetDatabase();

	wxString sql = wxT("SELECT DISTINCT id FROM books WHERE sha1sum=?");
	PreparedStatement * pStatement = database->PrepareStatement(sql);
	pStatement->SetParamString(1, sha1sum);
	DatabaseResultSet * result = pStatement->ExecuteQuery();

    int id = 0;
	if (result && result->Next())
        id = result->GetResultInt(wxT("id"));

    database->CloseResultSet(result);
    database->CloseStatement(pStatement);
	return id;
}

int ImportThread::FindBySize(const wxString &sha1sum, wxFileOffset size)
{
    wxArrayInt books;

    {
        wxCriticalSectionLocker enter(wxGetApp().m_DbSection);
        DatabaseLayer * database = wxGetApp().GetDatabase();

        wxString sql = wxT("SELECT DISTINCT id FROM books WHERE file_size=? AND (sha1sum='' OR sha1sum IS NULL)");
        PreparedStatement* pStatement = database->PrepareStatement(sql);
        pStatement->SetParamInt(1, size);
        DatabaseResultSet* result = pStatement->ExecuteQuery();

        while (result && result->Next()) {
            int id = result->GetResultInt(wxT("id"));
            books.Add(id);
        }
        database->CloseResultSet(result);
        database->CloseStatement(pStatement);
    }

    for (size_t i=0; i<books.Count(); i++) {

		ZipReader book(books[i]);
		if (!book.IsOK()) continue;

		ImportParsingContext info;
		info.sha1only = true;
		LoadXml(book.GetZip(), info);

        wxCriticalSectionLocker enter(wxGetApp().m_DbSection);
        DatabaseLayer * database = wxGetApp().GetDatabase();

		wxString sql = wxT("UPDATE books SET sha1sum=? WHERE id=?");
		PreparedStatement* pStatement = database->PrepareStatement(sql);
		pStatement->SetParamString(1, info.sha1sum);
		pStatement->SetParamInt(2, books[i]);
		pStatement->ExecuteUpdate();
        database->CloseStatement(pStatement);
		if (info.sha1sum == sha1sum) return books[i];
	}

	return 0;
}

bool ImportThread::ParseXml(wxInputStream& stream, const wxString &filename, const int id_archive)
{
    ImportParsingContext info;

	if (LoadXml(stream, info)) {
		int id_book = FindBySHA1(info.sha1sum);
		if (id_book == 0)
			id_book = FindBySize(info.sha1sum, stream.GetLength());
		if (id_book == 0)
			AppendBook(info, filename, stream.GetLength(), id_archive);
		else
			AppendFile(id_book, id_archive, filename);
        return true;
	}
    return false;
}

void ImportThread::AppendFile(const int id_book, const int id_archive, const wxString &file_name)
{
	wxCriticalSectionLocker enter(wxGetApp().m_DbSection);

	Files files (wxGetApp().GetDatabase());
	FilesRow * row = files.Find(id_book, id_archive);

	if (row) {
	    if (row->file_name == file_name) return;
	} else {
        row = files.New();
        row->id_book = id_book;
        row->id_archive = id_archive;
	}

	row->file_name = file_name;
	row->Save();
}

int ImportThread::AddArchive(const wxString &filename, const int file_size, const int file_count)
{
    wxFileName file(filename);
    wxString name = file.GetFullName();
    wxString path = file.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);

    wxCriticalSectionLocker enter(wxGetApp().m_DbSection);

	Archives archives(wxGetApp().GetDatabase());

	ArchivesRow * row = archives.FindFile(name, path);
	if (row) return row->id;

	row = archives.New();
	row->id = BookInfo::NewId(DB_NEW_ARCHIVE);
	row->file_name = name;
	row->file_path = path;
	row->file_size = file_size;
	row->file_count = file_count;
	row->Save();
	return row->id;
}

void *ZipImportThread::Entry()
{
    wxCriticalSectionLocker enter(sm_queue);

    AutoTransaction trans;

	wxFFileInputStream in(m_filename);
	wxZipInputStream zip(in);

	int id_archive = AddArchive(m_filename, in.GetLength(), zip.GetTotalEntries());

    DoStart(zip.GetTotalEntries(), wxFileName(m_filename).GetFullName());

	while (wxZipEntry * entry = zip.GetNextEntry()) {
		if (entry->GetSize()) {
			wxString filename = entry->GetName(wxPATH_UNIX);
			if (filename.Right(4).Lower() == wxT(".fb2")) {
			    DoStep(filename);
				zip.OpenEntry(*entry);
                ParseXml(zip, filename, id_archive);
			}
		}
		delete entry;
	}

	DoFinish();

	return NULL;
}

class CountTraverser : public wxDirTraverser
{
public:
    CountTraverser() : m_count(0) { }
    virtual wxDirTraverseResult OnFile(const wxString& filename) {
		wxString ext = filename.Right(4).Lower();
		if (ext== wxT(".fb2")) {
            m_count++;
        } else if (ext== wxT(".zip")) {
            m_count++;
        }
        return wxDIR_CONTINUE;
    }
    virtual wxDirTraverseResult OnDir(const wxString& WXUNUSED(dirname)) {
        return wxDIR_CONTINUE;
    }
    unsigned int GetCount() { return m_count; }
private:
    unsigned int m_count;
};

class FolderTraverser : public wxDirTraverser
{
public:
    FolderTraverser(DirImportThread* thread) : m_thread(thread), m_progress(0) { }

    virtual wxDirTraverseResult OnFile(const wxString& filename) {
		wxString ext = filename.Right(4).Lower();
		if (ext == wxT(".fb2")) {
		    Progress(filename);
		    wxFFileInputStream file(filename);
            m_thread->ParseXml(file, filename, 0);
        } else if (ext == wxT(".zip")) {
		    Progress(filename);
            m_thread->ParseZip(filename);
        }
        return wxDIR_CONTINUE;
    }

    virtual wxDirTraverseResult OnDir(const wxString& WXUNUSED(dirname))  {
        return wxDIR_CONTINUE;
    }
private:
    void Progress(const wxString &filename) {
        m_thread->DoStep( wxFileName(filename).GetFullName() );
    }
private:
    DirImportThread *m_thread;
    unsigned int m_progress;
};

void *DirImportThread::Entry()
{
    wxCriticalSectionLocker enter(sm_queue);

    AutoTransaction trans;

    wxDir dir(m_dirname);
    if ( !dir.IsOpened() ) return NULL;

	{
        DoStart(0, m_dirname);
        CountTraverser counter;
        dir.Traverse(counter);
        DoStart(counter.GetCount(), m_dirname);
	}

    FolderTraverser traverser(this);
    dir.Traverse(traverser);

    DoFinish();

	return NULL;
}

bool DirImportThread::ParseZip(const wxString &filename)
{
	wxFFileInputStream in(filename);
	wxZipInputStream zip(in);

	int id_archive = AddArchive(filename, in.GetLength(), zip.GetTotalEntries());

	while (wxZipEntry * entry = zip.GetNextEntry()) {
		if (entry->GetSize()) {
			wxString filename = entry->GetName(wxPATH_UNIX);
			if (filename.Right(4).Lower() == wxT(".fb2")) {
				zip.OpenEntry(*entry);
                ParseXml(zip, filename, id_archive);
			}
		}
		delete entry;
	}

    return true;
}


