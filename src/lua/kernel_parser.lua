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


--[[
file = loadFile("test_kernel.c")
print(getNextKernel(file))
--]]