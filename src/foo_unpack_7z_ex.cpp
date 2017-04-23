#define UNICODE
#include <SDK/foobar2000.h>
#undef UNICODE

#include "7z/CPP/Windows/Defs.h"
#include "7z/CPP/Windows/PropVariant.h"
#include "7z/CPP/Windows/PropVariantConv.h"
#include "7z/CPP/7zip/Archive/7z/7zHandler.h"

#include <string>

FString FileName(const char* _name_utf8)
{
	FString name;
	auto s = pfc::stringcvt::estimate_utf8_to_wide(_name_utf8);
	auto* buf = name.GetBuf_SetEnd(s);
	pfc::stringcvt::convert_utf8_to_wide(buf, name.Len() + 1, _name_utf8, -1);
	for(auto* c = buf; *c; ++c)
	{
		if(IS_PATH_SEPAR(*c))
			*c = WCHAR_PATH_SEPARATOR;
	}
	return name;
}
std::string FileName(const FString& _name_wide)
{
	std::string name;
	auto s = pfc::stringcvt::estimate_wide_to_utf8(_name_wide.Ptr(), _name_wide.Len());
	name.resize(s);
	pfc::stringcvt::convert_wide_to_utf8((char*)name.c_str(), s + 1, _name_wide.Ptr(), -1);
	for(auto c = name.begin(); c != name.end(); ++c)
	{
		if(IS_PATH_SEPAR(*c))
			*c = '/';
	}
	return name;
}

using namespace NWindows;

static void PrintError(const char *message)
{
	assert(message);
	throw exception_io_data();
}

static HRESULT IsArchiveItemProp(IInArchive *archive, UInt32 index, PROPID propID, bool &result)
{
	NCOM::CPropVariant prop;
	RINOK(archive->GetProperty(index, propID, &prop));
	if(prop.vt == VT_BOOL)
		result = VARIANT_BOOLToBool(prop.boolVal);
	else if(prop.vt == VT_EMPTY)
		result = false;
	else
		return E_FAIL;
	return S_OK;
}

static HRESULT IsArchiveItemFolder(IInArchive *archive, UInt32 index, bool &result)
{
	return IsArchiveItemProp(archive, index, kpidIsDir, result);
}

class CArchiveOpenCallback :
	public IArchiveOpenCallback,
	public ICryptoGetTextPassword,
	public CMyUnknownImp
{
public:
	MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

	STDMETHOD(SetTotal)(const UInt64 *files, const UInt64 *bytes) { return S_OK; }
	STDMETHOD(SetCompleted)(const UInt64 *files, const UInt64 *bytes) { return S_OK; }

	STDMETHOD(CryptoGetTextPassword)(BSTR *password) { return E_ABORT; }
};


class CArchiveExtractCallback :
	public IArchiveExtractCallback,
	public ICryptoGetTextPassword,
	public ISequentialOutStream,
	public CMyUnknownImp
{
public:
	CArchiveExtractCallback(IInArchive* _archive) : archive(_archive), item_proc(NULL) {}

	struct IItemProc
	{
		virtual void OnOk(const char* name, const t_filestats& stat, const std::string& data) = 0;
	};

	MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

	// IProgress
	STDMETHOD(SetTotal)(UInt64 size) { return S_OK; }
	STDMETHOD(SetCompleted)(const UInt64 *completeValue) { return S_OK; }

	// IArchiveExtractCallback
	STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode);
	STDMETHOD(PrepareOperation)(Int32 askExtractMode) { return S_OK; }
	STDMETHOD(SetOperationResult)(Int32 resultEOperationResult);

	// ICryptoGetTextPassword
	STDMETHOD(CryptoGetTextPassword)(BSTR *password) { return E_ABORT; }
	STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize)
	{
		file_data.insert(file_data.length(), (const char*)data, size);
		if(*processedSize)
			*processedSize = size;
		return S_OK;
	}

	t_filestats Stat(UInt32 idx);

	std::string file_data;

	IItemProc* item_proc;

private:
	IInArchive* archive;
	struct CProcessedFileInfo
	{
		CProcessedFileInfo() : index(-1), size(0) { time.dwLowDateTime = time.dwHighDateTime = 0; }
		UInt32 index;
		UInt64 size;
		FILETIME time;
	} fi;
};


