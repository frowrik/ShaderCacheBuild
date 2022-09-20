#pragma once

//
const char* validstrtargets[] = {
    "ps_5_0", "ps_5_1", "ps_6_0", "ps_6_1", "ps_6_2", "ps_6_3", "ps_6_4", "ps_6_5", "ps_6_6", "ps_6_7", "vs_5_0", "vs_5_1", "vs_6_0", "vs_6_1", "vs_6_2", "vs_6_3", "vs_6_4", "vs_6_5", "vs_6_6", "vs_6_7", "gs_5_0", "gs_5_1", "gs_6_0", "gs_6_1", "gs_6_2", "gs_6_3", "gs_6_4", "gs_6_5", "gs_6_6", "gs_6_7",
    "hs_5_0", "hs_5_1", "hs_6_0", "hs_6_1", "hs_6_2", "hs_6_3", "hs_6_4", "hs_6_5", "hs_6_6", "hs_6_7", "ds_5_0", "ds_5_1", "ds_6_0", "ds_6_1", "ds_6_2", "ds_6_3", "ds_6_4", "ds_6_5", "ds_6_6", "ds_6_7", "cs_5_0", "cs_5_1", "cs_6_0", "cs_6_1", "cs_6_2", "cs_6_3", "cs_6_4", "cs_6_5", "cs_6_6", "cs_6_7",
    "lib_6_1",
    "lib_6_2",
    "lib_6_3",
    "lib_6_4"
    "lib_6_5",
    "lib_6_6",
    "lib_6_7",
    "ms_6_5",
    "ms_6_6",
    "ms_6_7",
    "as_6_5",
    "as_6_6",
    "as_6_7" 
};

//
enum class shader_type { none, vertex, pixel, geom, hull, domain, compute, lib, mesh, as };

const char* shader_type_name[] = { "none", "vs", "ps", "geom", "hs", "ds", "cs", "lib", "ms", "as" };

//
struct source_desc {
    std::string      path          = "";
    size_t           filesize      = 0;
    std::time_t      filetimewrite = 0;
    size_t           datahash      = 0;
    std::string      data          = "";
    std::vector<int> includes_idx;           // индексы инклуд сорцов
    bool             is_in_cache   = false;  // найден ли кеш для шейдера
    bool             is_include    = true;   // исходник это инклуд
    bool             is_needupdate = true;   // кеш не совпадает надо обновить
};

//
struct shader_desc {
    std::string     path          = "";
    std::string     name          = "";
    std::string     namefile      = "";
    std::string     hlslversion    = "";
    shader_type     hlsl_type     = shader_type::none;
    uint8_t         hlsl_major    = 6;
    uint8_t         hlsl_minor    = 0;
    std::string     entry         = "";
    int             source_idx    = 0;
    xmldesc_shader* xmlshader     = nullptr;
    bool            is_recompile  = false;  // надо ли рекомпилить шейдер
    bool            is_badcompile = false;  // произошла ошибка
};

struct shadercache {
public:
    
    shadercachexml* xml_data = nullptr;
    
    std::vector<shader_desc> shaderlist;
    std::vector<source_desc> sourcelist;

public:
    ~shadercache() { destroy(); }

    void destroy() {
        shaderlist.clear();
        sourcelist.clear();
    }

