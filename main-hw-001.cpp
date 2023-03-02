
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef double f64;
typedef float  f32;

typedef int32_t b32;

#define BYTES(count)      (count)
#define KILO_BYTES(count) (BYTES(count) * 1024)
#define MEGA_BYTES(count) (KILO_BYTES(count) * 1024)

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))

#define ASSERT(statement) if(!(statement)) { int* x = 0; *x = 0; }
#define INVALID_DEFAULT_CASE default: { ASSERT(!"Invalid Default Case"); }break;

struct file_data
{
	b32 IsValid;
	u32 FileSize;
	void* Data;
};

file_data
ReadFile(char* fileName)
{
	file_data result = {};
	FILE* file = fopen(fileName, "r");
	
	if(file)
	{
		fseek(file, 0, SEEK_END);
		result.FileSize = ftell(file);
		fseek(file, 0, SEEK_SET);
		
		result.Data = calloc(1, result.FileSize);
		
		if(result.Data)
		{
			result.IsValid = (fread(result.Data, result.FileSize, 1, file) == 1);
		}
		
		fclose(file);
	}
	
	return(result);
}

struct output_buffer
{
	u32 Used;
	u8 Data[1024];
};

output_buffer GlobalOutputBuffer = {};

void
PushNullTerminatedStringToGlobalOutputBuffer(char* s)
{
	while(*s)
	{
		ASSERT(GlobalOutputBuffer.Used < ARRAY_LENGTH(GlobalOutputBuffer.Data));
		u32 index = GlobalOutputBuffer.Used;
		GlobalOutputBuffer.Data[index] = *s++;
		++GlobalOutputBuffer.Used;
	}
}

struct instruction_stream
{
	u32 Size;
	u8* Bytes;
};

void
PrintByteBits(u8* byte, s32 count = 1)
{
	ASSERT(count > 0);
	
	for(s32 offset = 0; offset < count; ++offset)
	{
		byte += offset;
		u8 b = *byte;
		printf("%x | ", b);
		
		for(s32 i = 7; i >= 0; --i)
		{
			printf("%d", (b >> i) & 1);
			
			if(i == 4)
			{
				printf("-");
			}
		}
		
		printf(" |\t");
		
	}
	
	printf("\n");
}

#define OPCODE_MOV_R_M_R 0X88

#define OPCODE_MASK 0xFC
#define MASK_OPCODE(byte) ((byte) & OPCODE_MASK)

#define MASK_OPERATION_MODE(byte) ((byte) & 0xC0)
#define MEMORY_MODE_NO_DISPLACEMENT 0x0
#define MEMORY_MODE_8_BIT 0x40
#define MEMORY_MODE_16_BIT 0x80
#define REGISTER_MODE_NO_DISPLACEMENT 0xC0

#define MASK_REG_FIELD(byte) ((byte) & 0x38)
#define MASK_RM_FIELD(byte) ((byte) & 0x07)

#define MASK_OPERATION_WIDTH_BIT(byte) ((byte) & 0x01)

#define MASK_DIRECTION_BIT(byte) ((byte) & 0x02)

char* GlobalRegisterTable[] =
{
	"al", "ax", //000
	"cl", "cx", //001
	"dl", "dx", //010
	"bl", "bx", //011
	"ah", "sp", //100
	"ch", "bp", //101
	"dh", "si", //110
	"bh", "di"  //111
};

void
DecodeInstructionStream(instruction_stream* instructionStream)
{
	PushNullTerminatedStringToGlobalOutputBuffer("bits 16\n");
	
	for(u32 offset = 0; offset < instructionStream->Size;)
	{
		u8 byte1 = instructionStream->Bytes[offset];
		
		switch(MASK_OPCODE(byte1))
		{
			case OPCODE_MOV_R_M_R:
			{
				//printf("OPCODE_MOV_R_M_R"); PrintByteBits(instructionStream->Bytes + offset, 2);
				u8 byte2 = instructionStream->Bytes[offset + 1];
				offset += 2;
				char* source = 0;
				char* destination = 0;
				
				switch(MASK_OPERATION_MODE(byte2))
				{
					case REGISTER_MODE_NO_DISPLACEMENT:
					{
						u8 operationWidthBit = MASK_OPERATION_WIDTH_BIT(byte1);
						u8 regField = MASK_REG_FIELD(byte2) >> 3;
						u8 rmField = MASK_RM_FIELD(byte2) >> 0;
						
						if(MASK_DIRECTION_BIT(byte1))
						{
							destination = GlobalRegisterTable[(2 * regField) + operationWidthBit];
							source = GlobalRegisterTable[(2 * rmField) + operationWidthBit];
						}
						else
						{
							destination = GlobalRegisterTable[(2 * rmField) + operationWidthBit];
							source = GlobalRegisterTable[(2 * regField) + operationWidthBit];
						}
						
					}break;
					INVALID_DEFAULT_CASE;
				}
				
				
				char buffer[16] = {};
				sprintf(buffer, "mov %s, %s\n", destination, source);
				PushNullTerminatedStringToGlobalOutputBuffer(buffer);
				//printf("mov %s, %s\n", destination, source);
				
			}break;
			INVALID_DEFAULT_CASE;
		}
	}
}

inline b32
IsWhiteSpace(char c)
{
	b32 result = (c == ' ' || c == '\n' || c == '\r');
	return(result);
}

b32
TestDecoding(char* fileName)
{
	b32 result = 0;
	file_data file = ReadFile(fileName);
	
	if(file.IsValid)
	{
		char* original = (char*)file.Data;
		char* generated = (char*)GlobalOutputBuffer.Data;
		
		while(*original && *generated)
		{
			while(IsWhiteSpace(*original)) { ++original; }
			while(IsWhiteSpace(*generated)) { ++generated; }
			
			result = *original == *generated;
			
			if(result == 0) 
			{ 
				break; 
			}
			
			++generated; ++original;
		}
		
	}
	else
	{
		printf("Cannot open file: %s\n", fileName);
	}
	
	return(result);
}

int
main(int argc, char** args)
{
	char* fileName = "test_asm";
	file_data file = ReadFile(fileName);
	
	if(file.IsValid)
	{
		
		instruction_stream instructionStream;
		instructionStream.Size = file.FileSize;
		instructionStream.Bytes = (u8*)file.Data;
		
		DecodeInstructionStream(&instructionStream);
		
		printf("%s\n", GlobalOutputBuffer.Data);
		
		if(TestDecoding("test_asm.asm")) 
		{ 
			printf("Sucessfully Decoded\n"); 
		}
		else
		{
			printf("Decoding Failure!!\n");
		}
	}
	else
	{
		printf("Cannot open file: %s\n", fileName);
	}
	
	return(0);
}
