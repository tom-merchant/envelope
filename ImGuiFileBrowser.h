
#pragma once

#ifndef ENVELOPE_IMGUIFILEBROWSER_H
#define ENVELOPE_IMGUIFILEBROWSER_H

#include <imgui.h>
#include <map>
#include <regex>

namespace ImGui
{
    namespace Ext
    {

        typedef enum FileBrowserType
        {
            FILE,
            FOLDER
        } FileBrowserType;

        /**
         * @struct FileBrowserContext
         *
         * Holds the current context for a FileBrowser
         *
         * ALWAYS ZERO INITIALISE ( i.e new FileBrowserContext() NOT new FileBrowserContext )
         *
         * @var FileBrowserContext::type Whether (files and folders) or just folders should be shown
         * @var FileBrowserContext::filter the filter to apply to the files in the browser, uses regex
         * @var FileBrowserContext::dimensions the width and height of the FileBrowser child window
         * @var FileBrowserContext::currentFolder the current folder in view
         * @var FileBrowserContext::selectedFile the file the user has currently selected
         * @var FileBrowserContext::iconMap a mapping of file extensions to ImGui textures, supply your own if you wish
         * @var FileBrowserContext::iconSizeInches the icon size in inches
         * @var FileBrowserContext::dpi the screen dpi
         * @var FileBrowserContext::showHidden Whether to show hidden files
         * @var FileBrowserContext::filecreated Whether the return value indicates a file creation
         */
        typedef struct FileBrowserContext
        {
            FileBrowserType type = FILE;
            const std::string filter;
            ImVec2 dimensions = ImVec2( 0.0f,  400 );
            std::string currentFolder;
            std::string selectedFile;
            std::map<std::string, ImTextureID> iconMap;
            float iconSizeInches = 3.0/8;
            float dpi = 96;
            std::regex _compiled;
            bool showHidden = false;
            bool fileCreated = false;
            bool _regex_compiled = false;
            bool _iconMapInit = false;
            int _listSelection = 0;
            char fileNameInput [ 512 ];
        } FileBrowserContext;

        /**
         * Displays a FileBrowser and updates the supplied ImGui::Ext::FileBrowserContext as necessary
         *
         * @param context The current state of the file browser
         * @param startFolder The folder to start at
         * @returns true when the user has clicked on or created a file or folder
         */
        IMGUI_API bool FileBrowser ( FileBrowserContext* context, const std::string startFolder );

        /**
         * Displays a FileBrowser and updates the supplied ImGui::Ext::FileBrowserContext as necessary
         *
         * @param context The current state of the file browser
         * @returns true when the user has clicked on or created a file or folder
         */
        IMGUI_API bool FileBrowser ( FileBrowserContext* context );
    }
}

#endif