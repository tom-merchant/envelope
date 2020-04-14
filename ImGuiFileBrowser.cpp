#include "ImGuiFileBrowser.h"

#include <regex>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <cstdio>

#include <SDL2/SDL.h>
#include <GL/gl.h>

std::map<std::string, ImTextureID> _FileBrowserDefaultIconMap;

std::map<std::string, ImTextureID> sdlImpl_constructDefaultIconMap ( float iconSize, float dpi )
{
    if ( _FileBrowserDefaultIconMap.size ( ) > 0 )
    {
        return _FileBrowserDefaultIconMap;
    }

    int i, NUMICONS = 6, iconSizePixels = iconSize * dpi;

    std::string iconExts[] = { "default", "folder", "txt|rtf|pdf|doc|docx|odt|docm|",
                               "mp3|ogg|midi|wav|aiff|flac|m4a|opus|wma|3gp|au|aac",
                               "webm|mkv|flv|ogv|webm|avi|mov|wmv|yuv|mp4|m4p|m4v|mpg|3g2|f4v",
                               "jpg|png|gif|jpeg|svg|tiff|nef|bmp|pbm|pnm|webp|psd"},

                iconPaths[] = { "icons/file.bmp", "icons/folder.bmp", "icons/file-text.bmp", "icons/music.bmp",
                                "icons/film.bmp", "icons/image.bmp"  },
                ext;

    std::map<std::string, ImTextureID> returnMap;

    for ( i = 0; i < NUMICONS; i++ )
    {
        SDL_Surface* icon =  SDL_LoadBMP ( iconPaths [ i ].c_str ( ) );

        GLuint glTex;

        glGenTextures ( 1,  &glTex );
        glBindTexture ( GL_TEXTURE_2D, glTex );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, icon->w, icon->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, icon->pixels );

        glBindTexture(GL_TEXTURE_2D, 0);

        SDL_FreeSurface ( icon );

        std::stringstream splitstream = std::stringstream ( iconExts [ i ] );

        while ( std::getline ( splitstream, ext, '|' ) )
        {
            returnMap [ ext ] =  (ImTextureID)(intptr_t)glTex;
        }
    }

    _FileBrowserDefaultIconMap = returnMap;

    return returnMap;
}

std::map<std::string, ImTextureID> (*constructDefaultIconMap)( float, float ) = &sdlImpl_constructDefaultIconMap;


bool createFolder ( std::string path )
{
    try
    {
        std::filesystem::create_directory ( path );
        return true;
    }
    catch ( std::filesystem::filesystem_error e )
    {
        return false;
    }
}

bool createFile ( std::string path )
{
    FILE *temp = fopen ( path.c_str ( ), "w+" );

    if ( temp )
    {
        fclose ( temp );
        return true;
    }

    return false;
}

