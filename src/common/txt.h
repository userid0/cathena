// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _COMMON_TXT_H_
#define _COMMON_TXT_H_

#include "../common/cbasetypes.h"


// Return codes
#define TXT_ERROR -1
#define TXT_SUCCESS 0


typedef enum TxtDataType
{
	TXTDT_NULL,
	// platform dependent size
	TXTDT_BOOL,
	TXTDT_CHAR,
	TXTDT_SHORT,
	TXTDT_INT,
	TXTDT_LONG,
	TXTDT_UCHAR,
	TXTDT_USHORT,
	TXTDT_UINT,
	TXTDT_ULONG,
	TXTDT_TIME,
	// other
	TXTDT_STRING,
	TXTDT_CSTRING,
	TXTDT_ENUM,
	TXTDT_UENUM,
}
TxtDataType;


typedef struct Txt Txt; // Txt handle (private access)


/// Allocates and initializes a new Txt handle.
///
/// @return new Txt handle or NULL
Txt* Txt_Malloc(void);


/// Records parameters which will be used during subsequent processing.
///
/// @param self Txt handle
/// @param str String to be parsed/written
/// @param length Size (for parsing) or capacity (for writing) of the string
/// @param max_fields Maximum number of fields to parse/write
/// @param field_delim Delimiter between fields
/// @param block_delim Delimiter between blocks of fields
/// @param escapes Characters to escape when writing fields (all delimiters should be present here)
/// @return TXT_SUCCESS or TXT_ERROR
int Txt_Init(Txt* self, char* str, size_t length, size_t max_fields, char field_delim, char block_delim, const char* escapes);


/// Binds a variable to the specified field.
///
/// @param self Txt handle
/// @param idx Field's ordinal (starting from 0)
/// @param buffer_type Type of variable
/// @param buffer Address of variable
/// @param buffer_len Size of variable
/// @return TXT_SUCCESS or TXT_ERROR
int Txt_Bind(Txt* self, size_t idx, TxtDataType buffer_type, void* buffer, size_t buffer_len);


/// Parses the string and writes values to bound variables.
///
/// @param self Txt handle
/// @return TXT_SUCCESS or TXT_ERROR
int Txt_Parse(Txt* self);


/// Constructs the string using values from bound variables.
///
/// @param self Txt handle
/// @return TXT_SUCCESS or TXT_ERROR
int Txt_Write(Txt* self);


/// Number of fields in the string.
/// Determined during Parse()/Write().
///
/// @param self Txt handle
/// @return Number of fields, or 0 if no procsesing has been done yet
size_t Txt_NumFields(Txt* self);


/// Frees a Txt handle returned by Txt_Malloc().
///
/// @param self Txt handle
/// @return TXT_SUCCESS or TXT_ERROR
int Txt_Free(Txt* self);


#endif // _COMMON_TXT_H_
