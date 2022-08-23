set_languages("cxx17")
if is_os("windows") then
    add_cxxflags("/utf-8")
end
add_rules("mode.debug")
add_rules("mode.release")
if is_mode("debug") then
    set_warnings("all")
end

includes("ext")
includes("D3DRender")


	
    
