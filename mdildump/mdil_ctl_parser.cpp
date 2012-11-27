#include "stdafx.h"
#include "mdil_ctl_parser.h"

using namespace std;

enum CompactLayoutToken
{
	INVALID,
	START_TYPE,
	SMALL_START_TYPE,
	SIMPLE_START_TYPE,
	MODEST_START_TYPE,
	END_TYPE,
	IMPLEMENT_INTERFACE,
	ADVANCE_ENCLOSING_TYPEDEF,
	ADVANCE_METHODDEF,
	ADVANCE_METHODDEF_SHORT_MINUS_8,
	ADVANCE_METHODDEF_SHORT_0 = 0x11,
	ADVANCE_METHODDEF_SHORT_PLUS_8 = 0x19,
	ADVANCE_FIELDDEF,
	ADVANCE_FIELDDEF_SHORT_MINUS_8,
	ADVANCE_FIELDDEF_SHORT_0 = 0x23,
	ADVANCE_FIELDDEF_SHORT_PLUS_8 = 0x2B,
	FIELD_OFFSET,
	IMPLEMENT_INTERFACE_METHOD,
	METHOD,
	NORMAL_METHOD,
	SIMPLE_METHOD,
	PINVOKE_METHOD = 0x50,
	METHOD_IMPL,
	FIELD_INSTANCE,
	FIELD_STATIC,
	FIELD_THREADLOCAL,
	FIELD_CONTEXTLOCAL,
	FIELD_RVA,
	FIELD_SIMPLE,
	FIELD_MAX = 0x67,
	NATIVECALLABLE_METHOD = 0x67,
	RUNTIME_IMPORT_METHOD,
	RUNTIME_EXPORT_METHOD
};

uint8_t mdil_ctl_parser::read_byte()
{
	return m_buffer[m_pos++];
}

uint32_t mdil_ctl_parser::read_uint32_le()
{
	unsigned long ret = m_buffer[m_pos] + (m_buffer[m_pos+1] << 8) + (m_buffer[m_pos+2] << 16) + (m_buffer[m_pos+3] << 24);
	m_pos += 4;
	return ret;
}

int32_t mdil_ctl_parser::read_compressed_int32()
{
	return (int32_t) read_compressed_uint32();
}

uint32_t mdil_ctl_parser::read_compressed_uint32()
{
	uint32_t ret = 0;

	uint8_t b = m_buffer[m_pos++];

	if ((b & 1) == 0) {
		ret = b >> 1;
	} else if ((b & 2) == 0) {
		ret = (b >> 2) + (m_buffer[m_pos++] << 6);
	} else if ((b & 4) == 0) {
		ret = (b >> 3) + (m_buffer[m_pos] << 5) + (m_buffer[m_pos+1] << 13);
		m_pos += 2;
	} else if ((b & 8) == 0) {
		ret = (b >> 4) + (m_buffer[m_pos] << 4) + (m_buffer[m_pos+1] << 12) + (m_buffer[m_pos+2] << 20);
		m_pos += 3;
	} else { // 0x0F
		ret = read_uint32_le();
	}

	return ret;
}

uint32_t mdil_ctl_parser::read_compressed_type_token()
{
	uint32_t ret = read_compressed_uint32();
	uint8_t token_type_flag = ret & 3;
	CorTokenType token_type = (token_type_flag == 0) ? mdtModule : (token_type_flag == 1) ? mdtTypeDef : (token_type_flag == 2) ? mdtTypeRef : mdtTypeSpec;
	return (ret >> 2) | token_type;
}

uint32_t mdil_ctl_parser::read_compressed_method_token()
{
	uint32_t ret = read_compressed_uint32();
	uint8_t token_type_flag = ret & 3;
	CorTokenType token_type = (token_type_flag == 0) ? mdtModule : (token_type_flag == 1) ? mdtMethodDef : (token_type_flag == 2) ? mdtMemberRef : mdtMethodSpec;
	return (ret >> 2) | token_type;
}