bool ImGui::Ext::FileBrowser ( ImGui::Ext::FileBrowserContext *context, const std::string startFolder )
{
    int i = 0, j = 0, iconSizePixels = context->dpi * context->iconSizeInches, nCols, col, row;
    float yAdjust = 0, maxTextHeight = 0;
    ImVec2 iconSize = ImVec2 ( iconSizePixels, iconSizePixels );
    std::stringstream pathSplitStream;
    std::string folder, workingPath;
    bool usingRegex = context->filter.length ( ) > 0, ret = false;
    const char* listItems [ 2 ] = { "folder", "file" };

    if ( ! context->_iconMapInit )
    {
        if ( context->iconMap.size () == 0 )
        {
            context->iconMap = (*constructDefaultIconMap) ( context->iconSizeInches, context->dpi );
        }

    }

    /* If the currentFolder variable hasn't been initialised then initialise it */
    if ( context->currentFolder.length() == 0 )
    {
        if ( startFolder.length() == 0 )
        {
            context->currentFolder = std::string ( std::getenv ( "HOME" ) );

            if ( context->currentFolder.length ( ) == 0 )
            {
                context->currentFolder = "/";
            }
        }
        else
        {
            context->currentFolder = startFolder;
        }
    }

    /* If there is a pattern regex and we haven't compiled it yet, then compile it */
    if ( ! context->_regex_compiled )
    {
        if ( usingRegex )
        {
            try
            {
                context->_compiled = std::regex ( context->filter );
            }
            catch ( std::regex_error e )
            {
                std::cerr << "ERROR INVALID REGEX PATTERN: " << context->filter << std::endl;
            }
            context->_regex_compiled = true;
        }
        else
        {
            context->_regex_compiled = true;
        }
    }

    /* Start drawing widget */
    ImGui::PushID ( "FileBrowser" );
    ImGui::BeginChild ( "FileBrowser", context->dimensions, true, ImGuiWindowFlags_HorizontalScrollbar );

    /* Split path string and draw buttons for each folder in the path  */
    pathSplitStream = std::stringstream ( context->currentFolder );
    workingPath = "/";

    if ( ImGui::SmallButton ( "/" ) )
    {
        context->currentFolder = "/";
    }

    while ( std::getline ( pathSplitStream, folder, '/' ) )
    {
        if ( folder.length() != 0 )
        {
            workingPath += folder + "/";

            ImGui::SameLine ( );

            /* If they click on this folder then change directory */
            if ( ImGui::SmallButton ( std::string ( folder + "##" + std::to_string ( i++ ) ).c_str ( ) ) )
            {
                context->currentFolder = workingPath;
            }
        }
    }

    ImGui::SameLine ( );

    if ( ImGui::SmallButton ( "+" ) )
    {
        ImGui::OpenPopup ( "NewFileFolder" );
    }

    if ( ImGui::BeginPopup ( "NewFileFolder" ) )
    {
        ImGui::Combo ( "type", &context->_listSelection, listItems, 2 );

        if ( ImGui::InputText ( "name", context->fileNameInput, 512, ImGuiInputTextFlags_EnterReturnsTrue )
             && ImGui::IsKeyPressed ( SDL_SCANCODE_RETURN, false ))
        {

            if ( context->_listSelection == 0 )
            {
                if ( createFolder ( workingPath + context->fileNameInput ) )
                {
                    context->currentFolder = workingPath + context->fileNameInput + "/";
                    ret = true;
                    context->fileCreated = false;
                }
            }
            else if ( createFile ( workingPath + context->fileNameInput ) )
            {
                context->selectedFile = workingPath + context->fileNameInput;
                ret = true;
                context->fileCreated = true;
            }

            std::cout << context->fileNameInput << std::endl;
            ImGui::CloseCurrentPopup ( );
        }

        ImGui::Dummy ( ImVec2 ( 1/8 * context->dpi, 1/8 * context->dpi ) );

        if ( ImGui::Button ( "cancel" ) )
        {
            ImGui::CloseCurrentPopup ( );
        }

        ImGui::SameLine ( );

        if ( ImGui::Button ( "create" ) )
        {
            if ( context->_listSelection == 0 )
            {
                if ( createFolder ( workingPath + context->fileNameInput ) )
                {
                    context->currentFolder = workingPath + context->fileNameInput + "/";
                }
            }
            else if ( createFile ( workingPath + context->fileNameInput ) )
            {
                context->selectedFile = workingPath + context->fileNameInput;

                if ( context->type == FILE )
                {
                    ret = true;
                    context->fileCreated = true;
                }
            }
            ImGui::CloseCurrentPopup ( );
        }

        ImGui::EndPopup ( );
    }

    ImGui::Separator ( );

    /* Draw folders and files in a subwindow */

    ImGui::BeginChild ( "FileIcons" );

    try{
        nCols =  ImGui::GetWindowWidth ( ) /  ( iconSizePixels * 2 );
        yAdjust = ImGui::GetCursorPosY ( );

        /* For each item in this folder */
        for ( std::filesystem::directory_entry e : std::filesystem::directory_iterator ( context->currentFolder ))
        {
            /* If it matches our filter */
            if (
                ( ( std::regex_match ( e.path ( ).filename ( ).generic_string ( ), context->_compiled ) )
                     || !usingRegex || e.is_directory () )

                && ( ! context->showHidden && e.path ( ).filename ( ).generic_string ( ).c_str( )[0] != '.'
                     || context->showHidden )

                && ( ( ( context->type == FOLDER ) && e.is_directory ( ) ) || ! (context->type == FOLDER) )
            )
            {
                /* Work out where it is in the grid */
                col = j % nCols;
                row = j / nCols;

                ImGui::PushID ( i++ + j++ );

                if ( e.is_directory ())
                {
                    /* Draw folder icon and process folder click if clicked */
                    ImGui::SetCursorPos ( ImVec2 ( col * (iconSizePixels * 2) + iconSizePixels / 2,
                            yAdjust + row * iconSizePixels ) );
                    if ( ImGui::ImageButton ( context->iconMap[ "folder" ], iconSize ))
                    {
                        context->currentFolder = e.path ( ).generic_string ( );
                        ret = true;
                        context->fileCreated = false;
                    }
                }
                else
                {
                    /* Draw file icon depending on extension and process file if clicked */

                    ImTextureID icon;
                    std::string ext = e.path ( ).filename ( ).extension ( );

                    if ( ext.length ( ) > 0 )
                    {
                        ext = e.path ( ).filename ( ).extension ( ).generic_string ( ).substr ( 1 );

                        /* Extensions in our map are lowercase so lowercase this extension */
                        std::for_each(ext.begin(), ext.end(), [](char & c)
                        {
                            c = std::tolower(c);
                        });
                    }

                    icon = context->iconMap.find ( ext ) != context->iconMap.end ( )
                            ? context->iconMap [ ext ]
                            : context->iconMap [ "default" ];

                    ImGui::SetCursorPos ( ImVec2 ( col * (iconSizePixels * 2) + iconSizePixels / 2,
                                                   yAdjust + row * iconSizePixels ) );

                    if ( ImGui::ImageButton ( icon, iconSize ) )
                    {
                        context->selectedFile = e.path ( );
                        ret = true;
                        context->fileCreated = false;
                    }
                }

                /* Draw file name */

                ImGui::SetCursorPosX ( col * (iconSizePixels * 2) + iconSizePixels / 2 );
                PushTextWrapPos ( ImGui::GetCursorPosX ( ) + 1.5 * iconSizePixels );
                ImGui::Text ( "%s", e.path ( ).filename ( ).c_str ( ) );
                PopTextWrapPos ();

                ImGui::PopID ();

                maxTextHeight = fmax ( maxTextHeight, ImGui::CalcTextSize ( e.path ( ).filename ( ).c_str ( ),
                                                                            NULL, false, 1.5 * iconSizePixels).y );

                if ( col == (nCols - 1) )
                {
                    yAdjust += maxTextHeight + 10;
                    maxTextHeight = 0;
                }
            }
        }
    }
    catch ( std::filesystem::filesystem_error )
    {
        /* probably didn't have permission
         * TODO: check each folder for permission, don't bother letting the user click on those */

        //Go back up a directory
        context->currentFolder = context->currentFolder.substr
                (0,
                context->currentFolder.substr(0, context->currentFolder.find_last_of ( '/' ) - 1).find_last_of ( '/' ));
    }

    ImGui::EndChild ( );

    /* End widget */
    ImGui::EndChild ( );
    ImGui::PopID ( );

    return ret;
}

bool ImGui::Ext::FileBrowser ( ImGui::Ext::FileBrowserContext *context )
{
    return FileBrowser ( context, "" );
}
