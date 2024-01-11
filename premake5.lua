-- TODO: put command line arguments to folders based on category

--------------------------------------------------------------------------------
-- COMMAND LINE ARGUMENTS
--------------------------------------------------------------------------------

newoption{
    trigger = "vpaths",
    value   = "MODE",
    description = "Virtual path handling mode for library files",
    default = "separate",
    allowed = {
       { "disable",   "No virtual paths" },
       { "separate",  "Separate include files from source files" },
       { "combined",  "Combine include and source files into the same folder" }
    }
}

newoption{
    trigger     = "lib_include_depth",
    value       = "DEPTH",
    description = "Depth at which library include files are included",
    default     = "2"
}

newoption{
    trigger     = "install_binaries",
    description = "Install binary files for libraries",
}

-- Whether we should generate a command line args JSON file or not
-- Used in conjunction with the following VS plugin:
-- https://marketplace.visualstudio.com/items?itemName=MBulli.SmartCommandlineArguments
newoption{
    trigger     = "disable_cmdargs",
    description = "Disables the generation of command line argument definitions",
}

newoption{
    trigger     = "disable_libraries",
    value       = "LIBS",
    description = "Comma separated list of libraries to disable",
    default     = "CUDA"
}

--------------------------------------------------------------------------------
-- REQUIRED PACKAGES
--------------------------------------------------------------------------------

json = require "Source/Build/json"
uuid = require "Source/Build/uuid"
md5 = require "Source/Build/md5"

--------------------------------------------------------------------------------
-- GLOBAL SETTINGS
--------------------------------------------------------------------------------

-- Name of the workspace
local WORKSPACE_NAME = "VisSimFramework"

-- Name of the project
local PROJECT_NAME = "VisSimFramework"

-- source file folder
local SOURCE_FOLDER = "Source"

-- folder where lib files are located
local LIB_FOLDER = "Libraries"

-- folder for build files
local BUILD_FILES_FOLDER = "Build"

-- folder for built binary files
local BINARY_FOLDER = "Build/Binaries/%{cfg.buildcfg}"

-- folder for installed binary files
local INSTALL_FOLDER = "Binaries/%{cfg.buildcfg}"

-- folder for assets
local ASSETS_FOLDER = "Assets"

--------------------------------------------------------------------------------
-- WORKSPACE DEFINITION
--------------------------------------------------------------------------------

