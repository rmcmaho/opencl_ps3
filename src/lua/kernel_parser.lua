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

      --- Check for new lines
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
	 local nextComma = line:find(",")

	 if nextComma == nil then
	    return funcName, 1
	 else
	    
	    local paramCount = 1
	    while nextComma ~= nil
	    do
	       paramCount = paramCount + 1
	       nextComma = line:find(",",nextComma+1)
	    end

	    return funcName, paramCount
	    
	 end --End else	 

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