void mdil_ctl_parser::dump_type_map( const char* title /*= nullptr*/, const char* description /*= nullptr*/ )
{
//	print_vector_size(m_data.type_map, title, description);

	for (unsigned long i = 1; i < m_data.type_map.size(); i++) {
		auto type_offset = m_data.type_map->at(i);
		if (type_offset == 0) continue;
		printf_s("TYPM(%04X)=TYPE(%04X)\n", i, type_offset);
		m_pos = type_offset;
		bool res = dump_type_def();
		if (!res) printf_s("*ERROR*\n");
		printf_s("\n");
	}

	printf_s("\n");
}

void ctl_dump_flags( uint32_t flags )
{
	bool first = true;
	for (int i = 0; i < 32; i++) {
		if (flags & (1 << i)) {
			if (first) first = false; else { printf_s("|"); }
			printf_s("0x%x", 1 << i);
		}
	}
}

bool mdil_ctl_parser::dump_type_def_members( uint32_t fieldCount, uint32_t methodCount, uint32_t interfaceCount )
{
	static const uint32_t field_encodings[] = { 0x0112, 0x1112, 0x0608, 0x0108, 0x0102, 0x0312, 0x0612, 0x1108,
												0x0308, 0x1612, 0x0111, 0x1312, 0x0618, 0x0309, 0x0609, 0x0311, };
	static const char* field_storage[] = { "Instance", "Static", "ThreadLocal", "ContextLocal", "RVA" };
	static const char* field_protection[] = { "Private Scope", "Private", "Family and Assembly", "Assembly", "Family", "Family or Assembly", "Public" };

	bool fine = true;

	if ((m_buffer[m_pos] == 0x73) && (m_buffer[m_pos+1] == 0x0A)) {
		printf_s("73 0A ; what's this ?\n");
		m_pos += 2;
	}

	for (uint32_t i = 0; i < fieldCount; i++) {
		printf_s("\tField %d\n", i);
		bool go_on = false;
		do {
			go_on = false;
			uint8_t byte = m_buffer[m_pos++];
			if (byte == ADVANCE_FIELDDEF) {
				printf_s("\t\tAdvance Diff = %04X\n", read_compressed_int32());
				go_on = true;
			} else if (byte == FIELD_OFFSET) {
				printf_s("\t\tExplicit Offset = %04X\n", read_compressed_uint32());
			} else if ((byte >= ADVANCE_FIELDDEF_SHORT_MINUS_8) && (byte <= ADVANCE_FIELDDEF_SHORT_PLUS_8)) {
				printf_s("\t\tAdvance Diff = %04X\n", byte - ADVANCE_FIELDDEF_SHORT_0);
				go_on = true;
			} else if ((byte >= FIELD_SIMPLE) && (byte  < FIELD_MAX)) {
				printf_s("\t\tEncoding Index = %04X\n", byte - FIELD_SIMPLE);
				uint32_t encoding = field_encodings[byte - FIELD_SIMPLE];
				printf_s("\t\tStorage = %s\n", field_storage[encoding >> 12]);
				printf_s("\t\tProtection = %s\n", field_protection[(encoding >> 8) & 0xf]);
				printf_s("\t\tType = %04X\n", encoding & 0xff);
			} else if ((byte >= FIELD_INSTANCE) && (byte <= FIELD_RVA)) {
				printf_s("\t\tStorage = %s\n", field_storage[byte - FIELD_INSTANCE]);
				uint8_t b = m_buffer[m_pos++];
				printf_s("\t\tProtection = %s\n", field_protection[b >> 5]);
				printf_s("\t\tType = %04X\n", b & 0x1F);
			} else {
				printf_s("\t\tUnknown %02X\n", byte);
				fine = false;
			}
		} while (go_on);

		if (!fine) break;
	}

	if (fine) for (uint32_t i = 0; i < methodCount; i++) {
		printf_s("\tMethod %d\n", i);
		bool go_on = false;
		do {
			dump_known_unknowns();

			go_on = false;
			uint8_t byte = m_buffer[m_pos++];
			if (byte == ADVANCE_METHODDEF) {
				printf_s("\t\tAdvance Diff = %04X\n", read_compressed_int32());
				go_on = true;
			} else if ((byte >= ADVANCE_METHODDEF_SHORT_MINUS_8) && (byte <= ADVANCE_METHODDEF_SHORT_PLUS_8)) {
				printf_s("\t\tAdvance Diff = %04X\n", byte - ADVANCE_METHODDEF_SHORT_0);
				go_on = true;
			} else if (byte == METHOD) {
				printf_s("\t\tMethod\n");
				printf_s("\t\tAttributes = "); ctl_dump_flags(read_compressed_uint32()); printf_s("\n");
				printf_s("\t\tImpl Attributes = %08X\n", read_compressed_uint32());
				printf_s("\t\tImpl Hints = %08X\n", read_compressed_uint32());
			} else if (byte == NORMAL_METHOD) {
				printf_s("\t\tNormal Method\n");
				printf_s("\t\tAttributes = "); ctl_dump_flags(read_compressed_uint32()); printf_s("\n");
			} else if ((byte >= SIMPLE_METHOD) && (byte < (SIMPLE_METHOD + 32))) {
				printf_s("\t\tSimple Method\n");
				printf_s("\t\tEncoding Index = %04X\n", byte - SIMPLE_METHOD);
			} else if (byte == PINVOKE_METHOD) {
				printf_s("\t\tPInvoke Method\n");
				printf_s("\t\tAttributes = "); ctl_dump_flags(read_compressed_uint32()); printf_s("\n");
				printf_s("\t\tImpl Attributes = %08X\n", read_compressed_uint32());
				printf_s("\t\tImpl Hints = %08X\n", read_compressed_uint32());
				printf_s("\t\tModule Name Offset = %04X\n", read_compressed_uint32());
				printf_s("\t\tEntry Point Name Offset = %08X\n", read_compressed_uint32());
			} else if (byte == NATIVECALLABLE_METHOD) {
				printf_s("\t\tNative Callable Method\n");
				printf_s("\t\tAttributes = "); ctl_dump_flags(read_compressed_uint32()); printf_s("\n");
				printf_s("\t\tImpl Attributes = %08X\n", read_compressed_uint32());
				printf_s("\t\tImpl Hints = %08X\n", read_compressed_uint32());
				printf_s("\t\tEntry Point Name Offset = %08X\n", read_compressed_uint32());
				printf_s("\t\tCalling Convention = %04X\n", read_compressed_uint32());
			} else if (byte == RUNTIME_IMPORT_METHOD) {
				printf_s("\t\tRuntime Import Method\n");
				printf_s("\t\tAttributes = "); ctl_dump_flags(read_compressed_uint32()); printf_s("\n");
				printf_s("\t\tImpl Attributes = %08X\n", read_compressed_uint32());
				printf_s("\t\tImpl Hints = %08X\n", read_compressed_uint32());
				printf_s("\t\tEntry Point Name Offset = %08X\n", read_compressed_uint32());
				printf_s("\t\tCalling Convention = %04X\n", read_compressed_uint32());
			} else if (byte == RUNTIME_EXPORT_METHOD) {
				printf_s("\t\tRuntime Export Method\n");
				printf_s("\t\tAttributes = "); ctl_dump_flags(read_compressed_uint32()); printf_s("\n");
				printf_s("\t\tImpl Attributes = %08X\n", read_compressed_uint32());
				printf_s("\t\tImpl Hints = %08X\n", read_compressed_uint32());
				printf_s("\t\tEntry Point Name Offset = %08X\n", read_compressed_uint32());
			} else {
				printf_s("\t\tUnknown %02X\n", byte);
				fine = false;
			}
		} while (go_on);

		if (!fine) break;
	}

	if (fine) for (uint32_t i = 0; i < interfaceCount; i++) {
		printf_s("\tInterface %d\n", i);

		uint8_t byte = read_byte();

		if (byte == IMPLEMENT_INTERFACE) {
			printf_s("\t\tImplement Interface = %08X\n", read_compressed_type_token());
		} else {
			printf_s("\t\tUnknown %02X\n", byte);
			fine = false;
		}

		if (!fine) break;
	}

	if (fine) {
		bool go_on = true;

		do {
			go_on = true;

			uint8_t byte = m_buffer[m_pos]; // peeking

			unique_ptr<int32_t> diff;
			if (byte == ADVANCE_METHODDEF) {
				m_pos++;
				diff.reset(new int32_t(read_compressed_int32()));
				byte = m_buffer[m_pos]; // peeking
			} else if ((byte >= ADVANCE_METHODDEF_SHORT_MINUS_8) && (byte <= ADVANCE_METHODDEF_SHORT_PLUS_8)) {
				m_pos++;
				diff.reset(new int32_t(byte - ADVANCE_METHODDEF_SHORT_0));
				byte = m_buffer[m_pos]; // peeking
			}

			if (byte == IMPLEMENT_INTERFACE_METHOD) {
				m_pos++;
				printf_s("\tImplement Interface Method\n");
				if (diff) printf_s("\t\tAdvance Diff = %04X\n", *diff);
				printf_s("\t\tInterface Method = %08X\n", read_compressed_method_token());
			} else go_on = false;

		} while (go_on);
	}

	return fine;
}