STDMETHODIMP CArchiveExtractCallback::GetStream(UInt32 index,
	ISequentialOutStream **outStream, Int32 askExtractMode)
{
	*outStream = NULL;
	fi.index = -1;

	if(askExtractMode != NArchive::NExtract::NAskMode::kExtract)
		return S_OK;

	bool is_dir = false;
	RINOK(IsArchiveItemFolder(archive, index, is_dir));
	if(is_dir)
		return S_OK;

	fi.index = index;
	// Get Size
	NCOM::CPropVariant prop;
	RINOK(archive->GetProperty(index, kpidSize, &prop));
	fi.size = 0;
	ConvertPropVariantToUInt64(prop, fi.size);

	file_data.clear();
	file_data.reserve(fi.size);
	*outStream = this;
	AddRef(); //WTF???

	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32 operationResult)
{
	switch(operationResult)
	{
	case NArchive::NExtract::NOperationResult::kOK:
		break;
	default:
		switch(operationResult)
		{
		case NArchive::NExtract::NOperationResult::kUnsupportedMethod:	break;
		case NArchive::NExtract::NOperationResult::kCRCError:			break;
		case NArchive::NExtract::NOperationResult::kDataError:			break;
		case NArchive::NExtract::NOperationResult::kUnavailable:		break;
		case NArchive::NExtract::NOperationResult::kUnexpectedEnd:		break;
		case NArchive::NExtract::NOperationResult::kDataAfterEnd:		break;
		case NArchive::NExtract::NOperationResult::kIsNotArc:			break;
		case NArchive::NExtract::NOperationResult::kHeadersError:		break;
		}
		PrintError("Operation error");
	}
	if(item_proc && fi.index != -1)
	{
		FString n;
		NCOM::CPropVariant prop;
		archive->GetProperty(fi.index, kpidPath, &prop);
		if(prop.vt == VT_BSTR)
			n = prop.bstrVal;
		auto name = FileName(n);
		if(!name.empty())
		{
			auto stat = Stat(fi.index);
			item_proc->OnOk(name.c_str(), stat, file_data);
		}
		fi.index = -1;
	}
	return S_OK;
}
t_filestats CArchiveExtractCallback::Stat(UInt32 idx)
{
	NCOM::CPropVariant prop;
	archive->GetProperty(idx, kpidSize, &prop);

	UInt64 fileSize = 0;
	ConvertPropVariantToUInt64(prop, fileSize);

	archive->GetProperty(idx, kpidMTime, &prop);

	ULARGE_INTEGER time;
	time.LowPart = prop.filetime.dwLowDateTime;
	time.HighPart = prop.filetime.dwHighDateTime;

	t_filestats ret;
	ret.m_size = fileSize;
	ret.m_timestamp = time.QuadPart;
	return ret;
}


class CInFoobarStream :
	public IInStream,
	public IStreamGetSize,
	public CMyUnknownImp
{
public:
	service_ptr_t< file > m_file;
	abort_callback& m_abort;

	CInFoobarStream(const service_ptr_t<file>& _file, abort_callback& p_abort) : m_file(_file), m_abort(p_abort) {}

	MY_QUERYINTERFACE_BEGIN2(IInStream)
	MY_QUERYINTERFACE_ENTRY(IStreamGetSize)
	MY_QUERYINTERFACE_END
	MY_ADDREF_RELEASE

	STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize)
	{
		auto readed = m_file->read(data, size, m_abort);
		if(processedSize)
			*processedSize = readed;
		return S_OK;
	}
	STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
	{
		switch(seekOrigin)
		{
		case STREAM_SEEK_SET:	m_file->seek_ex(offset, file::seek_from_beginning, m_abort);	break;
		case STREAM_SEEK_CUR:	m_file->seek_ex(offset, file::seek_from_current, m_abort);		break;
		case STREAM_SEEK_END:	m_file->seek_ex(offset, file::seek_from_eof, m_abort);			break;
		default:
			throw exception_io_seek_out_of_range();
		}
		if(newPosition)
			*newPosition = m_file->get_position(m_abort);
		return S_OK;
	}

	STDMETHOD(GetSize)(UInt64 *size)
	{
		*size = m_file->get_size(m_abort);
		return S_OK;
	}
};


class Archive7Z : public NArchive::N7z::CHandler
{
	typedef NArchive::N7z::CHandler inherited;
public:
	Archive7Z(service_ptr_t< file >& file, abort_callback & p_abort)
		: ifs(new CInFoobarStream(file, p_abort)), aoc(new CArchiveOpenCallback)
		, aec(new CArchiveExtractCallback(this))
	{
		const UInt64 scanSize = 1 << 23;
		if(inherited::Open(ifs, &scanSize, aoc) != S_OK)
		{
			throw exception_io_unsupported_format();
		}
	}
	void Process(CArchiveExtractCallback::IItemProc* p, bool stat_only)
	{
		aec->item_proc = p;
		inherited::Extract(NULL, -1, false, aec);
		aec->item_proc = NULL;
	}
	void Extract(const char* file)
	{
		UInt32 idx = FindFile(file);
		if(inherited::Extract(&idx, 1, false, aec))
		{
			throw exception_io_not_found();
		}
	}

	t_filestats Stat(const char* file)
	{
		return aec->Stat(FindFile(file));
	}

