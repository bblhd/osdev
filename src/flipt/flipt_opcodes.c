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
	{.symbol="#", .opcode=OP_DEL},
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
	{.symbol="@", .opcode=OP_LOAD},
	{.symbol="$", .opcode=OP_STORE},
	{.symbol="", .opcode=OP_END},
};

//xxxxxxLL
//L -> 1<<L // size of either name bytes length
//xxxSSRLL
//R -> boolean(isarray)
//L -> 1<<L // size of either array length bytes or just length
//S:
	//[add 0 zeros every 1] 8in, 8out
	//[add 1 zeros every 1] 8in, 16out
	//[add 3 zeros every 1] 8in, 32out
	//[add 2 zeros every 2] 16in, 32out


void flipt_setupOpNames(char *names[]) {
	names[OP_END] = "end";
	names[OP_START] = "start";
	
	names[OP_GET] = "get";
	names[OP_SET] = "set";
	
	names[OP_LOAD] = "load";
	names[OP_STORE] = "store";
	
	names[OP_INPUT] = "output";
	names[OP_OUTPUT] = "output";
	
	names[OP_PUSH] = "push";
	names[OP_PUSHN] = "push_negative";
	names[OP_PUSHSTR] = "push_string";
	
	names[OP_DEL] = "delete";
	names[OP_SWAP] = "swap";
	names[OP_DUP] = "duplicate";
	
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
}