    bool update( shadercachexml* xmldata ) {
        xml_data = xmldata;
        if ( !xml_data ) {
            printf( "xml_data zero \n" );
            return false;
        }

        shaderlist.clear();
        sourcelist.clear();
        
        //
        if ( !std::filesystem::exists( std::filesystem::path( xmldata->xml_dirs.dir_sources ) ) ) {
            printf( "Not find source dir '%s'\n", xmldata->xml_dirs.dir_sources.c_str() );
            return false;
        }

        //
        bool crdir = true;
        if ( crdir && xmldata->xml_options.OptionsCompile_Hlsl ) 
            crdir = create_dir( xmldata->xml_dirs.dir_result_d3d11 );
        if ( crdir && xmldata->xml_options.OptionsCompile_Dxil ) 
            crdir = create_dir( xmldata->xml_dirs.dir_result_d3d12 );
        if ( crdir && xmldata->xml_options.OptionsCompile_SpirV ) 
            crdir = create_dir( xmldata->xml_dirs.dir_result_vk );
        if ( !crdir ) return false;

        //
        for ( auto& shader : xmldata->xml_shaders ) {
            // valid name string
            if ( !valid_name( shader.name ) ) { 
                printf( "shader '%s' name non valid! \n", shader.name.c_str() );
                shader.is_prev_bad_compile = true;
                shader.prev_error_message  = "name not valid";
                continue;
            }

            // valid name prev add
            for ( auto& shaderdesc : shaderlist ) {
                if ( shaderdesc.name == shader.name ) {
                    printf( "shader '%s' this name is used prev! \n", shader.name.c_str() );
                    shader.is_prev_bad_compile = true;
                    shader.prev_error_message  = "name re-use prev";
                    continue;
                }
            }

            // valid entry string
            if ( !valid_name( shader.entry ) ) {
                printf( "shader '%s' entry-name '%s' non valid! \n", shader.name.c_str(), shader.entry.c_str() );
                shader.is_prev_bad_compile = true;
                shader.prev_error_message  = "entry-name not valid";
                continue;
            }

            // valid hlsltarget
            if ( !valid_hlsltarget_to_type( shader.hlslversion ) ) {
                printf( "shader '%s' not valid hlslversion'%s' \n", shader.name.c_str(), shader.hlslversion.c_str() );
                shader.is_prev_bad_compile = true;
                shader.prev_error_message  = "hlslversion not valid";
                continue;
            }
            auto hlsl_type = hlsltarget_to_type( shader.hlslversion );
            auto version   = hlsltarget_to_version( shader.hlslversion );

            // valid file & namefile
            auto path = xmldata->xml_dirs.dir_sources + "/" + shader.namefile;
            if ( !std::filesystem::exists( std::filesystem::path( path ) ) ) {
                printf( "shader '%s' not find source '%s' \n", shader.name.c_str(), path.c_str() );
                shader.is_prev_bad_compile = true;
                shader.prev_error_message  = "shader source not find or not valid";
                continue;
            }

            // load source
            int source_idx = source_load( path );
            if ( source_idx < 0 ) {
                printf( "shader '%s' not open-read source '%s' \n", shader.name.c_str(), path.c_str() );
                shader.is_prev_bad_compile = true;
                shader.prev_error_message  = "shader source not open-read";
                continue;
            }

            // parse include recursive
            if ( !source_load_includes( source_idx ) ) {
                printf( "shader '%s' (warn) not parse #includes in '%s'\n", shader.name.c_str(), path.c_str() );
                continue;
            }

            //
            shader_desc temp_desc  = {};
            temp_desc.path         = path;
            temp_desc.name         = shader.name;
            temp_desc.namefile     = shader.namefile;
            temp_desc.hlslversion  = shader.hlslversion; 
            temp_desc.hlsl_type    = hlsl_type;
            temp_desc.hlsl_major   = version.first;
            temp_desc.hlsl_minor   = version.second;
            temp_desc.entry        = shader.entry;
            temp_desc.is_recompile = shader.is_force_recompile || shader.is_prev_bad_compile;  // рекомпиляция если предудущий с ошибкой но не текущий
            temp_desc.source_idx   = source_idx;
            temp_desc.xmlshader    = &shader;

            shaderlist.push_back( temp_desc );
        }

        // check prev cache
        for ( auto& cache : xmldata->xml_caches ) {
            //
            auto fsrcidx = source_find_frompath( cache.path );
            if ( fsrcidx < 0 ) {
                // cache not found in new sources -> delete record
                cache.is_delete = true; 
                continue;
            }

            bool isvalid = ( sourcelist[fsrcidx].filesize == cache.lastfilesize ) && ( sourcelist[fsrcidx].filetimewrite == cache.lastfiletimewrite ) && ( sourcelist[fsrcidx].datahash == cache.lastdatahash);

            sourcelist[fsrcidx].is_needupdate = !isvalid;
            if (sourcelist[fsrcidx].is_needupdate) {
                // update values
                cache.lastfilesize      = sourcelist[fsrcidx].filesize;
                cache.lastfiletimewrite = sourcelist[fsrcidx].filetimewrite;
                cache.lastdatahash      = sourcelist[fsrcidx].datahash;
            }
                 
            sourcelist[fsrcidx].is_in_cache = true;
        }

        // update new caches
        for ( auto& cache : sourcelist ) { 
            if ( cache.is_in_cache == false ) {
                cache.is_in_cache      = true;

                xmldesc_cache desc     = {};
                desc.path              = cache.path;
                desc.lastfilesize      = cache.filesize;
                desc.lastfiletimewrite = cache.filetimewrite;
                desc.lastdatahash      = cache.datahash;

                xmldata->xml_caches.push_back( desc );
            }
        }

        // check shader recompile flag
        for ( auto& shader : shaderlist ) { 
            // if not force recompile then check recompiled flags
            if ( shader.is_recompile == false ) shader.is_recompile = check_is_needupdate_rec( &sourcelist[shader.source_idx] );
            if ( shader.is_recompile ) { printf( "set recompile flag for shader '%s' \n", shader.name.c_str() ); }
        }

        return true;
    }

    void set_shader_good( shader_desc& shader ) {
        assert( shader.xmlshader );
        if ( !shader.xmlshader ) return;
        shader.xmlshader->is_prev_bad_compile = false;
    }

    void set_shader_bad_compile( shader_desc& shader, std::string message ) {
        assert( shader.xmlshader );
        if ( !shader.xmlshader ) return;
        shader.xmlshader->is_prev_bad_compile = true;
        shader.xmlshader->prev_error_message  = message;
    }

