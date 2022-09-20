//
#include <cassert>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <codecvt>

//
template<typename... Args>
inline std::string string_format( const char* format, Args... args ) {
    const auto _Len = static_cast<size_t>( std::snprintf( nullptr, 0, format, args... ) ) + 1;
    assert( _Len >= 0 );
    if ( _Len < 0 ) return "";
    std::string _Str;
    _Str.resize( _Len );
    std::snprintf( _Str.data(), _Str.length(), format, args... );
    _Str.resize( _Len - 1 );  // remove 0
    return _Str;
}

std::string wstring_to_utf8( std::wstring_view source ) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.to_bytes( source._Unchecked_begin(), source._Unchecked_end() );
}

std::wstring utf8_to_wstring( std::string_view source ) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.from_bytes( source._Unchecked_begin(), source._Unchecked_end() );
}

//
#include <windows.h>
//#include <dxc/DxilContainer/DxilContainer.h>
//#include <dxc/dxcapi.h>
#include <dxcapi.h>

//
#include <d3d11.h>
#include <d3dcompiler.h>

//
#include "tinyxml2.h"

//
#define STB_INCLUDE_IMPLEMENTATION
#include "stb_include.h"

//
#include "shadercachexml_imp.h"
#include "shadercache_imp.h"

//
shadercachexml           cachexml;
shadercache              cache;

HMODULE                  dxcompiler_dll                = nullptr;
DxcCreateInstanceProc    dxcompiler_createInstanceFunc = nullptr;
IDxcLibrary*             dxc_library                   = nullptr;
IDxcCompiler*            dxc_compiler                  = nullptr;
IDxcContainerReflection* dxc_reflection                = nullptr;
IDxcLinker*              dxc_linker                    = nullptr;

class ScIncludeHandler : public IDxcIncludeHandler {
public:
    HRESULT STDMETHODCALLTYPE LoadSource( LPCWSTR fileName, IDxcBlob** includeSource ) override {
        if ( ( fileName[0] == L'.' ) && ( fileName[1] == L'/' ) ) { fileName += 2; }

        std::string includeName = wstring_to_utf8( fileName );

        std::string temppath = cachexml.xml_dirs.dir_sources + "/" + std::string( includeName );
        auto fsrcidx = cache.source_find_frompath( temppath );
        if ( fsrcidx < 0 ) return E_FAIL;

        std::string& data = cache.sourcelist[fsrcidx].data;

        *includeSource = nullptr;
        return dxc_library->CreateBlobWithEncodingOnHeapCopy( data.data(), data.size(), CP_UTF8, reinterpret_cast<IDxcBlobEncoding**>( includeSource ) );
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        ++m_ref;
        return m_ref;
    }

    ULONG STDMETHODCALLTYPE Release() override {
        --m_ref;
        ULONG result = m_ref;
        if ( result == 0 ) { delete this; }
        return result;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** object ) override {
        if ( IsEqualIID( iid, __uuidof( IDxcIncludeHandler ) ) ) {
            *object = dynamic_cast<IDxcIncludeHandler*>( this );
            this->AddRef();
            return S_OK;
        } else if ( IsEqualIID( iid, __uuidof( IUnknown ) ) ) {
            *object = dynamic_cast<IUnknown*>( this );
            this->AddRef();
            return S_OK;
        } else {
            return E_NOINTERFACE;
        }
    }

private:
    std::atomic<ULONG> m_ref = 0;
};

ScIncludeHandler dxc_includer;

struct IncludeCallback : public ID3DInclude {
    virtual HRESULT __stdcall Open( D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes ) {

        std::string temppath = cachexml.xml_dirs.dir_sources + "/" + std::string( pFileName );
        auto        fsrcidx  = cache.source_find_frompath( temppath );
        if ( fsrcidx < 0 ) return E_FAIL;

        std::string& data = cache.sourcelist[fsrcidx].data;


        *ppData = data.data();
        *pBytes = data.size();

        return S_OK;
    }

    virtual HRESULT __stdcall Close( LPCVOID pData ) { return S_OK; }
};

IncludeCallback dxbc_includer;


