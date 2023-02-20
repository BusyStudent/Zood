add_rules("mode.debug", "mode.release")

set_optimize("smallest")

-- Import UI ToolKit
includes("./external/btk")

target("zood")
    if is_mode("release") then 
        set_strip("all")
    end

    set_kind("binary")
    set_languages("c++17")

    add_files("./src/*.cpp")

    add_includedirs("./external/btk/include")
    add_deps("btk")