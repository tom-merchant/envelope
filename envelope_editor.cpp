
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileBrowser.h"
#include "ImGuiEnvelopeEditor.h"
#include "envelope.h"

typedef enum states
{
    DEFAULT = 0,
    SAVE_AS_DIALOG_OPEN = 1,
    OPEN_DIALOG_OPEN = 2
} states;

states save_file ( std::string path, ImGui::Ext::EnvelopeEditorContext *ctx );
void   new_file  (ImGui::Ext::EnvelopeEditorContext *ctx );
void   open_file ( std::string path, ImGui::Ext::EnvelopeEditorContext *ctx );

int main ( int argc, char** argv )
{
    const char* glsl_version = "#version 130";
    std::string save_path = "";
    bool darkmode = true, running = true, saved = true, popupOpened = false;
    float dpi;
    SDL_Window* window;
    SDL_GLContext gl_context;
    SDL_WindowFlags window_flags;
    SDL_DisplayMode DM;
    SDL_Event event;
    int err;
    ImGui::Ext::FileBrowserContext fileBrowserCtx = {};
    ImGui::Ext::EnvelopeEditorContext envelopeEditorContext = {};

    states state = DEFAULT;

    if ( SDL_Init ( SDL_INIT_EVENTS | SDL_INIT_VIDEO ) != 0)
    {
        SDL_Log ("Unable to initialize SDL: %s", SDL_GetError ( ) );
    }


    SDL_GL_SetAttribute ( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG );
    SDL_GL_SetAttribute ( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
    SDL_GL_SetAttribute ( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute ( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
    SDL_GL_SetAttribute ( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_GL_SetAttribute ( SDL_GL_DEPTH_SIZE, 24 );
    SDL_GL_SetAttribute ( SDL_GL_STENCIL_SIZE, 8 );
    SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLEBUFFERS, 1 );
    SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLESAMPLES, 4 );

    SDL_GetDesktopDisplayMode ( 0, &DM );

    window_flags = (SDL_WindowFlags)( SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_BORDERLESS /*|
            SDL_WINDOW_MAXIMIZED*/ );
    window = SDL_CreateWindow ( "Envelope editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              DM.w, DM.h, window_flags );

    gl_context = SDL_GL_CreateContext ( window );
    SDL_GL_MakeCurrent ( window, gl_context );
    SDL_GL_SetSwapInterval (1 ); // Enable vsync

    err = glewInit ( ) ;

    if ( err != GLEW_OK )
    {
        SDL_Log ("Unable to initialize OpenGL!\n" );
        return err;
    }

    IMGUI_CHECKVERSION ( );
    ImGui::CreateContext ( );
    ImGuiIO& io = ImGui::GetIO ( );
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if ( darkmode )
    {
        ImGui::StyleColorsDark ( );
    }
    else
    {
        ImGui::StyleColorsLight ( );
    }

    ImGui_ImplSDL2_InitForOpenGL ( window, gl_context );
    ImGui_ImplOpenGL3_Init ( glsl_version );


    if ( SDL_GetDisplayDPI( 0, NULL, NULL, &dpi ) )
    {
        dpi = 96;
    }

    envelopeEditorContext.dpi = dpi;
    fileBrowserCtx.dpi = dpi;
    fileBrowserCtx.type = ImGui::Ext::FileBrowserType::FILE;

    io.Fonts->AddFontFromFileTTF ( "Ubuntu-L.ttf", 12 * 0.0138897638 * dpi );

    glEnable ( GL_MULTISAMPLE ); //Enable antialiasing
    glEnable ( GL_DITHER );

    while ( running )
    {
        SDL_GetCurrentDisplayMode ( 0, &DM );

        while ( SDL_PollEvent( &event ) )
        {
            ImGui_ImplSDL2_ProcessEvent ( &event );

            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if ( event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE
                     && event.window.windowID == SDL_GetWindowID ( window ) )
            {
                running = false;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        /**************************************************************************************************************
        * *************************************************************************************************************
        * *************************************************************************************************************
        * *************************************************************************************************************
        * *************************************************************************************************************
        * *************************************************************************************************************
        * RENDERING CODE GOES BELOW HERE ******************************************************************************/

        ImGui::SetNextWindowSize (ImVec2 ( DM.w, DM.h ), ImGuiCond_Always );
        ImGui::SetNextWindowPos ( ImVec2 (0, 0) );

        //Center titlebar text
        ImGui::PushStyleVar ( ImGuiStyleVar_WindowTitleAlign, ImVec2 ( 0.5f, 0.5f ) );

        ImGui::Begin ( "Envelope Editor", &running, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
                     | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |ImGuiWindowFlags_MenuBar
                     | ( saved ? 0 : ImGuiWindowFlags_UnsavedDocument));


        if ( ImGui::BeginMenuBar ( ) )
        {
            if ( ImGui::BeginMenu ( "File" ) )
            {
                if ( ImGui::MenuItem ( "Open..", "Ctrl+O" ) && state == DEFAULT )
                {
                    state = OPEN_DIALOG_OPEN;
                    popupOpened = false;
                }
                if ( ImGui::MenuItem ( "New", "Ctrl+N" ) && state == DEFAULT )
                { new_file ( &envelopeEditorContext ); }
                if ( ImGui::MenuItem ( "Save", "Ctrl+S" ) && state == DEFAULT )
                {
                    state = save_file ( save_path, &envelopeEditorContext );
                    if ( state == SAVE_AS_DIALOG_OPEN )
                    { popupOpened = false; }
                }
                if ( ImGui::MenuItem ( "Save as", "Ctrl+Shift+S" ) && state == DEFAULT )
                {
                    state = SAVE_AS_DIALOG_OPEN;
                    popupOpened = false;
                }
                ImGui::EndMenu ( );
            }
            ImGui::EndMenuBar ( );
        }

        if ( state == DEFAULT && (ImGui::IsKeyDown ( SDL_SCANCODE_LCTRL )
        ||   ImGui::IsKeyDown ( SDL_SCANCODE_RCTRL )))
        {
            if ( ImGui::IsKeyDown ( SDL_SCANCODE_LSHIFT )
            ||   ImGui::IsKeyDown ( SDL_SCANCODE_RSHIFT ))
            {
                if ( ImGui::IsKeyPressed ( SDL_SCANCODE_S, false ) )
                {
                    state = SAVE_AS_DIALOG_OPEN;
                }
            }
            else
            {
                if ( ImGui::IsKeyPressed ( SDL_SCANCODE_S, false ) )
                {
                    state = save_file ( save_path, &envelopeEditorContext );
                }
                else if ( ImGui::IsKeyPressed ( SDL_SCANCODE_O, false ) )
                {
                    state = OPEN_DIALOG_OPEN;
                }
                else if ( ImGui::IsKeyPressed ( SDL_SCANCODE_N, false ) )
                {
                    new_file ( &envelopeEditorContext );
                }
            }
        }

        ImGui::SetNextWindowSize ( ImVec2( 600, 600 ) );
        ImGui::SetNextWindowPosCenter ( ImGuiCond_Always );

        switch ( state )
        {
            //TODO: OPEN FILE DIALOG AND SAVE DIALOG
            case OPEN_DIALOG_OPEN:

                if ( ! popupOpened )
                {
                    popupOpened = true;
                    ImGui::OpenPopup ( "OpenDialog" );
                }

                if ( ImGui::BeginPopup ( "OpenDialog" ) )
                {
                    ImGui::SetCursorPosX ( ImGui::GetWindowWidth () / 2 - ImGui::CalcTextSize ("Open File").x / 2 );
                    ImGui::Text ( "Open File" );
                    ImGui::Ext::FileBrowser ( &fileBrowserCtx );
                    if ( ImGui::Button ( "Open" ) )
                    {
                        open_file ( fileBrowserCtx.selectedFile, &envelopeEditorContext );
                        save_path = fileBrowserCtx.selectedFile;
                        ImGui::CloseCurrentPopup ( );
                    }
                    ImGui::SameLine ( );
                    if ( ImGui::Button ( "Cancel" ) )
                    {
                        ImGui::CloseCurrentPopup ( );
                    }
                    ImGui::EndPopup ( );
                }
                else
                {
                    state = DEFAULT;
                    popupOpened = false;
                }
                break;
            case SAVE_AS_DIALOG_OPEN:
                if ( ! popupOpened )
                {
                    popupOpened = true;
                    ImGui::OpenPopup ( "SaveAsDialog" );
                }

                if ( ImGui::BeginPopup ( "SaveAsDialog" ) )
                {
                    ImGui::SetCursorPosX ( ImGui::GetWindowWidth () / 2 - ImGui::CalcTextSize ("Save File As").x / 2 );
                    ImGui::Text ( "Save File As" );

                    if ( ( ImGui::Ext::FileBrowser ( &fileBrowserCtx ) && fileBrowserCtx.fileCreated )
                    ||     ImGui::Button ( "Save" ) )
                    {
                        save_file ( fileBrowserCtx.selectedFile, &envelopeEditorContext );
                        save_path = fileBrowserCtx.selectedFile;
                        ImGui::CloseCurrentPopup ( );
                    }
                    ImGui::SameLine ( );
                    if ( ImGui::Button ( "Cancel" ) )
                    {
                        ImGui::CloseCurrentPopup ( );
                    }
                    ImGui::EndPopup ();
                }
                else
                {
                    state = DEFAULT;
                    popupOpened = false;
                }
                break;
            case DEFAULT:
                break;
            default:
                break;
        }

        if ( envelopeEditorContext.env )
        {
            if ( ImGui::InputDouble ( "Max time ", &envelopeEditorContext.env->maxTime ) )
            {
                envelopeEditorContext._updatePlot = true;
                envelopeEditorContext.env->maxTime = fmax ( envelopeEditorContext.env->maxTime, 0 );
            }

            ImGui::Ext::EnvelopeEditor ( &envelopeEditorContext );
        }

        ImGui::End ( );

        ImGui::PopStyleVar ( );

        /* RENDERING CODE GOES ABOVE HERE******************************************************************************
         * ************************************************************************************************************
         * ************************************************************************************************************
         * ************************************************************************************************************
         * ************************************************************************************************************
         * ************************************************************************************************************
         * ************************************************************************************************************/


        ImGui::Render();
        glViewport ( 0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y );
        glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
        glClear ( GL_COLOR_BUFFER_BIT );
        ImGui_ImplOpenGL3_RenderDrawData ( ImGui::GetDrawData() );
        SDL_GL_SwapWindow ( window );
    }

    ImGui_ImplOpenGL3_Shutdown ( );
    ImGui_ImplSDL2_Shutdown ( );
    ImGui::Ext::EnvelopeEditorFreeContext ( &envelopeEditorContext );
    ImGui::DestroyContext ( );
    SDL_GL_DeleteContext ( gl_context );
    SDL_DestroyWindow ( window );
    SDL_Quit ( );

    return 0;
}

states save_file ( std::string path, ImGui::Ext::EnvelopeEditorContext *ctx )
{

    if ( path.length ( ) == 0 )
    {
        return SAVE_AS_DIALOG_OPEN;
    }

    if ( ctx->env ) save_breakpoints ( path.c_str( ), ctx->env );

    return DEFAULT;
}

void new_file ( ImGui::Ext::EnvelopeEditorContext *ctx )
{
    if ( ctx->env )
    {
        free_env ( ctx->env );
    }

    ctx->env = ( envelope* ) calloc ( 1, sizeof ( envelope ) );

    ctx->env->minVal  = 0;
    ctx->env->maxVal  = 1;
    ctx->env->minTime = 0;
    ctx->env->maxTime = 1;

    ctx->env->first =       ( breakpoint* ) calloc ( 1, sizeof( breakpoint ) );
    ctx->env->first->next = ( breakpoint* ) calloc ( 1, sizeof( breakpoint ) );

    ctx->env->first->time  = 0;
    ctx->env->first->value = 1;
    ctx->env->first->interpType = LINEAR;
    ctx->env->first->interpCallback = interp_functions [ LINEAR ];
    ctx->env->current = ctx->env->first;

    ctx->env->first->next->time  = 1;
    ctx->env->first->next->value = 0;
    ctx->env->first->next->interpType = LINEAR;
    ctx->env->first->next->interpCallback = interp_functions [ LINEAR ];
}

void open_file ( std::string path, ImGui::Ext::EnvelopeEditorContext *ctx )
{
    if ( ctx->env == NULL )
    {
        ctx->env = ( envelope* ) calloc ( 1, sizeof ( envelope ) );
    }

    load_breakpoints ( path.c_str( ), ctx->env );
}