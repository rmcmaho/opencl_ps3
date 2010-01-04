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


-- Returns kernel name and number of parameters
function getNextKernel()
   
   local file = FILE

   while true do
      local line = file:read("*line")
      if line == nil then break end

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

	 while endParam ~= nil
	 do
	    table.insert(param_table, trim(string.sub(line, beginParam, endParam-1)))
	    paramCount = paramCount + 1
	    beginParam = endParam+1
	    endParam = line:find(",",beginParam)
	 end --End little while

	 endParam = line:find(")", beginParam)
	 table.insert(param_table, trim(string.sub(line, beginParam, endParam-1)))

	 return funcName, paramCount
	    
      end --End big if

   end --End while

   --end of file reached
   return nil, nil

end


--Get kernel name from line and trim white space
function getKernelName(s)

   local begin, finish = s:find("%s+%w+%s-%(")
   
   local funcName = s:sub(begin, finish-1)
   
   funcName = trim(funcName)
   return funcName

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