void compiler_free() {
    if ( dxc_linker ) {
        dxc_linker->Release();
        dxc_linker = nullptr;
    }
    if ( dxc_reflection ) {
        dxc_reflection->Release();
        dxc_reflection = nullptr;
    }
    if ( dxc_compiler ) {
        dxc_compiler->Release();
        dxc_compiler = nullptr;
    }
    if ( dxc_library ) {
        dxc_library->Release();
        dxc_library = nullptr;
    }
    if (dxcompiler_dll){ 
        ::FreeLibrary( dxcompiler_dll );
        dxcompiler_dll                = nullptr;
        dxcompiler_createInstanceFunc = nullptr;
    }  
}

bool compiler_init() {
    dxcompiler_dll = ::LoadLibraryA( "dxcompiler.dll" );
    if ( !dxcompiler_dll ) {
        printf( "dxcompiler.dll not found \n" );
        return false;
    }

    dxcompiler_createInstanceFunc = ( DxcCreateInstanceProc )::GetProcAddress( dxcompiler_dll, "DxcCreateInstance" );
    if ( !dxcompiler_dll ) {
        printf( "DxcCreateInstance not create\n" );
        return false;
    }

    auto hr = dxcompiler_createInstanceFunc( CLSID_DxcLibrary, __uuidof( IDxcLibrary ), reinterpret_cast<void**>( &dxc_library ) );
    if ( FAILED( hr ) ) {
        printf( "dxc_library not create\n" );
        return false;
    }

    hr = dxcompiler_createInstanceFunc( CLSID_DxcCompiler, __uuidof( IDxcCompiler ), reinterpret_cast<void**>( &dxc_compiler ) );
    if ( FAILED( hr ) ) {
        printf( "dxc_compiler not create\n" );
        return false;
    }

    hr = dxcompiler_createInstanceFunc( CLSID_DxcContainerReflection, __uuidof( IDxcContainerReflection ), reinterpret_cast<void**>( &dxc_reflection ) );
    if ( FAILED( hr ) ) {
        printf( "dxc_reflection not create\n" );
        return false;
    }

    hr = dxcompiler_createInstanceFunc( CLSID_DxcLinker, __uuidof( IDxcLinker ), reinterpret_cast<void**>( &dxc_linker ) );
    if ( FAILED( hr ) ) {
        printf( "dxc_linker not create \n" );
        return false;
    }

    dxc_includer.AddRef();
    
    return true;
}

bool compiler_process_dxbc( shader_desc& shader, source_desc& source ) { 
    if ( (uint8_t)shader.hlsl_type > (uint8_t)shader_type::compute ) return true;
    
    ID3DBlob* ErrorMsgs = nullptr;
    ID3DBlob* Blob_out  = nullptr;

    std::vector<D3D_SHADER_MACRO> Defines;
    for ( auto& def : shader.xmlshader->macros ) { Defines.push_back( { def.first.c_str(), def.second.c_str() } ); }
    Defines.push_back( { nullptr, nullptr } );

    UINT Flags = 0;

    if ( cachexml.xml_options.enableDebugInfo ) Flags |= D3DCOMPILE_DEBUG;

    if ( cachexml.xml_options.packMatricesInRowMajor ) Flags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
    else
        Flags |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;

    if ( cachexml.xml_options.optimizationLevel == 0 ) Flags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
    if ( cachexml.xml_options.optimizationLevel == 1 ) Flags |= D3DCOMPILE_OPTIMIZATION_LEVEL1;
    if ( cachexml.xml_options.optimizationLevel == 2 ) Flags |= D3DCOMPILE_OPTIMIZATION_LEVEL2;
    if ( cachexml.xml_options.optimizationLevel == 3 ) Flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
    if ( cachexml.xml_options.optimizationLevel > 4 || cachexml.xml_options.optimizationLevel < 0 ) Flags |= D3DCOMPILE_SKIP_OPTIMIZATION;

    Flags |= D3DCOMPILE_ENABLE_STRICTNESS;

    std::string TargetStr = string_format( "%s_%d_%d", shader_type_name[(uint8_t)shader.hlsl_type], shader.hlsl_major, shader.hlsl_minor );

    auto hr = D3DCompile( (LPCSTR)source.data.data(), source.data.size(), shader.namefile.c_str(), Defines.data(), &dxbc_includer, shader.entry.c_str(), TargetStr.c_str(), Flags, 0, &Blob_out, &ErrorMsgs );
    if ( FAILED( hr ) ) {
        std::string temp_err( (const char*)ErrorMsgs->GetBufferPointer(), ErrorMsgs->GetBufferSize() );
        cache.set_shader_bad_compile( shader, temp_err );
        printf( "dxbc_compiler not create \n" );
        ErrorMsgs->Release();
        ErrorMsgs = nullptr;

        return false;
    }

    if ( ErrorMsgs ) {
        std::string temp_err( (const char*)ErrorMsgs->GetBufferPointer(), ErrorMsgs->GetBufferSize() );
        cache.set_shader_message( shader, temp_err );
        printf( "dxbc_compiler message '%s'\n", temp_err.c_str() );
        ErrorMsgs->Release();
        ErrorMsgs = nullptr;
    }

    if ( Blob_out != nullptr ) {
        std::string   outpath = cachexml.xml_dirs.dir_result_d3d11 + "/" + shader.name;
        std::ofstream filein( outpath, std::ios::binary );
        if ( filein.is_open() ) {
            filein.write( (const char*)Blob_out->GetBufferPointer(), static_cast<uint32_t>( Blob_out->GetBufferSize() ) );
            filein.close();

            printf( "ShaderConductor::Compiler::Dxil '%s'\n", outpath.c_str() );
        }
        Blob_out->Release();
        Blob_out = nullptr;
    }

    return true; 
}


