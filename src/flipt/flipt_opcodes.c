#include <flipt.h>
#include <stdint.h>

struct {
	char *symbol;
	uint8_t opcode;
} flipt_opTable[] = {
	{.symbol="..", .opcode=OP_DUP},
	{.symbol="~=", .opcode=OP_NE},
	{.symbol="?!", .opcode=OP_IFELSE},
	{.symbol=">=", .opcode=OP_GE},
	{.symbol="??", .opcode=OP_WHILE},
	{.symbol="<=", .opcode=OP_LE},
	{.symbol="|", .opcode=OP_SWAP},
	{.symbol=".", .opcode=OP_OUTPUT},
	{.symbol="!", .opcode=OP_CALL},
	{.symbol="+", .opcode=OP_ADD},
	{.symbol="-", .opcode=OP_SUB},
	{.symbol="<", .opcode=OP_LT},
	{.symbol=">", .opcode=OP_GT},
	{.symbol="=", .opcode=OP_EQ},
	{.symbol="*", .opcode=OP_MULT},
	{.symbol="?", .opcode=OP_IF},
	{.symbol="~", .opcode=OP_NOT},
	{.symbol="/", .opcode=OP_DIV},
	{.symbol="%", .opcode=OP_MOD},
	{.symbol="", .opcode=OP_END},
};

void flipt_setupOpNames(char *names[]) {
	names[OP_END] = "end";
	
	names[OP_CALL] = "call";
	names[OP_IF] = "if";
	names[OP_WHILE] = "while";
	names[OP_IFELSE] = "ifelse";
	
	names[OP_NOT] = "not";
	names[OP_NE] = "not_equal";
	names[OP_EQ] = "equal";
	names[OP_GT] = "gt";
	names[OP_LT] = "lt";
	names[OP_GE] = "ge";
	names[OP_LE] = "le";
	
	names[OP_ADD] = "add";
	names[OP_SUB] = "sub";
	names[OP_MULT] = "mult";
	names[OP_DIV] = "div";
	names[OP_MOD] = "mod";
	
	names[OP_SWAP] = "swap";
	names[OP_DUP] = "duplicate";
	names[OP_OUTPUT] = "output";
	
	names[OP_PUSH] = "push";
	names[OP_PUSHOFFSET] = "push_offset";
	names[OP_PUSHN] = "push_negative";
	names[OP_PUSHSTR] = "push_string";
	
	names[OP_VALUEOF] = "valueof";
	names[OP_BIND] = "bind";
	names[OP_UNBIND] = "unbind";
	names[OP_REBIND] = "rebind";
	
	names[OP_SKIP] = "skip";
}
