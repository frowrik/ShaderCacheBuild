#pragma once
using namespace tinyxml2;

struct xmldesc_dirs {
    std::string dir_sources         = "shaders_src";
    std::string dir_result_d3d11    = "shaders_bin/dx11";
    std::string dir_result_d3d12    = "shaders_bin/dx12";
    std::string dir_result_vk       = "shaders_bin/vk";
};

struct xmldesc_options {
    bool        enableDebugInfo              = true;
    bool        packMatricesInRowMajor       = false;
    bool        enable16bitTypes_hlsl_6_2    = false;
    int8_t      optimizationLevel            = -1;  // def disable, 0..3
    uint8_t     shiftAllCBuffersBindings_vk  = 0;
    uint8_t     shiftAllUABuffersBindings_vk = 0;
    uint8_t     shiftAllSamplersBindings_vk  = 0;
    uint8_t     shiftAllTexturesBindings_vk  = 0;
    bool        OptionsCompile_Dxil          = true;
    bool        OptionsCompile_SpirV         = true;
    bool        OptionsCompile_Hlsl          = true;
};

struct xmldesc_shader {
    std::string                                      name        = "";
    std::string                                      namefile    = "";
    std::string                                      entry       = "";
    std::string                                      hlslversion = "";
    std::vector<std::pair<std::string, std::string>> macros;
    std::string                                      prev_error_message  = "";
    bool                                             is_prev_bad_compile = false;  // в прошлый раз неудалось скомпилить попробовать сново
    bool                                             is_force_recompile  = false;  // при указании шейдер всегда будет рекомпилиться
};

struct xmldesc_cache {
    std::string path              = "";
    size_t      lastfilesize      = 0;
    std::time_t lastfiletimewrite = 0;
    size_t      lastdatahash      = 0;
    bool        is_delete         = false;
};

struct shadercachexml {
public:
    xmldesc_dirs                xml_dirs;
    xmldesc_options             xml_options;
    std::vector<xmldesc_shader> xml_shaders;
    std::vector<xmldesc_cache>  xml_caches;

public:
    shadercachexml() { destroy(); }
    ~shadercachexml() { destroy(); }

    void destroy() {
        xml_shaders.clear();
        xml_caches.clear();

        xml_dirs    = {};
        xml_options = {};
        xml_shaders = {};
        xml_caches  = {};
    }

