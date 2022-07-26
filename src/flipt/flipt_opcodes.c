#include <flipt.h>
#include <stdint.h>

struct {
	char *symbol;
	uint8_t opcode;
} flipt_opTable[] = {
	{.symbol="~=", .opcode=OP_NE},
	{.symbol="?!", .opcode=OP_IFELSE},
	{.symbol=">=", .opcode=OP_GE},
	{.symbol="??", .opcode=OP_WHILE},
	{.symbol="<=", .opcode=OP_LE},
	{.symbol="!", .opcode=OP_CALL},
	{.symbol="+", .opcode=OP_ADD},
	{.symbol="-", .opcode=OP_SUB},
	{.symbol="<", .opcode=OP_LT},
	{.symbol=">", .opcode=OP_GT},
	{.symbol="=", .opcode=OP_EQ},
	{.symbol="*", .opcode=OP_MULT},
	{.symbol="?", .opcode=OP_IF},
	{.symbol="~", .opcode=OP_NOT},
	{.symbol="/", .opcode=OP_DIV}
};

char *flipt_opNames[] = {
	"end",
	"pushb", "pushs", "pushw",
	"pushbn", "pushsn", "pushwn",
	"pushstrlb", "pushstrls", "pushstrlw",
	"valueoflb", "valueofls", "valueoflw",
	"bindlb", "bindls", "bindlw",
	"unbindlb", "unbindls", "unbindlw",
	"rebindlb", "rebindls", "rebindlw",
	"pushoffsetb", "pushoffsets", "pushoffsetw",
	"skip", "call", "if", "while", "ifelse",
	"not", "ne", "eq", "gt", "lt", "ge", "le",
	"add", "sub", "mult", "div"
};