void compiler_process( shader_desc& shader ) {
    if ( shader.is_recompile == false ) return;

    auto&             source_main   = cache.sourcelist[shader.source_idx];

    // dxbc compile
    if ( cachexml.xml_options.OptionsCompile_Hlsl && shader.hlsl_major < 6 ) { 
        if ( !compiler_process_dxbc( shader, source_main ) ) return;
    }

    //  create main blob
    IDxcBlobEncoding* includeSource = nullptr;

    auto hr = dxc_library->CreateBlobWithEncodingOnHeapCopy( source_main.data.data(), source_main.data.size(), CP_UTF8, reinterpret_cast<IDxcBlobEncoding**>( &includeSource ) );
    
    if ( FAILED( hr ) || !includeSource ) {
        cache.set_shader_bad_compile( shader, "compiler internal error 1" );
        printf( "dxc_library blob not create \n" );
        return;
    }

    std::wstring SourceName    = utf8_to_wstring( shader.namefile );
    std::wstring EntryPoint    = utf8_to_wstring( shader.entry );
    std::string  TargetStr     = string_format( "%s_%d_%d", shader_type_name[(uint8_t)shader.hlsl_type], shader.hlsl_major, shader.hlsl_minor );
    if ( shader.hlsl_major < 6 ) 
        TargetStr = string_format( "%s_%d_%d", shader_type_name[(uint8_t)shader.hlsl_type], 6, 0 );

    std::wstring TargetProfile = utf8_to_wstring( TargetStr );

    // defines
    std::vector<std::wstring> DefinesStrings;
    std::vector<DxcDefine>    Defines;

    DefinesStrings.reserve( shader.xmlshader->macros.size() * 2 );

    for ( auto& def : shader.xmlshader->macros ) { 
        std::wstring nameUtf16Str  = utf8_to_wstring( def.first );
        std::wstring valueUtf16Str = utf8_to_wstring( def.second );

        DefinesStrings.emplace_back( std::move( nameUtf16Str ) );
        const wchar_t* nameUtf16 = DefinesStrings.back().c_str();

        const wchar_t* valueUtf16 = nullptr;
        if ( !valueUtf16Str.empty() ) {
            DefinesStrings.emplace_back( std::move( valueUtf16Str ) );
            valueUtf16 = DefinesStrings.back().c_str();
        }

        Defines.push_back( { nameUtf16, valueUtf16 } );
    }

    // arguments
    std::vector<std::wstring>   ArgumentsStrings;

    ArgumentsStrings.push_back( cachexml.xml_options.packMatricesInRowMajor ? L"-Zpr" : L"-Zpc" ); 

    if ( cachexml.xml_options.enable16bitTypes_hlsl_6_2 && shader.hlsl_major >= 6 && shader.hlsl_minor >= 2 ) ArgumentsStrings.push_back( L"-enable-16bit-types" );
    
    if ( cachexml.xml_options.enableDebugInfo ) {
        ArgumentsStrings.push_back( L"-Zi" );
        ArgumentsStrings.push_back( L"-Qembed_debug" );
    }

    if ( cachexml.xml_options.optimizationLevel < 4 && cachexml.xml_options.optimizationLevel >= 0 ) {
        ArgumentsStrings.push_back( std::wstring( L"-O" ) + static_cast<wchar_t>( L'0' + cachexml.xml_options.optimizationLevel ) );
    } else  {
        ArgumentsStrings.push_back( L"-Od" );
    }

    if ( cachexml.xml_options.shiftAllCBuffersBindings_vk > 0 ) {
        ArgumentsStrings.push_back( L"-fvk-b-shift" );
        ArgumentsStrings.push_back( std::to_wstring( cachexml.xml_options.shiftAllCBuffersBindings_vk ) );
        ArgumentsStrings.push_back( L"all" );
    }

    if ( cachexml.xml_options.shiftAllUABuffersBindings_vk > 0 ) {
        ArgumentsStrings.push_back( L"-fvk-u-shift" );
        ArgumentsStrings.push_back( std::to_wstring( cachexml.xml_options.shiftAllUABuffersBindings_vk ) );
        ArgumentsStrings.push_back( L"all" );
    }

    if ( cachexml.xml_options.shiftAllSamplersBindings_vk > 0 ) {
        ArgumentsStrings.push_back( L"-fvk-s-shift" );
        ArgumentsStrings.push_back( std::to_wstring( cachexml.xml_options.shiftAllSamplersBindings_vk ) );
        ArgumentsStrings.push_back( L"all" );
    }

    if ( cachexml.xml_options.shiftAllTexturesBindings_vk > 0 ) {
        ArgumentsStrings.push_back( L"-fvk-t-shift" );
        ArgumentsStrings.push_back( std::to_wstring( cachexml.xml_options.shiftAllTexturesBindings_vk ) );
        ArgumentsStrings.push_back( L"all" );
    }

    ArgumentsStrings.push_back( L"-spirv" );

    std::vector<const wchar_t*> Arguments;
    Arguments.reserve( ArgumentsStrings.size() );
    for ( const auto& arg : ArgumentsStrings ) Arguments.push_back( arg.c_str() );

    // dxil complele
    if ( cachexml.xml_options.OptionsCompile_Dxil) {
        IDxcOperationResult* Result = nullptr;

        hr = dxc_compiler->Compile( includeSource, SourceName.c_str(), EntryPoint.c_str(), TargetProfile.c_str(), Arguments.data(), Arguments.size() - 1, Defines.data(), Defines.size(), &dxc_includer, &Result );
        if ( FAILED( hr ) ) {
            IDxcBlobEncoding* errors = nullptr;
            if ( SUCCEEDED( Result->GetErrorBuffer( &errors ) ) && errors != nullptr ) {
                std::string temp_err( (const char*)errors->GetBufferPointer(), errors->GetBufferSize() );
                cache.set_shader_message( shader, temp_err );
                printf( "dxc_compiler error '%s'\n", temp_err.c_str() );

                errors->Release();
                errors = nullptr;
            } else {
                cache.set_shader_bad_compile( shader, "compile error" );
            }

            printf( "dxc_compiler not create \n" );

            includeSource->Release();
            includeSource = nullptr;

            return;
        }

        //
        IDxcBlobEncoding* errors = nullptr;
        if ( SUCCEEDED( Result->GetErrorBuffer( &errors ) ) && errors != nullptr ) {
            std::string temp_err( (const char*)errors->GetBufferPointer(), errors->GetBufferSize() );
            
            if ( !temp_err.empty() ) {
                cache.set_shader_message( shader, temp_err );
                printf( "dxc_compiler message '%s'\n", temp_err.c_str() );
            }

            errors->Release();
            errors = nullptr;
        }

        //
        HRESULT status;
        hr = Result->GetStatus( &status );
        if ( SUCCEEDED( hr ) && SUCCEEDED(status) ) {
            //
            IDxcBlob* program = nullptr;
            hr                = Result->GetResult( &program );

            //
            Result->Release();
            Result = nullptr;

            //
            if ( program != nullptr ) {
                std::string   outpath = cachexml.xml_dirs.dir_result_d3d12 + "/" + shader.name;
                std::ofstream filein( outpath, std::ios::binary );
                if ( filein.is_open() ) {
                    filein.write( (const char*)program->GetBufferPointer(), static_cast<uint32_t>( program->GetBufferSize() ) );
                    filein.close();

                    printf( "ShaderConductor::Compiler::Dxil '%s'\n", outpath.c_str() );
                }
            }
        }

    }

    // spirv complele
    if ( cachexml.xml_options.OptionsCompile_SpirV ) {
        IDxcOperationResult* Result = nullptr;

        hr = dxc_compiler->Compile( includeSource, SourceName.c_str(), EntryPoint.c_str(), TargetProfile.c_str(), Arguments.data(), Arguments.size(), Defines.data(), Defines.size(), &dxc_includer, &Result );
        if ( FAILED( hr ) ) {
            IDxcBlobEncoding* errors = nullptr;
            if ( SUCCEEDED( Result->GetErrorBuffer( &errors ) ) && errors != nullptr ) {
                std::string temp_err( (const char*)errors->GetBufferPointer(), errors->GetBufferSize() );
                cache.set_shader_message( shader, temp_err );
                printf( "dxc_compiler error '%s'\n", temp_err.c_str() );

                errors->Release();
                errors = nullptr;
            } else {
                cache.set_shader_bad_compile( shader, "compile error" );
            }

            printf( "dxc_compiler not create spirv \n" );

            includeSource->Release();
            includeSource = nullptr;

            return;
        }

        //
        IDxcBlobEncoding* errors = nullptr;
        if ( SUCCEEDED( Result->GetErrorBuffer( &errors ) ) && errors != nullptr ) {
            std::string temp_err( (const char*)errors->GetBufferPointer(), errors->GetBufferSize() );
            if ( !temp_err.empty() ) {
                cache.set_shader_message( shader, temp_err );
                printf( "dxc_compiler message '%s'\n", temp_err.c_str() );
            }

            errors->Release();
            errors = nullptr;
        }

        //
        HRESULT status;
        hr = Result->GetStatus( &status );
        if ( SUCCEEDED( hr ) && SUCCEEDED( status ) ) {
            //
            IDxcBlob* program = nullptr;
            hr                = Result->GetResult( &program );

            //
            Result->Release();
            Result = nullptr;

            //
            if ( program != nullptr ) {
                std::string   outpath = cachexml.xml_dirs.dir_result_vk + "/" + shader.name;
                std::ofstream filein( outpath, std::ios::binary );
                if ( filein.is_open() ) {
                    filein.write( (const char*)program->GetBufferPointer(), static_cast<uint32_t>( program->GetBufferSize() ) );
                    filein.close();

                    printf( "ShaderConductor::Compiler::Dxil '%s'\n", outpath.c_str() );
                }
            }
        }
    }

    includeSource->Release();
    includeSource = nullptr;

    cache.set_shader_good( shader );
}


int main( int argc, char** argv ) {

    std::string patch_cfg = "shaders.xml";

    if ( argc == 2 ) { 
        patch_cfg = argv[1];
    }

    printf( "set config '%s'\n", patch_cfg.c_str() );
     
    if ( compiler_init() ) {
        if ( cachexml.load( patch_cfg.c_str() ) ) {
            if ( cache.update( &cachexml ) ) {
                for ( auto& shader : cache.shaderlist ) { 
                    compiler_process( shader ); 
                }
            }
            cachexml.save( patch_cfg.c_str() );
        }
        cachexml.destroy();
    }
    compiler_free();

    return 0;
}

