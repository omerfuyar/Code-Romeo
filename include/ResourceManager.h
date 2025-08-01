#pragma once

#include "raylib.h"

/// <summary>
/// Looks for the specified resource dir in several common locations
/// The working dir
/// The app dir
/// Up to 3 levels above the app dir
/// When found the dir will be set as the working dir so that assets can be loaded relative to that.
/// </summary>
/// <param name="folderName">The name of the resources dir to look for</param>
/// <returns>True if a dir with the name was found, false if no change was made to the working dir</returns>
inline static bool SearchAndSetResourceDir(const char *folderName)
{
    // check the working dir
    if (DirectoryExists(folderName))
    {
        ChangeDirectory(TextFormat("%s/%s", GetWorkingDirectory(), folderName));
        return true;
    }

    const char *appDir = GetApplicationDirectory();

    // check the applicationDir
    const char *dir = TextFormat("%s%s", appDir, folderName);
    if (DirectoryExists(dir))
    {
        ChangeDirectory(dir);
        return true;
    }

    // check one up from the app dir
    dir = TextFormat("%s../%s", appDir, folderName);
    if (DirectoryExists(dir))
    {
        ChangeDirectory(dir);
        return true;
    }

    // check two up from the app dir
    dir = TextFormat("%s../../%s", appDir, folderName);
    if (DirectoryExists(dir))
    {
        ChangeDirectory(dir);
        return true;
    }

    // check three up from the app dir
    dir = TextFormat("%s../../../%s", appDir, folderName);
    if (DirectoryExists(dir))
    {
        ChangeDirectory(dir);
        return true;
    }

    return false;
}