    bool load( const char* filepath ) {
        destroy();

        tinyxml2::XMLDocument doc;
        auto docerr = doc.LoadFile( filepath );
        if ( docerr != XML_SUCCESS ) {
            printf( "Not open or parse xml file! '%s'\n", doc.ErrorStr() );
            doc.Clear();
            return false;
        }

        {

            //
            XMLElement* dirs_Element = doc.FirstChildElement( "dirs" );
            if ( !dirs_Element ) {
                dirs_Element = doc.NewElement( "dirs" );
                doc.InsertEndChild( dirs_Element );
            }

            {
                //
                xml_dirs.dir_sources         = "shaders_src";
                xml_dirs.dir_result_d3d11    = "shaders_bin/dx11";
                xml_dirs.dir_result_d3d12    = "shaders_bin/dx12";
                xml_dirs.dir_result_vk       = "shaders_bin/vk";

                //
                XMLElement* dirs_sources_Element = dirs_Element->FirstChildElement( "dir_sources" );
                if ( !dirs_sources_Element ) {
                    dirs_sources_Element = doc.NewElement( "dir_sources" );
                    dirs_sources_Element->SetText( xml_dirs.dir_sources.c_str() );
                    dirs_Element->InsertEndChild( dirs_sources_Element );
                } else {
                    xml_dirs.dir_sources = dirs_sources_Element->GetText();
                }

                //
                XMLElement* dirs_d3d11_Element = dirs_Element->FirstChildElement( "dir_d3d11" );
                if ( !dirs_d3d11_Element ) {
                    dirs_d3d11_Element = doc.NewElement( "dir_d3d11" );
                    dirs_d3d11_Element->SetText( xml_dirs.dir_result_d3d11.c_str() );
                    dirs_Element->InsertEndChild( dirs_d3d11_Element );
                } else {
                    xml_dirs.dir_result_d3d11 = dirs_d3d11_Element->GetText();
                }

                //
                XMLElement* dirs_d3d12_Element = dirs_Element->FirstChildElement( "dir_d3d12" );
                if ( !dirs_d3d12_Element ) {
                    dirs_d3d12_Element = doc.NewElement( "dir_d3d12" );
                    dirs_d3d12_Element->SetText( xml_dirs.dir_result_d3d12.c_str() );
                    dirs_Element->InsertEndChild( dirs_d3d12_Element );
                } else {
                    xml_dirs.dir_result_d3d12 = dirs_d3d12_Element->GetText();
                }

                //
                XMLElement* dirs_vk_Element = dirs_Element->FirstChildElement( "dir_vk" );
                if ( !dirs_vk_Element ) {
                    dirs_vk_Element = doc.NewElement( "dir_vk" );
                    dirs_vk_Element->SetText( xml_dirs.dir_result_vk.c_str() );
                    dirs_Element->InsertEndChild( dirs_vk_Element );
                } else {
                    xml_dirs.dir_result_vk = dirs_vk_Element->GetText();
                }
            }

            //
            XMLElement* options_Element = doc.FirstChildElement( "options" );
            if ( !options_Element ) {
                options_Element = doc.NewElement( "options" );
                doc.InsertEndChild( options_Element );
            }

            {
                {
                    XMLElement* _Element = options_Element->FirstChildElement( "enableDebugInfo" );
                    if ( !_Element ) {
                        _Element = doc.NewElement( "enableDebugInfo" );
                        _Element->SetText( xml_options.enableDebugInfo );
                        options_Element->InsertEndChild( _Element );
                    } else {
                        _Element->QueryBoolText( &xml_options.enableDebugInfo );
                    }
                }
                {
                    XMLElement* _Element = options_Element->FirstChildElement( "packMatricesInRowMajor" );
                    if ( !_Element ) {
                        _Element = doc.NewElement( "packMatricesInRowMajor" );
                        _Element->SetText( xml_options.packMatricesInRowMajor );
                        options_Element->InsertEndChild( _Element );
                    } else {
                        _Element->QueryBoolText( &xml_options.packMatricesInRowMajor );
                    }
                }
                {
                    XMLElement* _Element = options_Element->FirstChildElement( "enable16bitTypes_hlsl_6_2" );
                    if ( !_Element ) {
                        _Element = doc.NewElement( "enable16bitTypes_hlsl_6_2" );
                        _Element->SetText( xml_options.enable16bitTypes_hlsl_6_2 );
                        options_Element->InsertEndChild( _Element );
                    } else {
                        _Element->QueryBoolText( &xml_options.enable16bitTypes_hlsl_6_2 );
                    }
                }
                {
                    XMLElement* _Element = options_Element->FirstChildElement( "OptionsCompile_Dxil" );
                    if ( !_Element ) {
                        _Element = doc.NewElement( "OptionsCompile_Dxil" );
                        _Element->SetText( xml_options.OptionsCompile_Dxil );
                        options_Element->InsertEndChild( _Element );
                    } else {
                        _Element->QueryBoolText( &xml_options.OptionsCompile_Dxil );
                    }
                }
                {
                    XMLElement* _Element = options_Element->FirstChildElement( "OptionsCompile_SpirV" );
                    if ( !_Element ) {
                        _Element = doc.NewElement( "OptionsCompile_SpirV" );
                        _Element->SetText( xml_options.OptionsCompile_SpirV );
                        options_Element->InsertEndChild( _Element );
                    } else {
                        _Element->QueryBoolText( &xml_options.OptionsCompile_SpirV );
                    }
                }
                {
                    XMLElement* _Element = options_Element->FirstChildElement( "OptionsCompile_Hlsl" );
                    if ( !_Element ) {
                        _Element = doc.NewElement( "OptionsCompile_Hlsl" );
                        _Element->SetText( xml_options.OptionsCompile_Hlsl );
                        options_Element->InsertEndChild( _Element );
                    } else {
                        _Element->QueryBoolText( &xml_options.OptionsCompile_Hlsl );
                    }
                }
                {
                    XMLElement* _Element = options_Element->FirstChildElement( "optimizationLevel" );
                    if ( !_Element ) {
                        _Element = doc.NewElement( "optimizationLevel" );
                        _Element->SetText( xml_options.optimizationLevel );
                        options_Element->InsertEndChild( _Element );
                    } else {
                        int temp;
                        _Element->QueryIntText( &temp );
                        xml_options.optimizationLevel = temp;
                    }
                }
                {
                    XMLElement* _Element = options_Element->FirstChildElement( "shiftAllCBuffersBindings_vk" );
                    if ( !_Element ) {
                        _Element = doc.NewElement( "shiftAllCBuffersBindings_vk" );
                        _Element->SetText( xml_options.shiftAllCBuffersBindings_vk );
                        options_Element->InsertEndChild( _Element );
                    } else {
                        unsigned int temp;
                        _Element->QueryUnsignedText( &temp );
                        xml_options.shiftAllCBuffersBindings_vk = temp;
                    }
                }
                {
                    XMLElement* _Element = options_Element->FirstChildElement( "shiftAllUABuffersBindings_vk" );
                    if ( !_Element ) {
                        _Element = doc.NewElement( "shiftAllUABuffersBindings_vk" );
                        _Element->SetText( xml_options.shiftAllUABuffersBindings_vk );
                        options_Element->InsertEndChild( _Element );
                    } else {
                        unsigned int temp;
                        _Element->QueryUnsignedText( &temp );
                        xml_options.shiftAllUABuffersBindings_vk = temp;
                    }
                }
                {
                    XMLElement* _Element = options_Element->FirstChildElement( "shiftAllSamplersBindings_vk" );
                    if ( !_Element ) {
                        _Element = doc.NewElement( "shiftAllSamplersBindings_vk" );
                        _Element->SetText( xml_options.shiftAllSamplersBindings_vk );
                        options_Element->InsertEndChild( _Element );
                    } else {
                        unsigned int temp;
                        _Element->QueryUnsignedText( &temp );
                        xml_options.shiftAllSamplersBindings_vk = temp;
                    }
                }
                {
                    XMLElement* _Element = options_Element->FirstChildElement( "shiftAllTexturesBindings_vk" );
                    if ( !_Element ) {
                        _Element = doc.NewElement( "shiftAllTexturesBindings_vk" );
                        _Element->SetText( xml_options.shiftAllTexturesBindings_vk );
                        options_Element->InsertEndChild( _Element );
                    } else {
                        unsigned int temp;
                        _Element->QueryUnsignedText( &temp );
                        xml_options.shiftAllTexturesBindings_vk = temp;
                    }
                }
            }

            //
            XMLElement* shaders_Element = doc.FirstChildElement( "shaderlist" );
            if ( !shaders_Element ) {
                shaders_Element = doc.NewElement( "shaderlist" );
                doc.InsertEndChild( shaders_Element );
            }

            for ( XMLElement* Arr_Element = shaders_Element->FirstChildElement(); Arr_Element; Arr_Element = Arr_Element->NextSiblingElement() ) {
                xmldesc_shader temp_desc = {};

                //
                if ( std::strcmp( Arr_Element->Name(), "shader" ) != 0 ) {
                    printf( "shader list invalid desc\n" );
                    continue;  // skip non shader desc
                }

                //
                XMLElement* name_Element = Arr_Element->FirstChildElement( "name" );
                if ( !name_Element ) {
                    printf( "shader list not found 'name'\n" );
                    continue;
                }

                temp_desc.name = name_Element->GetText();

                //
                XMLElement* namefile_Element = Arr_Element->FirstChildElement( "namefile" );
                if ( !namefile_Element ) {
                    printf( "shader list not found 'namefile'\n" );
                    continue;
                }

                temp_desc.namefile = namefile_Element->GetText();

                //
                XMLElement* entry_Element = Arr_Element->FirstChildElement( "entry" );
                if ( !entry_Element ) {
                    printf( "shader list not found 'entry'\n" );
                    continue;
                }

                temp_desc.entry = entry_Element->GetText();

                //
                XMLElement* version_Element = Arr_Element->FirstChildElement( "hlslversion" );
                if ( !version_Element ) {
                    printf( "shader list not found 'hlslversion'\n" );
                    continue;
                }

                temp_desc.hlslversion = version_Element->GetText();

                //
                XMLElement* defines_Element = Arr_Element->FirstChildElement( "defines" );
                if (defines_Element) {
                    for (XMLElement* Arr_Element = defines_Element->FirstChildElement(); Arr_Element; Arr_Element = Arr_Element->NextSiblingElement()) {
                        if (std::strcmp(Arr_Element->Name(), "define") != 0) {
                            printf("shader list invalid desc\n");
                            continue;  // skip non shader desc
                        }

                        auto name_Attribute = Arr_Element->FindAttribute("name");
                        if (!name_Attribute) {
                            printf("shader list not found 'name'\n");
                            continue;
                        }

                        auto value_Attribute = Arr_Element->FindAttribute("value");

                        temp_desc.macros.push_back({ std::string(name_Attribute->Value()), std::string(value_Attribute ? value_Attribute->Value() : "") });
                    }
                }

                //
                XMLElement* is_prev_bad_compile_Element = Arr_Element->FirstChildElement( "is_prev_bad_compile" );
                if ( is_prev_bad_compile_Element ) is_prev_bad_compile_Element->QueryBoolText( &temp_desc.is_prev_bad_compile );
                else
                    temp_desc.is_prev_bad_compile = true;

                //
                XMLElement* is_force_recompile_Element = Arr_Element->FirstChildElement( "is_force_recompile" );
                if ( is_force_recompile_Element ) is_force_recompile_Element->QueryBoolText( &temp_desc.is_force_recompile );
                else
                    temp_desc.is_prev_bad_compile = true; // !

                //
                xml_shaders.push_back( temp_desc );
            }

            //
            XMLElement* cache_Element = doc.FirstChildElement( "cache" );
            if ( !cache_Element ) {
                cache_Element = doc.NewElement( "cache" );
                doc.InsertEndChild( cache_Element );
            }

            for ( XMLElement* Arr_Element = cache_Element->FirstChildElement(); Arr_Element; Arr_Element = Arr_Element->NextSiblingElement() ) {
                xmldesc_cache temp_desc = {};

                //
                if ( std::strcmp( Arr_Element->Name(), "source" ) != 0 ) {
                    printf( "cache list invalid desc\n" );
                    continue;
                }

                //
                XMLElement* path_Element = Arr_Element->FirstChildElement( "path" );
                if ( !path_Element ) {
                    printf( "cache list not found 'path'\n" );
                    continue;
                }

                temp_desc.path = path_Element->GetText();

                //
                XMLElement* lastfilesize_Element = Arr_Element->FirstChildElement( "lastfilesize" );
                if ( !lastfilesize_Element ) {
                    printf( "cache list not found 'lastfilesize'\n" );
                    continue;
                }

                if ( lastfilesize_Element->QueryUnsigned64Text( &temp_desc.lastfilesize ) != XML_SUCCESS ) {
                    printf( "non valid value 'lastfilesize'\n" );
                    continue;
                }

                //
                XMLElement* lastfiletimewrite_Element = Arr_Element->FirstChildElement( "lastfiletimewrite" );
                if ( !lastfiletimewrite_Element ) {
                    printf( "cache list not found 'lastfiletimewrite'\n" );
                    continue;
                }

                uint64_t lastfiletimewrite = 0;
                if ( lastfiletimewrite_Element->QueryUnsigned64Text( &lastfiletimewrite ) != XML_SUCCESS ) {
                    printf( "non valid value 'lastfilesize'\n" );
                    continue;
                }

                temp_desc.lastfiletimewrite = lastfiletimewrite;

                //
                XMLElement* lastdatahash_Element = Arr_Element->FirstChildElement( "lastdatahash" );
                if ( !lastdatahash_Element ) {
                    printf( "cache list not found 'lastdatahash'\n" );
                    continue;
                }

                if ( lastdatahash_Element->QueryUnsigned64Text( &temp_desc.lastdatahash ) != XML_SUCCESS ) {
                    printf( "non valid value 'lastfilesize'\n" );
                    continue;
                }

                //
                xml_caches.push_back( temp_desc );
            }
        }

        return true;
    }

