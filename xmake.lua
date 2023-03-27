add_rules("mode.debug", "mode.release")

-- set_optimize("smallest")
-- set_exceptions("no-cxx")
-- set_warnings("all")
-- add_defines("BTK_NO_EXCEPTIONS")

-- Import CPR
add_requires("nlohmann_json", "libxml2")
add_packages("nlohmann_json", "libxml2")

-- We need zlib to process defalte
add_requires("cpr", {configs = {ssl = true} })
add_packages("cpr", {configs = {ssl = true} })

-- Import UI ToolKit
includes("./external/btk")

target("zood")
    if is_mode("release") then 
        set_strip("all")
    end

    set_kind("binary")
    set_languages("c++17")

    add_files("./src/*.cpp")
    add_cxxflags("cl::/utf-8")

    add_includedirs("./external/btk/include")
    add_deps("btk", "btk_multimedia")