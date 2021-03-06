#include "stdafx.h"
#include "cli_metadata_reader.h"

#pragma comment(lib, "mscoree.lib")

cli_metadata_reader::cli_metadata_reader(const wchar_t* _filename) : filename(_filename)
{
}


cli_metadata_reader::~cli_metadata_reader(void)
{
}

bool cli_metadata_reader::init()
{
	CComPtr<ICLRMetaHost> host;
	CComPtr<ICLRRuntimeInfo> runtime;
	CComPtr<IMetaDataDispenserEx> dispenser;

	HRESULT hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, (void **)&host);
	if (SUCCEEDED(hr)) hr = host->GetRuntime(L"v4.0.30319", IID_ICLRRuntimeInfo, (void **)&runtime);
	if (SUCCEEDED(hr)) hr = runtime->GetInterface(CLSID_CorMetaDataDispenser, IID_IMetaDataDispenserEx, (void **)&dispenser);
	if (SUCCEEDED(hr)) hr = dispenser->OpenScope(filename.c_str(), ofReadOnly, IID_IMetaDataImport2, (IUnknown**)&metadata_import);
	return SUCCEEDED(hr) && metadata_import;
}

std::wstring cli_metadata_reader::format_token( mdToken token, bool no_fallback )
{
	if (metadata_import) {
		if (TypeFromToken(token) == mdtTypeDef) {
			wchar_t name[1024];
			if(SUCCEEDED(metadata_import->GetTypeDefProps(token, name, _countof(name), NULL, NULL, NULL))) {
				return std::wstring(name);
			}
		}

		if (TypeFromToken(token) == mdtTypeRef) {
			wchar_t name[1024];
			if(SUCCEEDED(metadata_import->GetTypeRefProps(token, NULL, name, _countof(name), NULL))) {
				return std::wstring(name);
			}
		}

		MDUTF8CSTR name;
		if (SUCCEEDED(metadata_import->GetNameFromToken(token, &name))) {
			USES_CONVERSION;
			return std::wstring(A2CW_CP(name, CP_UTF8));
		}

		if (TypeFromToken(token) == mdtGenericParam) {
			wchar_t name[1024];
			if(SUCCEEDED(metadata_import->GetGenericParamProps(token, NULL, NULL, NULL, NULL, name, _countof(name), NULL))) {
				return std::wstring(name);
			}
		}

	}

	if (no_fallback) return std::wstring();
	else {
		std::wstringstream s;

		switch (TypeFromToken(token))
		{
		case mdtModule: s << L"module"; break;
		case mdtTypeRef: s << L"type_ref"; break;
		case mdtTypeDef: s << L"type"; break;
		case mdtFieldDef: s << L"field"; break;
		case mdtMethodDef: s << L"method"; break;
		case mdtParamDef: s << L"param"; break;
		case mdtInterfaceImpl: s << L"if_impl"; break;
		case mdtMemberRef: s << L"member_ref"; break;
		case mdtModuleRef: s << L"module_ref"; break;
		case mdtTypeSpec: s << L"type_spec"; break;
		case mdtGenericParam: s << L"gen_param"; break;
		case mdtMethodSpec: s << L"method_spec"; break;
		default: s << std::uppercase << std::hex << std::setfill(L'0') << std::setw(2) << TypeFromToken(token);
			break;
		}

		s << L"_" << std::uppercase << std::hex << std::setfill(L'0') << std::setw(6) << RidFromToken(token);
		return s.str();
	}
}

void cli_metadata_reader::dump_type( mdTypeDef token )
{
	bool more = true;
	HCORENUM ce = NULL;
	while (more) {
		mdFieldDef fields[100] = {};
		DWORD count = 0;
		HRESULT hr = metadata_import->EnumFields(&ce, token, fields, _countof(fields), &count);
		if (FAILED(hr)) break;
		more = hr != S_FALSE;
		for (DWORD i = 0; i < count; ++i) {
			printf_s("field %08X = %S\n", fields[i], format_token(fields[i]).c_str());
		}
	}

	more = true;
	ce = NULL;
	while (more) {
		mdMethodDef methods[100] = {};
		DWORD count = 0;
		HRESULT hr = metadata_import->EnumMethods(&ce, token, methods, _countof(methods), &count);
		if (FAILED(hr)) break;
		more = hr != S_FALSE;
		for (DWORD i = 0; i < count; ++i) {
			printf_s("method %08X = %S\n", methods[i], format_token(methods[i]).c_str());
		}
	}
}