    bool save( const char* filepath ) {
        tinyxml2::XMLDocument doc;

        {

            //
            XMLElement* dirs_Element = doc.NewElement( "dirs" );
            doc.InsertEndChild( dirs_Element );
            
            {
                //
                XMLElement*  dirs_sources_Element = doc.NewElement( "dir_sources" );
                dirs_sources_Element->SetText( xml_dirs.dir_sources.c_str() );
                dirs_Element->InsertEndChild( dirs_sources_Element );
                
                //
                XMLElement* dirs_d3d11_Element = doc.NewElement( "dir_d3d11" );
                dirs_d3d11_Element->SetText( xml_dirs.dir_result_d3d11.c_str() );
                dirs_Element->InsertEndChild( dirs_d3d11_Element );
                
                //
                XMLElement* dirs_d3d12_Element = doc.NewElement( "dir_d3d12" );
                dirs_d3d12_Element->SetText( xml_dirs.dir_result_d3d12.c_str() );
                dirs_Element->InsertEndChild( dirs_d3d12_Element );
                
                //
                XMLElement* dirs_vk_Element = doc.NewElement( "dir_vk" );
                dirs_vk_Element->SetText( xml_dirs.dir_result_vk.c_str() );
                dirs_Element->InsertEndChild( dirs_vk_Element );
                
            }

            //
            XMLElement* options_Element = doc.NewElement( "options" );
            doc.InsertEndChild( options_Element );
            
            {
                XMLElement* _Element;
                
                _Element = doc.NewElement( "enableDebugInfo" );
                _Element->SetText( xml_options.enableDebugInfo );
                options_Element->InsertEndChild( _Element );
                   
                _Element = doc.NewElement( "packMatricesInRowMajor" );
                _Element->SetText( xml_options.packMatricesInRowMajor );
                options_Element->InsertEndChild( _Element );
                
                _Element = doc.NewElement( "enable16bitTypes_hlsl_6_2" );
                _Element->SetText( xml_options.enable16bitTypes_hlsl_6_2 );
                options_Element->InsertEndChild( _Element );
               
                _Element = doc.NewElement( "OptionsCompile_Dxil" );
                _Element->SetText( xml_options.OptionsCompile_Dxil );
                options_Element->InsertEndChild( _Element );
                
                _Element = doc.NewElement( "OptionsCompile_SpirV" );
                _Element->SetText( xml_options.OptionsCompile_SpirV );
                options_Element->InsertEndChild( _Element );
                   
                _Element = doc.NewElement( "OptionsCompile_Hlsl" );
                _Element->SetText( xml_options.OptionsCompile_Hlsl );
                options_Element->InsertEndChild( _Element );
                  
                _Element = doc.NewElement( "optimizationLevel" );
                _Element->SetText( xml_options.optimizationLevel );
                options_Element->InsertEndChild( _Element );
              
                _Element = doc.NewElement( "shiftAllCBuffersBindings_vk" );
                _Element->SetText( xml_options.shiftAllCBuffersBindings_vk );
                options_Element->InsertEndChild( _Element );
                  
                _Element = doc.NewElement( "shiftAllUABuffersBindings_vk" );
                _Element->SetText( xml_options.shiftAllUABuffersBindings_vk );
                options_Element->InsertEndChild( _Element );
                   
                _Element = doc.NewElement( "shiftAllSamplersBindings_vk" );
                _Element->SetText( xml_options.shiftAllSamplersBindings_vk );
                options_Element->InsertEndChild( _Element );
                    
                _Element = doc.NewElement( "shiftAllTexturesBindings_vk" );
                _Element->SetText( xml_options.shiftAllTexturesBindings_vk );
                options_Element->InsertEndChild( _Element );
            }

            //
            XMLElement* shaders_Element = doc.NewElement( "shaderlist" );
            doc.InsertEndChild( shaders_Element );
            
            for ( auto& shader : xml_shaders ) {
                //
                XMLElement* Arr_Element = doc.NewElement( "shader" );
                shaders_Element->InsertEndChild( Arr_Element );

                //
                XMLElement* name_Element = doc.NewElement( "name" );
                name_Element->SetText( shader.name.c_str() );
                Arr_Element->InsertEndChild( name_Element );

                //
                XMLElement* namefile_Element = doc.NewElement( "namefile" );
                namefile_Element->SetText( shader.namefile.c_str() );
                Arr_Element->InsertEndChild( namefile_Element );

                //
                XMLElement* entry_Element = doc.NewElement( "entry" );
                entry_Element->SetText( shader.entry.c_str() );
                Arr_Element->InsertEndChild( entry_Element );

                //
                XMLElement* version_Element = doc.NewElement( "hlslversion" );
                version_Element->SetText( shader.hlslversion.c_str() );
                Arr_Element->InsertEndChild( version_Element );
                
                //
                XMLElement* defines_Element = doc.NewElement( "defines" );
                for ( auto& def : shader.macros ) {
                    //
                    XMLElement* Arr_Element = doc.NewElement( "define" );
                    defines_Element->InsertEndChild( Arr_Element );

                    //
                    Arr_Element->SetAttribute( "name", def.first.c_str() );

                    //
                    if ( !def.second.empty() ) Arr_Element->SetAttribute( "value", def.second.c_str() );
                }
                Arr_Element->InsertEndChild( defines_Element );

                
                //
                XMLElement* is_prev_bad_compile_Element = doc.NewElement( "is_prev_bad_compile" );
                is_prev_bad_compile_Element->SetText( shader.is_prev_bad_compile );
                Arr_Element->InsertEndChild( is_prev_bad_compile_Element );

                //
                XMLElement* is_force_recompile_Element = doc.NewElement( "is_force_recompile" );
                is_force_recompile_Element->SetText( shader.is_force_recompile );
                Arr_Element->InsertEndChild( is_force_recompile_Element );

                //
                if ( shader.prev_error_message.size() > 0) {
                    XMLElement* message_Element = doc.NewElement( "prev_error_message" );
                    message_Element->SetText( shader.prev_error_message.c_str() );
                    Arr_Element->InsertEndChild( message_Element );
                }
            }

            //
            XMLElement* cache_Element = doc.NewElement( "cache" );
            doc.InsertEndChild( cache_Element );
            
            for ( auto& cache : xml_caches ) {
                if ( cache.is_delete ) continue; // not add removed cache

                //
                XMLElement* Arr_Element = doc.NewElement( "source" );
                cache_Element->InsertEndChild( Arr_Element );

                //
                XMLElement* path_Element = doc.NewElement( "path" );
                path_Element->SetText( cache.path.c_str() );
                Arr_Element->InsertEndChild( path_Element );

                //
                XMLElement* lastfilesize_Element = doc.NewElement( "lastfilesize" );
                lastfilesize_Element->SetText( cache.lastfilesize );
                Arr_Element->InsertEndChild( lastfilesize_Element );

                //
                XMLElement* lastfiletimewrite_Element = doc.NewElement( "lastfiletimewrite" );
                lastfiletimewrite_Element->SetText( cache.lastfiletimewrite );
                Arr_Element->InsertEndChild( lastfiletimewrite_Element );

                //
                XMLElement* lastdatahash_Element = doc.NewElement( "lastdatahash" );
                lastdatahash_Element->SetText( cache.lastdatahash );
                Arr_Element->InsertEndChild( lastdatahash_Element );

            }
        }

        auto docerr = doc.SaveFile( filepath );
        if ( docerr != XML_SUCCESS ) {
            printf( "Not save xml file!\n", doc.ErrorStr() );
            return false;
        }

        return true;
    }

};