	const std::string FileData() const { return aec->file_data; }

private:
	UInt32 FindFile(const char* _name)
	{
		FString name = FileName(_name);
		UInt32 numItems = 0;
		GetNumberOfItems(&numItems);
		for(UInt32 i = 0; i < numItems; i++)
		{
			NCOM::CPropVariant prop;
			GetProperty(i, kpidPath, &prop);
			switch(prop.vt)
			{
			case VT_BSTR:
				if(prop.bstrVal == name)
					return i;
				break;
			case VT_EMPTY:
				break;
			default:
				PrintError("ERROR!");
				break;
			}
		}
		throw exception_io_not_found();
	}

private:
	CMyComPtr<IInStream> ifs;
	CMyComPtr<IArchiveOpenCallback> aoc;
	CMyComPtr<CArchiveExtractCallback> aec;
};


class archive_7z_ex : public archive_impl
{
public:
	virtual bool supports_content_types() { return false; }
	virtual const char * get_archive_type() { return "7z"; }

	virtual t_filestats get_stats_in_archive( const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
		service_ptr_t< file > m_file;
		filesystem::g_open( m_file, p_archive, filesystem::open_mode_read, p_abort );

		Archive7Z archive(m_file, p_abort);
		return archive.Stat(p_file);
	}

	virtual void open_archive( service_ptr_t< file > & p_out, const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
		service_ptr_t< file > m_file;
		filesystem::g_open( m_file, p_archive, filesystem::open_mode_read, p_abort );
		t_filetimestamp timestamp = m_file->get_timestamp( p_abort );

		Archive7Z archive(m_file, p_abort);
		archive.Extract(p_file);
		p_out = new service_impl_t<reader_membuffer_simple>(archive.FileData().data(), archive.FileData().size(), timestamp, m_file->is_remote());
	}

	virtual void archive_list( const char * path, const service_ptr_t< file > & p_reader, archive_callback & p_out, bool p_want_readers )
	{
		if ( stricmp_utf8( pfc::string_extension( path ), "7z" ) )
			throw exception_io_data();

		service_ptr_t< file > m_file = p_reader;
		if ( m_file.is_empty() )
			filesystem::g_open( m_file, path, filesystem::open_mode_read, p_out );
		else
			m_file->reopen( p_out );

		Archive7Z archive(m_file, p_out);

		struct CItemProc : public CArchiveExtractCallback::IItemProc
		{
			CItemProc(archive_impl* _owner, archive_callback& _p_out, const std::string& _path, bool _is_remote)
				: owner(_owner), p_out(_p_out), path(_path), is_remote(_is_remote)
			{
			}
			virtual void OnOk(const char* name, const t_filestats& stat, const std::string& data)
			{
				service_ptr_t<file> m_out_file;
				if(data.size())
					m_out_file = new service_impl_t<reader_membuffer_simple>(data.data(), data.size(), stat.m_timestamp, is_remote);
				pfc::string8_fastalloc m_path;
				owner->make_unpack_path(m_path, path.c_str(), name);
				if(!p_out.on_entry(owner, m_path, stat, m_out_file))
					PrintError("Stop");
			}
			bool is_remote;
			archive_impl* owner;
			archive_callback& p_out;
			std::string path;
		};
		CItemProc proc(this, p_out, path, m_file->is_remote());
		archive.Process(&proc, !p_want_readers);
	}
};

class unpacker_7z_ex : public unpacker
{
	inline bool skip_ext( const char * p )
	{
		static const char * exts[] = { "txt", "nfo", "info", "diz" };
		pfc::string_extension ext( p );
		for ( unsigned n = 0; n < tabsize( exts ); ++n )
		{
			if ( ! stricmp_utf8( ext, exts[ n ] ) ) return true;
		}
		return false;
	}

public:
	virtual void open( service_ptr_t< file > & p_out, const service_ptr_t< file > & p_source, abort_callback & p_abort )
	{
		if ( p_source.is_empty() ) throw exception_io_data();

/*		foobar_File_Reader in( p_source, p_abort );
		Zip7_Extractor ex;
		handle_error( ex.open( &in ) );
		while ( ! ex.done() )
		{
			handle_error( ex.stat() );
			if ( ! skip_ext( ex.name() ) )
			{
				const void * data;
				handle_error( ex.data( &data ) );
				p_out = new service_impl_t<reader_membuffer_simple>( data, (t_size)ex.size(), p_source->get_timestamp( p_abort ), p_source->is_remote() );
				return;
			}
			handle_error( ex.next() );
		}
		throw exception_io_data();
*/
	}
};

static archive_factory_t < archive_7z_ex >  g_archive_7z_ex_factory;
static unpacker_factory_t< unpacker_7z_ex > g_unpacker_7z_ex_factory;

DECLARE_COMPONENT_VERSION("7-ZIP Reader Ex", "0.01", "(C) 2017 djdron");
DECLARE_FILE_TYPE_EX("7Z", "7-Zip Archive", "7-Zip Archives");
VALIDATE_COMPONENT_FILENAME("foo_unpack_7z_ex.dll");
