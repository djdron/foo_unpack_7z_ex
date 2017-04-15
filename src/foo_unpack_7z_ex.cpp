#define UNICODE
#include <SDK/foobar2000.h>
#undef UNICODE

#include "7z/CPP/7zip/Archive/IArchive.h"

class archive_7z_ex : public archive_impl
{
public:
	virtual bool supports_content_types() { return false; }
	virtual const char * get_archive_type() { return "7z"; }

	virtual t_filestats get_stats_in_archive( const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
		service_ptr_t< file > m_file;
		filesystem::g_open( m_file, p_archive, filesystem::open_mode_read, p_abort );
		foobar_File_Reader in( m_file, p_abort );
		Zip7_Extractor ex;
		handle_error( ex.open( &in ) );
		while ( ! ex.done() )
		{
			handle_error( ex.stat() );
			if ( ! strcmp( ex.name(), p_file ) ) break;
			handle_error( ex.next() );
		}
		if ( ex.done() ) throw exception_io_not_found();
		t_filestats ret;
		ret.m_size = ex.size();
		ret.m_timestamp = m_file->get_timestamp( p_abort );
		return ret;
	}

	virtual void open_archive( service_ptr_t< file > & p_out, const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
		service_ptr_t< file > m_file;
		filesystem::g_open( m_file, p_archive, filesystem::open_mode_read, p_abort );
		t_filetimestamp timestamp = m_file->get_timestamp( p_abort );
		foobar_File_Reader in( m_file, p_abort );
		Zip7_Extractor ex;
		handle_error( ex.open( &in ) );
		while ( ! ex.done() )
		{
			handle_error( ex.stat() );
			if ( ! strcmp( ex.name(), p_file ) ) break;
			handle_error( ex.next() );
		}
		if ( ex.done() ) throw exception_io_not_found();
		const void * data;
		handle_error( ex.data( &data ) );
		p_out = new service_impl_t<reader_membuffer_simple>( data, (t_size)ex.size(), timestamp, m_file->is_remote() );
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

		t_filetimestamp timestamp = m_file->get_timestamp( p_out );

		foobar_File_Reader in( m_file, p_out );
		Zip7_Extractor ex;
		handle_error( ex.open( &in ) );
		// if ( ! p_want_readers ) ex.scan_only(); // this is only needed for Rar_Extractor
		pfc::string8_fastalloc m_path;
		service_ptr_t<file> m_out_file;
		t_filestats m_stats;
		m_stats.m_timestamp = timestamp;
		while ( ! ex.done() )
		{
			handle_error( ex.stat() );
			make_unpack_path( m_path, path, ex.name() );
			m_stats.m_size = ex.size();
			if ( p_want_readers )
			{
				const void * data;
				handle_error( ex.data( &data ) );
				m_out_file = new service_impl_t<reader_membuffer_simple>( data, (t_size)m_stats.m_size, timestamp, m_file->is_remote() );
			}
			if ( ! p_out.on_entry( this, m_path, m_stats, m_out_file ) ) break;
			handle_error( ex.next() );
		}
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

		foobar_File_Reader in( p_source, p_abort );
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
	}
};

static archive_factory_t < archive_7z_ex >  g_archive_7z_ex_factory;
static unpacker_factory_t< unpacker_7z_ex > g_unpacker_7z_ex_factory;

DECLARE_COMPONENT_VERSION("7-ZIP Reader Ex", "0.01", "(C) 2017 djdron");
DECLARE_FILE_TYPE_EX("7Z", "7-Zip Archive", "7-Zip Archives");
VALIDATE_COMPONENT_FILENAME("foo_unpack_7z_ex.dll");
