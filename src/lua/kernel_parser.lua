-- Kernel Parser

FILE = ""

function loadFile(filename)

   local file,error, errcode = io.open(filename, "r")
   if file == nil then
      print(error)
      return
   end
   
   FILE = file
   
end


-- Returns kernel name, number of parameters, and table of parameters
function getNextKernel()
   
   local file = FILE
   
   while true do
      local line = file:read("*line")
      
      if line == nil then 
      	 -- End of file 
     	 return nil, nil, nil
      end
      
      if line:find("__kernel") ~= nil then
	 
	 --- Check for new lines (will break if extra parentheses are used)
	 local func_end = line
	 local s = newStack()      
	 while func_end:find(")") == nil
	 do
	    addString(s, func_end)
	    func_end = file:read("*line")
	 end
	 addString(s, func_end)
	 s = table.concat(s)
	 line = s
	 ---
	 local funcName = getKernelName(line)
	 
	 -- Why does this give "unfinished capture"?
	 --local beginParam = line:find("(")
	 local begin, finish = line:find("%s+%w+%s-%(")	
	 local beginParam = finish+1
	 local endParam = line:find(",")
	 
	 local param_table = {}
	 local paramCount = 1	 
	 local full_param = ""

	 while endParam ~= nil
	 do
	    full_param = trim(string.sub(line, beginParam, endParam-1))
	    table.insert(param_table, getParamType(full_param))
	    paramCount = paramCount + 1
	    beginParam = endParam+1
	    endParam = line:find(",",beginParam)
	 end --End little while
	 
	 endParam = line:find(")", beginParam)
	 full_param = trim(string.sub(line, beginParam, endParam-1))
	 table.insert(param_table, getParamType(full_param))
	 
	 return funcName, paramCount, param_table
	 
      end -- End big if
      
   end -- End big while
   
   return nil, nil, nil 
   
end


--Get kernel name from line and trim white space
function getKernelName(s)
   
   local begin, finish = s:find("%s+%w+%s-%(")
   
   local funcName = s:sub(begin, finish-1)
   
   funcName = trim(funcName)
   return funcName
   
end

function getParamType(param)

   if string.find(param, "global%s+") ~=nil or string.find(param, "local%s+") ~= nil
   then
      local begin, finish = param:find("%s+%w+%s+")
      param = param:sub(begin, finish)
      param = trim(param)
   else
      local begin, finish = param:find("^%w+%s")
      param = param:sub(begin, finish-1)
   end

   return param

end

function trim(s)
   return (string.gsub(s, "^%s*(.-)%s*$", "%1"))
end


function newStack ()
   return {""}   -- starts with an empty string
end

function addString (stack, s)
   table.insert(stack, s)    -- push 's' into the the stack
   -- What does this loop do?
   for i=table.getn(stack)-1, 1, -1 do       
      if string.len(stack[i]) > string.len(stack[i+1]) then
	 break
      end
      stack[i] = stack[i] .. table.remove(stack)
   end
end

--[[
file = loadFile("test_kernel.c")
print(getNextKernel(file))
--]]