bool mdil_ctl_parser::dump_known_unknowns()
{
	while (true) {
		uint8_t byte = m_buffer[m_pos];
		if (byte == 0x6a) {
			printf_s("6A ; what's this ?\n");
			m_pos += 1;
		} else if (byte == 0x6f) {
			printf_s("6F %02X %02X ; what's this ?\n", m_buffer[m_pos+1], m_buffer[m_pos+2]);
			m_pos += 3;
		} else if (byte == 0xe5) {
			printf_s("E5 %02X ; what's this ?\n", m_buffer[m_pos+1]);
			m_pos += 2;
		} else if (byte == 0x25) {
			printf_s("25 %02X ; what's this ?\n", m_buffer[m_pos+1]);
			m_pos += 2;
		} else break;
	}
	return true;
}

bool mdil_ctl_parser::dump_type_def()
{
	bool fine = true;

	uint8_t byte = read_byte();

	if ((byte == 0x6a) && (m_buffer[m_pos] == 0x6f)) {
		printf_s("6A 6F %02X %02X; what's this ?\n", m_buffer[m_pos+1], m_buffer[m_pos+2]);
		m_pos += 3;
		byte = read_byte();
	}

	if (byte == ADVANCE_ENCLOSING_TYPEDEF) {
		printf_s("Advance Diff = %04X\n", read_compressed_uint32());
		byte = read_byte();
	}

	switch (byte)
	{
	case START_TYPE:
		{
			printf_s("Start\n");
			printf_s("Type Attributes = "); ctl_dump_flags(read_compressed_uint32()); printf_s("\n");
			printf_s("Base = %08X\n", read_compressed_type_token());
			uint32_t interfaceCount = read_compressed_uint32();
			printf_s("Interfaces Count = %d\n", interfaceCount);
			uint32_t fieldCount = read_compressed_uint32();
			printf_s("Field Count = %d\n", fieldCount);
			uint32_t methodCount = read_compressed_uint32();
			printf_s("Method Count = %d\n", methodCount);
			printf_s("New Virtual Method Count = %d\n", read_compressed_uint32());
			printf_s("Override Virtual Method Count = %d\n", read_compressed_uint32());
			fine = dump_type_def_members(fieldCount, methodCount, interfaceCount);
			break;
		}
	case SMALL_START_TYPE:
		{
			printf_s("Small Start\n");
			printf_s("TypeAttributes = "); ctl_dump_flags(read_compressed_uint32()); printf_s("\n");
			printf_s("Base = %08X\n", read_compressed_type_token());
			uint32_t counts = read_compressed_uint32();
			printf_s("Field Count = %d\n", counts & 7);
			printf_s("Method Count = %d\n", counts >> 3);
			fine = dump_type_def_members(counts & 7, counts >> 3, 0);
			break;
		}
	case SIMPLE_START_TYPE:
		{
			printf_s("Simple Start\n");
			printf_s("TypeAttributes = "); ctl_dump_flags(read_compressed_uint32()); printf_s("\n");
			printf_s("Base = %08X\n", read_compressed_type_token());
			uint32_t fieldCount = read_compressed_uint32();
			printf_s("Field Count = %d\n", fieldCount);
			uint32_t methodCount = read_compressed_uint32();
			printf_s("Method Count = %d\n", methodCount);
			fine = dump_type_def_members(fieldCount, methodCount, 0);
			break;
		}
	case MODEST_START_TYPE:
		{
			printf_s("Modest Start\n");
			printf_s("TypeAttributes = "); ctl_dump_flags(read_compressed_uint32()); printf_s("\n");
			printf_s("Base = %08X\n", read_compressed_type_token());
			uint32_t fieldCount = read_compressed_uint32();
			printf_s("Field Count = %d\n", fieldCount);
			uint32_t methodCount = read_compressed_uint32();
			printf_s("Method Count = %d\n", methodCount);
			uint32_t counts = read_compressed_uint32();
			printf_s("Interfaces Count = %d\n", counts & 3);
			printf_s("New Virtual Method Count = %d\n", (counts >> 2) & 3);
			printf_s("Override Virtual Method Count = %d\n", counts >> 4);
			fine = dump_type_def_members(fieldCount, methodCount, counts & 3);
			break;
		}
	default: printf_s("Unknown %02X\n", byte); fine = false; break;
	}

	if (fine) {
		byte = read_byte();
		if (byte == END_TYPE) printf_s("End\n", byte);
		else {  printf_s("Unknown %02X\n", byte); fine = false; }
	}


	return fine;
}