workspace(WORKSPACE_NAME)
    -- folder for generated files
    generated_folders = {}
    generated_folders["Debug"] = path.join(BUILD_FILES_FOLDER, "Generated/Debug")
    generated_folders["DebugOptimized"] = path.join(BUILD_FILES_FOLDER, "Generated/DebugOptimized")
    generated_folders["Development"] = path.join(BUILD_FILES_FOLDER, "Generated/Development")
    generated_folders["Release"] = path.join(BUILD_FILES_FOLDER, "Generated/Release")

    -- clear any existing generated file
    prev_generated_files = os.matchfiles(path.join(BUILD_FILES_FOLDER, "Generated/**/*.*"))

    for _, filename in pairs(prev_generated_files) do
        os.remove(filename)
    end

    -- open the generated files
    generated_files = {}
    
    function create_generated_header(filename)
        result = {}
        for config, generated_folder in pairs(generated_folders) do
            result[config] = io.open(path.join(generated_folder, filename), "w")
            result[config]:write("#pragma once\n\n")
        end
        return result
    end

    generated_files["settings"] = create_generated_header("settings.h")
    generated_files["component_ids"] = create_generated_header("component_ids.h")
    generated_files["component_includes"] = create_generated_header("component_includes.h")

    -- set the workspace locations
    location(BUILD_FILES_FOLDER)

    -- add the platforms and configurations
    configurations({ "Debug", "DebugOptimized", "Development", "Release" })
    platforms({"Windows64"})
    
    -- set up some common defines
    defines("_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
    defines("_HAS_EXCEPTIONS=1")

    -- set some of the common flags flags
    cppdialect("C++17")
    flags("MultiProcessorCompile")
    clr("Off")
    rtti("On")
    exceptionhandling("On")
    buildoptions({ "/Zc:preprocessor", "/openmp", "/bigobj", "/cgthreads8", "/permissive-", "/Zc:twoPhase-", "/Zc:inline" })

    -- configure the configurations
    filter("configurations:Debug")
        symbols("On")
        optimize("Debug")
        runtime("Debug")
        ignoredefaultlibraries{ "libcmt" }
        warnings("Default")
        inlining("Disabled")
        buildoptions()
        defines({ "CONFIG_DEBUG", "CONFIG=\"Debug\"", "_DEBUG" })
    filter("configurations:DebugOptimized")
        symbols("On")
        optimize("Debug")
        runtime("Release")
        ignoredefaultlibraries{ "libcmt" }
        warnings("Default")
        inlining("Explicit")
        defines({ "CONFIG_DEBUG_OPTIMIZED", "CONFIG=\"DebugOptimized\"" })
    filter("configurations:Development")
        optimize("Debug")
        omitframepointer("On")
        runtime("Release")
        ignoredefaultlibraries{ "libcmt" }
        warnings("Off")
        inlining("Auto")
        linkoptions({ "/DEBUG:FASTLINK" })
        defines({ "CONFIG_RELEASE", "CONFIG=\"Development\"", "NDEBUG" })
    filter("configurations:Release")
        optimize("Full")
        flags("LinkTimeOptimization")
        omitframepointer("On")
        runtime("Release")
        ignoredefaultlibraries{ "libcmt" }
        warnings("Off")
        inlining("Auto")
        defines({ "CONFIG_RELEASE", "CONFIG=\"Release\"", "NDEBUG" })
    filter({})

    -- configure the platforms
    filter("platforms:Windows64")
        system("windows")
        systemversion("latest")
        architecture("x86_64")
        toolset("msc")
        vectorextensions("AVX2")
    filter({})
    
    -- list with all the include directories
    include_folders = {}

    -- set up the source dir as an include folder for the PCH to work properly
    includedirs(SOURCE_FOLDER)
    include_folders[#include_folders + 1] = SOURCE_FOLDER

    -- append as include folders the various shader folders
    shader_folder_names = { "OpenGL" }

    for i, shader_folder_name in pairs(shader_folder_names) do
        shader_folder = path.join(path.join(ASSETS_FOLDER, "Shaders"), shader_folder_name)
        includedirs(shader_folder)
        include_folders[#include_folders + 1] = shader_folder
    end
    
    -- set the precompiled header files
    pchheader("PCH.h")
    pchsource(SOURCE_FOLDER .. "/PCH.cpp")

    -- Create a folder for the generated files
    for config, generated_folder in pairs(generated_folders) do
        filter("configurations:" .. config .. "*")
            includedirs(generated_folder)
        filter({})
    end

    -- the main project
    project(PROJECT_NAME)
        -- Build and install folders
        local build_folder = path.getabsolute(BINARY_FOLDER)
        local install_folder = path.getabsolute(INSTALL_FOLDER)

        -- Output location for the build files
        location(BUILD_FILES_FOLDER)

        -- Type of project
        kind("ConsoleApp")
        
        -- set the binary folder
        targetdir(BINARY_FOLDER)

        -- set the install command to copy over the built binary
        if _OPTIONS["install_binaries"] ~= nil then
            local installcmd = "copy " .. 
                "\"" .. path.join(build_folder, PROJECT_NAME .. ".exe") .. "\" " .. 
                "\"" .. path.join(install_folder, PROJECT_NAME .. ".exe") .. "\""
    
            postbuildcommands(path.translate(installcmd))
        end

        -- set the debug folder to the main project folder
        debugdir(path.getabsolute("."))

        library_def_env = {}
        library_def_env["options"] = _OPTIONS
        libraries = {}
        for _, library_file in pairs(os.matchfiles(path.join(LIB_FOLDER, "**/premake5.lua"))) do
            library_module = dofile(path.getabsolute(library_file))
            if library_module["library_configure"] ~= nil then
                library_module.library_configure()
            end
            library_def = library_module.library_definition(library_def_env)
            libraries[#libraries + 1] = library_def
        end  
        --print(table.tostring(libraries, 3))

        -- make a lookup table for the list of disabled libraries
        disabled_lib_names = string.explode(_OPTIONS["disable_libraries"], ",")
        disabled_libraries = {}
        for _, lib in pairs(disabled_lib_names) do
            disabled_libraries[lib:lower()] = true
        end

        -- root folders for external libraries
        library_roots = {}
        dll_folders = {}
        libraries_present = {}

        print('Enumerating libraries...')

        -- link to the referenced libraries
        for _, library in ipairs(libraries) do
            -- various paths
            local lib_folder_relative = path.join(LIB_FOLDER, library.name)
            local lib_folder = path.getabsolute(path.join(LIB_FOLDER, library.name))
            local lib_include = path.join(lib_folder, "Include")
            local lib_include_relative = path.join(lib_folder_relative, "Include")
            local lib_source = path.join(lib_folder, "Source")
            local lib_libs = path.join(lib_folder, "Libraries")
            local lib_bins = path.join(lib_folder, "Libraries")
            
            io.write('  - ' .. library.name .. ': ')

            -- make sure the library is not marked to be disabled
            if disabled_libraries[library.name:lower()] ~= nil then
                libraries_present[library.name] = false
                io.write('disabled\n')
                goto skip_library
            else
                io.write('enabled')
            end

            -- handle external libraries
            if library.paths ~= nil then
                library_roots[library.name] = library.paths.root
                lib_include = library.paths.includes
                lib_libs = library.paths.libraries
                lib_bins = library.paths.binaries
                lib_include_relative = nil

                -- make sure the library exists
                if next(os.matchdirs(lib_include)) == nil then
                    libraries_present[library.name] = false
                    goto skip_library
                end
            end

            local function handle_variant(folder, variant)
                variant_dir = path.join(folder, variant)
                if next(os.matchdirs(variant_dir)) ~= nil then
                    return variant_dir
                else
                    return folder
                end
            end

            -- handle library variants
            if library.variant ~= nil then
                io.write(' [variant: ' .. library.variant .. ']')
                lib_include = handle_variant(lib_include, library.variant)
                lib_include_relative = handle_variant(lib_include_relative, library.variant)
                lib_source = handle_variant(lib_source, library.variant)
                lib_libs = handle_variant(lib_libs, library.variant)
                lib_bins = handle_variant(lib_bins, library.variant)
            end

            -- turn the binary folders to a table
            if (type(lib_bins) == "string") then
                lib_bins = { lib_bins }
            end

            -- mark the library as present
            libraries_present[library.name] = true

            -- apply any filters
            if library.config then
                filter("configurations:" .. library.config .. "*")
            end
            
            -- setup the includes
            includedirs(lib_include)
            if lib_include_relative ~= nil then
                include_folders[#include_folders + 1] = lib_include_relative
            end
            
            -- add the header files
            for include_level = 1, tonumber(_OPTIONS["lib_include_depth"]), 1 do
                patterns = { "*.h", "*.hpp" }
                for level = 2, include_level, 1 do
                    for id, pattern in pairs(patterns) do
                        patterns[id] = "*/" .. patterns[id]
                    end
                end
                for id, pattern in pairs(patterns) do
                    files(path.join(lib_include, pattern))
                end
            end

            -- add the cpp files in case of a source distributed library
            files(path.join(lib_source, "*.*"))

            -- Append the PCH source to the start of every cpp file
            local function append_pch_include(extension)
                lib_source_files = os.matchfiles(path.join(lib_source, "*" .. extension))
                for _, src_file_path in pairs(lib_source_files) do
                    src_file = io.open(src_file_path, "r")

                    first_line = src_file:lines()()
                    if first_line ~= "#include \"PCH.h\"" then
                        contents = src_file:read("*a")
                        src_file:close()

                        src_file = io.open(src_file_path, "w")
                        src_file:write("#include \"PCH.h\"\n\n")
                        src_file:write(first_line)
                        src_file:write("\n")
                        src_file:write(contents)
                    end
                    src_file:close()
                end
            end
            append_pch_include(".cpp")
            append_pch_include(".cc")
            
            -- link to the libraries, in case it is binary distributed
            libdirs(lib_libs)
            for libFilter, libList in pairs(library.libs) do
                filter(libFilter)
                    links(libList)
                filter({})
            end

            -- Helper function to generate the various copy commands
            local function copyLibFiles(libFolder, binFolder, libFiles, val, ext)
                -- Helper function for actually generating the copy commands
                local function generateCommands(libFolder, binFolder, filter, ext)
                    filePaths = os.matchfiles(path.join(libFolder, filter .. ext))
                    -- if libFiles ~= "*" and next(filePaths) == nil then
                    --     premake.error("No file matching the pattern '" .. filter .. ext .. "' found!")
                    -- end
                    for _, lib_files in pairs(filePaths) do
                        local lib_file_path = path.getrelative(libFolder, lib_files)
                        local copy_cmd = "copy \"" .. path.join(libFolder, lib_file_path) .. 
                            "\" \"" .. path.join(binFolder, lib_file_path) .. "\""
                        local echo_cmd = "echo \"Copying " .. path.join(libFolder, lib_file_path) .. "...\""
                        postbuildcommands(echo_cmd)
                        postbuildcommands(path.translate(copy_cmd))
                    end
                end

                if val == "none" then
                    -- noop
                elseif val == "all" then
                    generateCommands(libFolder, binFolder, "*", ext)
                elseif val == nil or val == "lib" then
                    for libFilter, libList in pairs(libFiles) do
                        filter(libFilter)
                            if type(libList) == "table" then
                                for i, libFile in pairs(libList) do
                                    generateCommands(libFolder, binFolder, libFile, ext)
                                end
                            else
                                generateCommands(libFolder, binFolder, libList, ext)
                            end
                        filter({})
                    end
                else
                    for libFilter, libList in pairs(val) do
                        filter(libFilter)
                            if type(libList) == "table" then
                                for i, libFile in pairs(libList) do
                                    generateCommands(libFolder, binFolder, libFile, ext)
                                end
                            else
                                generateCommands(libFolder, binFolder, libList, ext)
                            end
                        filter({})
                    end
                end
            end

            for _, binary_dir in pairs(lib_bins) do
                -- copy the dll's into the binary folder upon build
                -- copyLibFiles(binary_dir, build_folder, library.libs, library.dlls, ".dll")
                copyLibFiles(binary_dir, build_folder, library.libs, library.inis, ".ini")
                if _OPTIONS["install_binaries"] ~= nil then
                    copyLibFiles(binary_dir, install_folder, library.libs, library.dlls, ".dll")
                    copyLibFiles(binary_dir, install_folder, library.libs, library.inis, ".ini")
                end

                -- append the binary folder for .DLL files
                dll_folders[#dll_folders + 1] = binary_dir
            end
            
            if _OPTIONS["vpaths"] == "separate" then
                vpaths
                ({
                    ["Libraries/Include/" .. library.name .. "/*"] = { path.join(lib_include, "**") },
                    ["Libraries/Source/" .. library.name .. "/*"] = { path.join(lib_source, "**") },
                })
            end
            
            -- jump to new line after all the info has been shown
            io.write('\n')

            -- reset the filter
            filter({})
            ::skip_library::
        end

        -- generate defines for each present/missing library
        libraries_present_string = ""
        for lib, present in pairs(libraries_present) do
            if present then
                libraries_present_string = libraries_present_string .. "#define HAS_" .. lib .. " 1\n"
            else
                libraries_present_string = libraries_present_string .. "#define NO_" .. lib .. " 1\n"
            end
        end

        -- append the list of include dirs to the settings file
        for config, generated_folder in pairs(generated_folders) do
            generated_files["settings"][config]:write(libraries_present_string)
            generated_files["settings"][config]:write("\n\n")
        end

        -- generate a define with all the include dirs
        include_dirs_string = "#define INCLUDE_FOLDERS\\\n"
        for i, folder in pairs(include_folders) do
            include_dirs_string = include_dirs_string .. "    \"" .. folder .. "\", \\\n"
        end

        -- append the list of include dirs to the settings file
        for config, generated_folder in pairs(generated_folders) do
            generated_files["settings"][config]:write(include_dirs_string)
            generated_files["settings"][config]:write("\n\n")
        end

        -- generate unique component ids
        print('Enumerating components...')
        components_folder = path.join(path.join(SOURCE_FOLDER, "Scene"), "Components")
        component_files = os.matchfiles(path.join(components_folder, "**/*.cpp"))
        component_id = 0;
        for _, filename in pairs(component_files) do
            for line in io.lines(filename) do
                match_begin, match_end, component_name = line:find('DEFINE_COMPONENT%((%S*)%)')
                if (match_begin) then
                    print('  - ' .. component_name)
                    component_source = filename
                    component_header = path.getrelative(SOURCE_FOLDER, component_source)
                    component_header = component_header:gsub(".cpp", ".h")

                    for config, generated_folder in pairs(generated_folders) do
                        generated_files["component_ids"][config]:write("#define __COMPONENT_ID_")
                        generated_files["component_ids"][config]:write(component_name)
                        generated_files["component_ids"][config]:write(" ")
                        generated_files["component_ids"][config]:write(component_id)
                        generated_files["component_ids"][config]:write("\n")

                        generated_files["component_includes"][config]:write("#include \"")
                        generated_files["component_includes"][config]:write(component_header)
                        generated_files["component_includes"][config]:write("\"\n")
                    end
                    component_id = component_id + 1
                end
            end
        end
        
        -- append the source files
        files("premake5.lua")
        files(path.join(SOURCE_FOLDER, "*.*"))
        files(path.join(SOURCE_FOLDER, "**/*.*"))
        files(path.join(path.join(ASSETS_FOLDER, "Shaders"), "**/*.*"))
        
        for config, generated_folder in pairs(generated_folders) do
            filter("configurations:" .. config .. "*")
                files(path.join(generated_folder, "*.*"))
            filter({})
        end

        -- setup some virtual paths
        vpaths
        ({
            ["Source/*"] = { "premake5.lua", path.join(SOURCE_FOLDER, "*.*"), path.join(SOURCE_FOLDER, "**/*.*") },
            ["Shaders/*"] = { path.join(path.join(ASSETS_FOLDER, "Shaders"), "*.*"), path.join(path.join(ASSETS_FOLDER, "Shaders"), "**/*.*") }
        })
            
        if _OPTIONS["vpaths"] ~= "disable" then
            if _OPTIONS["vpaths"] == "combined" then
                vpaths
                ({
                    ["Libraries/*"] = { path.join(LIB_FOLDER, "**") },
                })
                for name, folder in pairs(library_roots) do
                    vpaths
                    ({
                        ["Libraries/" .. name .. "/*"] = { path.join(folder, "**") },
                    })
                end
            end

            for config, generated_folder in pairs(generated_folders) do
                filter("configurations:" .. config .. "*")
                    vpaths
                    ({
                        ["Source/Generated/" .. config .. "/"] = { path.join(generated_folder, "**") },
                    })
                filter({})
            end
        end

        -- generate path environment vars
        path_env = "%PATH%"
        for _, folder in pairs(dll_folders) do
            if folder:sub(-1) ~= "/" then
                folder = folder .. "/"
            end
            path_env = folder .. ";" .. path_env
        end
        path_env = "PATH=" .. path_env

        debugenvs(path_env)
        --print(path_env)

        if _OPTIONS["disable_cmdargs"] == nil then
            -- Helper function to generate unique ids
            local function make_cmd_id()
                return uuid("00:0c:29:69:41:c6")
            end
            
            -- Helper function to make a command line arg folder
            local function make_folder(folder_name, children)
                return 
                {
                    ["Id"] = make_cmd_id(),
                    ["Command"] = folder_name,
                    ["Items"] = children
                }
            end
            
            -- Helper function to make a command line arg
            local function make_arg(arg_value)
                return 
                {
                    ["Id"] = make_cmd_id(),
                    ["Command"] = arg_value
                }
            end
            
            -- Helper function to make a command line arg lists
            local function make_args(arg_name, arg_values)
                arg_list = {}
                for _, val in pairs(arg_values) do
                    arg_list[#arg_list + 1] = make_arg(arg_name .. " " .. val)
                end
                return arg_list
            end

            print('Building command line argument list...')

            -- extract all command line arguments
            source_files = os.matchfiles(path.join(SOURCE_FOLDER, "**/*.cpp"))
            config_vars_ = {}
            for _, filename in pairs(source_files) do
                for line in io.lines(filename) do
                    match_begin, match_end, var_def = line:find('@CONSOLE_VAR%(([%w_%- ,%.]*)')
                    if (match_begin) then
                        var_values = string.explode(var_def, ", ")
                        var_group, var_name, var_regex = var_values[1], var_values[2], var_values[3]
                        --print(filename, var_group, var_name, var_regex)
                        
                        if config_vars_[var_group] == nil then
                            config_vars_[var_group] = {}
                        end
                        if config_vars_[var_group][var_name] == nil then
                            config_vars_[var_group][var_name] = {}
                        end
                        config_vars_[var_group][var_name]["var_regex"] = var_regex
                        if config_vars_[var_group][var_name]["var_values"] == nil then
                            config_vars_[var_group][var_name]["var_values"] = {}
                        end
                        for i = 4, #var_values do
                            config_vars_[var_group][var_name]["var_values"][#config_vars_[var_group][var_name]["var_values"] + 1] = var_values[i]
                        end
                    end
                end
            end
    
            -- build a sorted array from the config vars
            config_var_groups = {}
            for group_name, group in pairs(config_vars_) do
                group_vars = {}
                for var_name, var in pairs(group) do
                    group_vars[#group_vars + 1] = {
                        var_group = group_name,
                        var_name = var_name,
                        var_regex = var["var_regex"],
                        var_values = var["var_values"]
                    }
                end
                config_var_groups[#config_var_groups + 1] = group_vars
            end
            table.sort(config_var_groups, function(a,b) return a[1]["var_group"] < b[1]["var_group"] end)
            for _, cvar in ipairs(config_var_groups) do
                table.sort(cvar, function(a,b) return a["var_name"] < b["var_name"] end)
            end
            --print(table.tostring(config_var_groups, 3))
    
            -- build command line argument value list
            function build_cvar_values(cvar)
                values = '{ '
                for id, val in ipairs(cvar["var_values"]) do
                    if id > 1 then
                        values = values .. ', '
                    end
                    values = values .. val
                end
                values = values .. ' }'
                return values
            end

            -- build the cmd line argument definitions from the list
            command_line_arguments = {}
            args_md5 = md5.new()
            for _, cvar_group in ipairs(config_var_groups) do
                group_variables = {}
                for _, cvar in ipairs(cvar_group) do
                    print(string.format('- %s - %s: %s', cvar["var_group"], cvar["var_name"], build_cvar_values(cvar)))
                    group_variables[#group_variables + 1] = make_folder(cvar["var_name"], make_args(cvar["var_regex"], cvar["var_values"]))
                    args_md5:update(cvar["var_group"])
                    args_md5:update(cvar["var_name"])
                    args_md5:update(cvar["var_regex"])
                    args_md5:update(table.tostring(cvar["var_values"]))
                end
                
                command_line_arguments[#command_line_arguments + 1] = make_folder(cvar_group[1]["var_group"], group_variables)
            end
            cmd_args_file_version = md5.tohex(args_md5:finish())
    
            --print(table.tostring(command_line_arguments, 3))
            --print(table.tostring(command_line_arguments, 3))
            --print(cmd_args_file_version)

            -- Version of the current command line
            cmd_args_file_version_header = "_MD5 "
    
            -- Read the current contents
            cmd_args_current_version = 0
            cmd_args_file = io.open(path.join(BUILD_FILES_FOLDER, WORKSPACE_NAME .. ".args.json"), "r")
            if cmd_args_file ~= nil then
                cmd_args_file_contents = json.decode(cmd_args_file:read("*a"))
                cmd_args_file:close()
    
                -- Extract the current file version
                cmd_args_current_version = string.match(cmd_args_file_contents["Items"][1]["Command"], cmd_args_file_version_header .. "(%S+)")
            end
            --print(cmd_args_current_version)
            --print(cmd_args_file_version)

            -- Generate the command line arguments file
            cmd_args = {}
            cmd_args["FileVersion"] = 2
            cmd_args["Id"] = make_cmd_id()
            cmd_args["Items"] = 
            {
                make_arg(cmd_args_file_version_header .. cmd_args_file_version)
            }
            for _, arg in pairs(command_line_arguments) do
                cmd_args["Items"][#cmd_args["Items"] + 1] = arg
            end

            -- Only override if it has different contents; otherwise
            if (cmd_args_current_version ~= cmd_args_file_version) then
                print("Updating command line .args.json...")

                cmd_args_file = io.open(path.join(BUILD_FILES_FOLDER, WORKSPACE_NAME .. ".args.json"), "w")
                cmd_args_file:write(json.encode(cmd_args))
                cmd_args_file:close()
            end
            
        end