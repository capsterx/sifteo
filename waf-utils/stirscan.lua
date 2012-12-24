function print_it(arg)
  if type(arg) == "string" then
    print(arg)
  else
    for key,value in pairs(arg) 
    do 
      print(value) 
    end
  end
end

function image(arg) print_it(arg[1]) end
function group(arg) end
function tracker(arg) print_it(arg[1]) end
function sound(arg) print_it(arg[1]) end

loadfile(arg[1])()