void mdil_ctl_parser::dump_type_specs( const char* title /*= nullptr*/, const char* description /*= nullptr*/ )
{
	for (unsigned long i = 1; i < m_data.type_specs.size(); i++) {
		auto type_spec = m_data.type_specs->at(i);
		unsigned long pos = 0;
		printf_s("TYPS(%04X)=TYPE(%04X)\n", i, type_spec);
		m_pos = type_spec;
		bool res = dump_type_spec();
		if (!res) printf_s("*ERROR*\n");
		printf_s(" (size=%04X, next=%04X)\n", m_pos - type_spec, m_pos);
		printf_s("\n");
	}

	printf_s("\n");
}

bool mdil_ctl_parser::dump_type_spec(uint32_t level)
{
	bool fine = true;

	uint8_t byte = read_byte();

	for (uint32_t l = 0; l < level; ++l) printf_s("\t");
	switch (byte)
	{
	case 0x01: printf_s("\tVoid\n"); break;
	case 0x02: printf_s("\tBoolean\n"); break;
	case 0x03: printf_s("\tChar\n"); break;
	case 0x04: printf_s("\tSByte\n"); break;
	case 0x05: printf_s("\tByte\n"); break;
	case 0x06: printf_s("\tShort\n"); break;
	case 0x07: printf_s("\tUShort\n"); break;
	case 0x08: printf_s("\tInt\n"); break;
	case 0x09: printf_s("\tUInt\n"); break;
	case 0x0a: printf_s("\tLong\n"); break;
	case 0x0b: printf_s("\tULong\n"); break;
	case 0x0c: printf_s("\tFloat\n"); break;
	case 0x0d: printf_s("\tDouble\n"); break;
	case 0x0e: printf_s("\tString\n"); break;
	case 0x0F: printf_s("\tUntraced Pointer\n"); dump_type_spec(level); break;
	case 0x10: printf_s("\tReference\n"); dump_type_spec(level); break;
	case 0x11: printf_s("\tStruct/Enum %08X\n", read_compressed_type_token()); break;
	case 0x12: printf_s("\tClass %08X\n", read_compressed_type_token()); break;
	case 0x15:
		printf_s("\tGeneric\n");
		fine = dump_type_spec(level);
		if (fine) {
			uint32_t count = read_compressed_uint32();
			for (uint32_t l = 0; l < level; ++l) printf_s("\t");
			printf_s("\tType Arguments: %04d\n", count);
			for (uint32_t i=0; i < count; ++i) {
				fine = dump_type_spec(level+1);
				if (!fine) break;
			}
		}
		break;
	case 0x18: printf_s("\tIntPtr\n"); break;
	case 0x19: printf_s("\tUIntPtr\n"); break;
	case 0x1b: printf_s("\tFunction Pointer\n"); break;
	case 0x13:
	case 0x1c: printf_s("\tObject ?? %008X\n", read_compressed_type_token()); break;
	case 0x1d: printf_s("\tVector\n"); dump_type_spec(level); break;
	case 0x1e:
		printf_s("\tKind_%02X %08X\n", byte, read_compressed_type_token()); // known unknowns
		break;
	default:
		printf_s("\tUnknown %02X\n", byte);
		fine = false;
		break;
	}

	return fine;
}