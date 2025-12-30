# Contribution Guidelines & Notes to Myself

Even though I do not think anyone would contribute to my non-significant project, I am writing this file to have a development guideline for myself. This guideline includes:

* Overall `structure of the framework` and `layers` of it.
* `Module design` and general principles that they are following.
* `Documentation` formats and how they should be written.
* `Testing` systems and writing `commit messages`.

## Core Principles

*  **Modularity:** The framework is built on a strict layered architecture. Functionality is separated into independent modules with clear responsibilities. The user can pick and choose which modules to include in their application. And that means maybe using another framework for rendering while using this framework for input and context management.
*  **Freedom:** As said, framework is built to provide modular design. This design do not force user to use any specific design pattern or architecture. User can build their application as they want while using the framework.
*  **Explicitness:** I try to keep everything explicit. Providing consistent namings and design, understandable documentation and clear function responsibilities.

## Project Layers

The framework's stability relies on a strict, one-way dependency rule. The layers are defined as follows:

0.   **RJGlobal:** The `RJGlobal` module. It includes all global definitions and utilities that every other header file in the framework must include.
1.   **Utilities:** Absolute utilities like `String`, `Maths` and `Data Structures`.
2.   **Tools:** Tools and wrappers that are abstraction layers over standard or third party libraries, like `Resources` and `Context`.
3.   **Systems:** Modules that are using wrappers to provide higher-level functionality like `Renderer`, `Physics`, and `Input`.

No system in a layer may depend on a system in a higher or same layer.

## Folder Structure

Layers explained above are also represented in the folder structure of the project:

```
.
├── dependencies
│   └── ...
├── include
│   ├── systems
│   ├── tools
│   ├── utilities
│   └── RJGlobal.h
└── src
    ├── systems
    ├── tools
    ├── utilities
    └── RJGlobal.c
```

*   All elements (function, struct, macro, extern variable) for a module must be declared in its corresponding header file in correct folder.
*   All elements declared in headers must have their documentation. Documentation guidelines are explained in the "Documentation Formats" section below.
*   All functions in header files must be defined in their corresponding source file under src folder.
*   Structs and typedefs must be defined in header files. No struct or typedef definitions in source files unless they are internal and not exposed to user.

## Naming Conventions

There are modules that are providing framework elements to the user, elements under a module must be prefixed with the module name to avoid name collisions and ease of use.

*   **Types:** `PascalCase` - Types also must be prefixed with the module name like `RendererScene`, `InputKeyCode` and `PhysicsComponent`.
*   **Functions:** `Prefix_PascalCase` - Name of the module or type to indicate must be prefixed to the function name like `RendererScene_CreateBatch`, `Context_Initialize` and `RJ_GetExecutablePath`. Macro functions must also follow this rule.
*   **Enum Members:** `Prefix_PascalCase` - The enum type must be prefixed to the enum value like `InputKeyCode_F`, `InputState_Down` and `InputMouseMode_Hidden`.
*   **Variables:** `camelCase` - Local variables like `window` and `deltaTime`.
*   **Constants & Internals:** `ALL_CAPS_SNAKE_CASE` - Constant variables or global values/functions not exposed to user like `RJ_BUILD_DEBUG` and `RJ_DEBUG_FILE`.

## Module / File Management

*  Each module that requires initialization must have an `Initialize` function.
*  If a module requires cleanup, it must have a `Terminate` function that frees all allocated resources.
*  Inside of each module source file, sections must be wrapped with `#pragma region` and `#pragma endregion` for better readability.
*  If there are any typedefs in a module header file, they must be inside `Typedefs` region.
*  If there are any internal elements in a module source file, this section must be inside `Source Only` region.

## Object Lifecycle Management

*   Any object in the framework that requires an initialization must have a `Create` function.
*   If an object requires cleanup, it must have a `Destroy` function that frees all allocated resources for that object.
*   If an object depends on another object, the dependent object must not outlive the object it depends on. That means if you need a `RendererScene` to create a `RendererBatch`, `Create` function must be handled by `RendererScene`. Also `RendererScene` object must be destroyed after the `RendererBatch`.

## Error Handling

Currently there are no standard error handling systems in the framework. There are some functions that return invalid value on failure, but other than that, if an error occurs, the application terminates with the appropriate exit message.

## Documentation Guidelines

Currently there are no documentation for entire files. But all public elements in header files must have documentation blocks explaining their purpose and usage. Internal elements which are not exposed to the user does not required to have documentation, but it is encouraged. Documentation must follow these formats:

### Functions

Documentation blocks for functions looks like this:

``` C
/// @brief Compares two String objects by subtracting their character arrays.
/// @param string View of the first String object.
/// @param other View of the second String object.
/// @return Zero if they are equal, negative if string < other, positive if string > other.
/// @note Comparison is done up to the length of the shorter string.
int String_Compare(StringView string, StringView other);

/// @brief Create view from string literal.
/// @param stringLiteral The literal string to create a view of.
#define scl(stringLiteral) \
    (StringView) { .characters = stringLiteral, .length = RJ_Size(stringLiteral) }
```

`@note` tag is not mandatory but if the developer thinks there is something extra to say about the function, it can be added. Other than that, every parameter must be documented with `@param` tag and the return value must be documented with `@return` tag. Because macro functions do not have return types, they do not need `@return` tag.

### Others

Other elements like types, enums and macros (if not explicit like `Vector3_One`) must have a brief documentation block explaining their purpose:

``` C
/// @brief The resize multiplier used when the ListArray size reached to the capacity when adding new item
#define LIST_ARRAY_RESIZE_MULTIPLIER 2.0f

/// @brief Standard view string for entire project. Used in parameters to indicate the function is not changing the string data and for other reasons. Does not owns the memory, just points it like the regular String.
typedef struct StringView
{
    const char *characters;
    RJ_Size length;
} StringView;

/// @brief The debug log file pointer for the application.
FILE *RJ_DEBUG_FILE = NULL;
```

## Git Workflow

### Branching

All new features, bugfixes, or refactors must be done on a separate branch. It is not ok to commit directly to `main` branch:

*   **feature/<feature>** - `feature/text-rendering`
*   **fix/<bug>** - `fix/physics-collision-bug`
*   **refactor/<area>** - `refactor/material-system`
*   **docs/<area>** - `docs/update-readme`

### Commit Messages 

Commit messages must follow the format:

```
-General info for the change

--Sub info if needed

---...

-Other changes if any
...
```

## For development
* Sanitizers may be enabled by adding commands "-DENABLE_ASAN=ON -DENABLE_UBSAN=ON" to the configuration command .

* If running on Windows with sanitizers, you will need to copy sanitizer runtime to the .exe directory. That command might work for you (change the name, architecture and paths, run after build):
``` powershell
$asanDll = Get-ChildItem -Path "path\to\LLVM" -Recurse -Filter "clang_rt.asan_dynamic-x86_64.dll" | Select-Object -First 1
Copy-Item $asanDll.FullName -Destination "path\to\exe"
```

## After development

Before the merge to main branch, make sure:

*   All new public elements have documentation blocks.
*   All new modules follow the folder structure and naming conventions.
*   All new code is compiling without any errors with all of the compilers.
*   All tests are passing and application is running without errors.
*   Commit messages are following the format.