    void set_shader_message( shader_desc& shader, std::string message ) {
        assert( shader.xmlshader );
        if ( !shader.xmlshader ) return;
        shader.xmlshader->prev_error_message  = message;
    }

private: 
    bool check_is_needupdate_rec( source_desc* parentdesc ) {
        if ( parentdesc->is_needupdate ) return true;
        for ( auto idx : parentdesc->includes_idx ) {
            if ( check_is_needupdate_rec( &sourcelist[idx] ) ) return true;
        }
        return false;
    }

private: // tools
    bool create_dir( std::string& dir ) {
        std::filesystem::path path( dir );

        if ( !std::filesystem::exists( path ) ) {
            if ( !std::filesystem::create_directories( path ) ) {
                printf( "Not create output dir '%s'\n", dir.c_str() );
                return false;
            }
        }

        return true;
    }

    bool valid_name( std::string& name) {
        // todo
        return true;
    }

    bool valid_hlsltarget_to_type( std::string& hlslversion ) {
        bool isvalidstrt = false;
        for ( size_t i = 0; i < std::size( validstrtargets ) && !isvalidstrt; i++ ) isvalidstrt = ( hlslversion == validstrtargets[i] );
        return isvalidstrt;
    }

    shader_type hlsltarget_to_type( std::string& str ) {
        if ( str.find( "vs_" ) != std::string::npos ) return shader_type::vertex;
        if ( str.find( "ps_" ) != std::string::npos ) return shader_type::pixel;
        if ( str.find( "gs_" ) != std::string::npos ) return shader_type::geom;
        if ( str.find( "hs_" ) != std::string::npos ) return shader_type::hull;
        if ( str.find( "ds_" ) != std::string::npos ) return shader_type::domain;
        if ( str.find( "cs_" ) != std::string::npos ) return shader_type::compute;
        if ( str.find( "lib_" ) != std::string::npos ) return shader_type::lib;
        if ( str.find( "ms_" ) != std::string::npos ) return shader_type::mesh;
        if ( str.find( "as_" ) != std::string::npos ) return shader_type::as;
        return shader_type::none;
    }
    
    std::pair<uint8_t, uint8_t> hlsltarget_to_version( std::string& str ) {
        auto Offset1 = str.find( "_" );
        if ( Offset1 != str.npos ) {
            auto Offset2 = str.find( "_", Offset1 + 1 );
            if (Offset2 != str.npos) {
                std::string MajStr( str, Offset1 + 1, Offset2 - Offset1 - 1 );
                std::string MinStr( str, Offset2 + 1);
                uint8_t     major_ver = std::stoul( MajStr );
                uint8_t     minor_ver = std::stoul( MinStr );
    
                return { major_ver, minor_ver };
            }
        }
        return { 6, 0 };
    }
public:
    int source_find_frompath( std::string& path ) {
        for ( size_t i = 0; i < sourcelist.size(); i++ ) {
            if ( sourcelist[i].path == path ) return i;
        }
        return -1;
    }

private:
    int source_load( std::string& path ) {
        source_desc temp_desc = {};

        temp_desc.path = path;

        // find prev file add
        auto fsrcidx = source_find_frompath( path );
        if ( fsrcidx >= 0 ) { return fsrcidx; }

        std::ifstream filein( path, std::ios::binary );
        if ( !filein.is_open() ) return -1;

        // sizefile
        auto fsize = filein.tellg();
        filein.seekg( 0, std::ios::end );
        temp_desc.filesize = filein.tellg() - fsize;
        filein.seekg( 0, std::ios::beg );

        // time write
        const auto fileTime     = std::filesystem::last_write_time( temp_desc.path );
        const auto systemTime   = std::chrono::clock_cast<std::chrono::system_clock>( fileTime );
        temp_desc.filetimewrite = std::chrono::system_clock::to_time_t( systemTime );

        // read data
        temp_desc.data.resize( temp_desc.filesize );
        filein.read( temp_desc.data.data(), temp_desc.filesize );
        filein.close();

        // hash data
        temp_desc.datahash = std::hash<std::string>{}( temp_desc.data );

        // time write
        temp_desc.is_include = false;

        // push src
        sourcelist.push_back( temp_desc );

        // info
        printf( "add source '%s'\n", path.c_str() );

        return sourcelist.size() - 1;
    }

    bool source_load_includes( int basesource ) {
        if ( basesource < 0 ) return false;

        include_info* includes_list  = nullptr;
        int           includes_count = stb_include_find_includes( sourcelist[basesource].data.data(), &includes_list );

        for ( size_t i = 0; i < includes_count; i++ ) {
            std::string temppath = xml_data->xml_dirs.dir_sources + "/" + std::string( includes_list[i].filename );

            auto fsrcidx = source_find_frompath( temppath );
            if ( fsrcidx >= 0 ) {  // уже нашли в прошлый раз
                continue;
            }

            // load new
            fsrcidx = source_load( temppath );
            if ( fsrcidx < 0 ) {
                stb_include_free_includes( includes_list, includes_count );
                return false;
            }

            // push idx
            sourcelist[basesource].includes_idx.push_back( fsrcidx );

            // сохраняем что это инклуд
            sourcelist[fsrcidx].is_include = true;

            // парсим дочерние инклуды
            if ( !source_load_includes( fsrcidx ) ) {
                stb_include_free_includes( includes_list, includes_count );
                return false;
            }
        }

        stb_include_free_includes( includes_list, includes_count );

        return true;
    }

};