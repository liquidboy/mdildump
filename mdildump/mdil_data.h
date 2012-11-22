#pragma once

#pragma pack(push,1)
struct	mdil_header
{
	DWORD	hdrSize;	// size of header in bytes
	DWORD	magic;	// ��MDIL��
	DWORD	version;	// currently 0x00020006
	DWORD	typeMapCount;	// number of entries in type map section
	DWORD	methodMapCount;	// number of entries in method map section
	DWORD	genericInstSize;	// size of generic inst section
	DWORD	extModRefsCount;	// number of entries in external module sect.
	DWORD	extTypeRefsCount;	// number of entries in external type section
	DWORD	extMemberRefsCount;	// number of entries in external member sect.
	DWORD	typeSpecCount;	// number of entries in typespec section
	DWORD	methodSpecCount ;	// number of entries in methodspec section
	DWORD	section10Size;	
	DWORD	namePoolSize;	// size of name pool in bytes
	DWORD	typeSize;	// size of compact type layout section
	DWORD	userStringPoolSize;	// size of user string pool
	DWORD	codeSize;		// size of MDIL code
	DWORD	section16Size;
	DWORD	section17Size;
	DWORD	debugMapCount;	// number of entries in debug info map
	DWORD	debugInfoSize;	// size of debug info<$1 tr>
	DWORD	timeDateStamp;
	DWORD	subsystem;	
	LPCVOID	baseAddress;	
	DWORD	entryPointToken;	// method def token for the entry point or 0

	enum mdil_header_flags
	{
		EntryPointReturnsVoid	= 0x01,
		WellKnownTypesPresent	= 0x02,
		TargetArch_Mask	= 0x0c,
		TargetArch_X86	= 0x00,
		TargetArch_AMD64	= 0x04,
		TargetArch_IA64	= 0x08,
		// room for future TargetArch_...   // it looks like TargetArch_ARM = _AMD64 | _IA64 !? 
		DebuggableILAssembly	= 0x10,	// Assembly created with /debug
		DebuggableMDILCode	= 0x20,	// MDIL file created with /debug
		IsHDRfile	= 0x40,
	};

	DWORD	flags;
	DWORD	Unknown;

	enum PlatformID
	{
		PlatformID_Unknown = 0,
		PlatformID_Triton = 1,
	};

	DWORD	platformID;
	DWORD	platformDataSize;
	DWORD	code1Size;
	DWORD	debugInfo1Size;
	WORD	is_4;
	DWORD	is_C68D0000;
	WORD	is_0;
	DWORD	is_0_too;
};

struct mdil_header_2
{
	DWORD	size; // should be 120 bytes
	DWORD	field_04;
	DWORD	field_08;
	DWORD	field_0c;
	DWORD	field_10;
	DWORD	field_14;
	DWORD	field_18;
	DWORD	field_1c;
	DWORD	field_20;
	DWORD	field_24;
	DWORD	field_28;
	DWORD	field_2c;
	DWORD	field_30;
	DWORD	field_34;
	DWORD	field_38;
	DWORD	field_3c;
	DWORD	field_40;
	DWORD	field_44;
	DWORD	field_48;
	DWORD	field_4c;
	DWORD	field_50;
	DWORD	field_54;
	DWORD	field_58;
	DWORD	field_5c;
	DWORD	field_60;
	DWORD	section_21_count;
	DWORD	section_22_count;
	DWORD	field_6c;
	DWORD	field_70;
	DWORD	field_74;
};
#pragma pack(pop)

struct	GenericInst
{
	WORD InstCount;
	BYTE Flags;
	BYTE Arity;
};

struct	ExtModRef
{
	ULONG	ModName;
	ULONG	RefName;
};

struct	ExtTypeRef
{
	ULONG	module	: 14;	// 16383 max modules to import from
	ULONG	ordinal	: 18;	// 262143 max types within a module
};

struct ExtMemberRef
{
	ULONG	extTypeRid : 15;	// 32767 max types to import
	ULONG	isTypeSpec : 1;	// refers to typespec?
	ULONG	isField : 1;	// is this a field or a	method?
	ULONG	ordinal : 15;	// 32767 max fields or	methods in a type
};

struct mdil_instruction {
	unsigned long offset;
	unsigned long length;
	std::string opcode;
	std::string operands;
	std::string annotation;

	void set(const std::string&& opcode) {
		this->opcode = move(opcode);
	}

	void set(const std::string&& opcode, const std::string&& operands) {
		this->opcode = move(opcode);
		this->operands = move(operands);
	}
};

template<typename T>
class shared_vector : public std::shared_ptr<std::vector<T>> {
public:
	typename std::vector<T>::size_type size() const { return get() ? get()->size() : 0; }
	void resize(typename std::vector<T>::size_type size) { if (get()) get()->resize(size); else reset(new std::vector<T>(size)); }
};

struct mdil_method
{
	size_t offset;
	size_t global_offset;
	size_t size;

	size_t routine_offset;
	size_t routine_size;
	unsigned long exception_count;

	std::vector<std::shared_ptr<mdil_instruction>> routine;

	mdil_method(size_t _offset, size_t _global_offset, size_t _size, size_t _routine_offset, size_t _routine_size, unsigned long _exception_count)
		: offset(_offset), global_offset(_global_offset), size(_size), routine_offset(_routine_offset), routine_size(_routine_size), exception_count(_exception_count) {}
};

struct mdil_code
{
	shared_vector<unsigned char> data;
	std::vector<mdil_method> methods;
};

class mdil_data {
public:
	std::shared_ptr<mdil_header>	header;
	std::shared_ptr<mdil_header_2>	header_2;
	shared_vector<unsigned char>	platform_data;
	shared_vector<unsigned long>	well_known_types;
	shared_vector<unsigned long>	type_map;
	shared_vector<unsigned long>	method_map;
	shared_vector<unsigned char>	generic_instances;
	shared_vector<ExtModRef>		ext_module_refs;
	shared_vector<ExtTypeRef>		ext_type_refs;
	shared_vector<ExtMemberRef>		ext_member_refs;
	shared_vector<unsigned long>	type_specs;
	shared_vector<unsigned long>	method_specs;
	shared_vector<unsigned long>	section_10;
	shared_vector<char>				name_pool;
	shared_vector<unsigned char>	types;
	shared_vector<char>				user_string_pool;
	mdil_code						code_1;
	mdil_code						code_2;
	shared_vector<unsigned char>	section_16;
	shared_vector<unsigned char>	section_17;
	shared_vector<unsigned long>	debug_map;
	shared_vector<unsigned char>	debug_info_1;
	shared_vector<unsigned char>	debug_info_2;
	shared_vector<unsigned char>	section_21;
	shared_vector<unsigned char>	section_22;
};


