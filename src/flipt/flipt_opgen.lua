opcodesString = [[
end

pushb
pushs
pushw

pushbn
pushsn
pushwn

pushstrlb
pushstrls
pushstrlw

valueoflb
valueofls
valueoflw

bindlb
bindls
bindlw

unbindlb
unbindls
unbindlw

rebindlb
rebindls
rebindlw

pushoffsetb
pushoffsets
pushoffsetw

skip
call !
if ?
while ??
ifelse ?!

not ~
ne ~=
eq =
gt >
lt <
ge >=
le <=

add +
sub -
mult *
div /
]]

local names = {{}}
local symboltable = {}

for line in string.gmatch(opcodesString, "[^\n]*\n") do
	if line == "\n" then
		table.insert(names, {})
	else
		local name,symbol = string.match(line, "^(%a+)%s*(.*)\n$")
		table.insert(names[#names], name)
		if symbol ~= "" then
			table.insert(symboltable, {symbol=symbol, opcode=name})
		end
	end
end

table.sort(symboltable, function (a,b) return #(a.symbol) > #(b.symbol) end)

do
	h_file = ""
	
	function reg_h(str)
		h_file = h_file .. str.."\n"
	end
	
	reg_h("#ifndef FLIPT_H")
	reg_h("#define FLIPT_H")
	
	reg_h("#include <stdint.h>\n")
	
	reg_h("")
	
	reg_h("extern char *flipt_opNames[];")
	reg_h("void flipt_compile(char *source, void *out);")
	reg_h("void flipt_interpret(uint8_t *bytecode);")

	reg_h("")
	
	reg_h("enum Opcodes {")
	for i,t in ipairs(names) do
		if i > 1 then
			h_file = h_file .. ",\n"
		end
		h_file = h_file .. "\t"
		for ii,name in ipairs(t) do
			if ii > 1 then
				h_file = h_file .. ", "
			end
			h_file = h_file .. "OP_"..string.upper(name)
		end
	end
	reg_h("\n};\n")
	
	reg_h("#endif")
	
	local file = io.open("include/flipt.h", "w")
	file:write(h_file)
	file:close()
end

do
	c_file = ""
	
	function reg_c(str)
		c_file = c_file .. str.."\n"
	end
	
	reg_c("#include <flipt.h>")
	reg_c("#include <stdint.h>\n")

	reg_c("struct {\n\tchar *symbol;\n\tuint8_t opcode;\n} flipt_opTable[] = {")
	for i,o in ipairs(symboltable) do
		if i > 1 then
			c_file = c_file .. ",\n"
		end
		c_file = c_file .. "\t"
		c_file = c_file .. "{.symbol=\""..o.symbol.."\", .opcode=OP_"..string.upper(o.opcode).."}"
	end
	reg_c("\n};\n")
	
	reg_c("char *flipt_opNames[] = {")
	for i,t in ipairs(names) do
		if i > 1 then
			c_file = c_file .. ",\n"
		end
		c_file = c_file .. "\t"
		for ii,name in ipairs(t) do
			if ii > 1 then
				c_file = c_file .. ", "
			end
			c_file = c_file .. "\""..name.."\""
		end
	end
	reg_c("\n};")
	
	local file = io.open("flipt_opcodes.c", "w")
	file:write(c_file)
	file:close()
end
