
#include <cstdlib>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "ImGuiEnvelopeEditor.h"
#include <iostream>


void calculatePlotDerivatives ( float *plotdata, float *derivs, int width )
{
    int i;

    for ( i = 0; i < width; i++ )
    {
        if ( i == width - 1 )
        {
            derivs [ i ] = ( plotdata [ i ] - plotdata [ i - 1 ] );
        }
        else
        {
            derivs [ i ] = ( plotdata [ i + 1 ] - plotdata [ i ] );
        }
    }
}

uint32_t packRGB ( ImColor col )
{
    unsigned char r, g, b, a;

    r = (unsigned char)(col.Value.x * 255);
    g = (unsigned char)(col.Value.y * 255);
    b = (unsigned char)(col.Value.z * 255);
    a = (unsigned char)(col.Value.w * 255);

    return (r << 24 | g << 16 | b << 8 | a);
}


#define CLAMP(x, min, max) x = ( x < min ? min : ( x > max ? max : x ) )


ImTextureID sdlImpl_drawEnvelopePlot ( float *plotData, float *plotDerivatives, int width, int height, ImColor bgColour,
        ImColor fgColour, float lineWidth )
{
    uint32_t fgColourPacked, bgColourPacked, *plot;
    int i, x, y, xmax, ymax;
    float theta, k, hypot, costheta, sintheta, j;
    GLuint tex;

    xmax = width - 1;
    ymax = height - 1;

    plot = (uint32_t*) malloc ( sizeof ( uint32_t ) * width * height );

    // Convert float colours into packed RGB form
    fgColourPacked = packRGB ( fgColour );
    bgColourPacked = packRGB ( bgColour );

    // Fill in the background
    for ( i = 0; i < width * height; i++ )
    {
        plot [ i ] = bgColourPacked;
    }

    /* For each pixel in the plot draw a lineWidth x sqrt ( dx^2 + dy^2 ) rectangle oriented on atan ( dy/dx )
     * Effectively a bunch of lineWidth thick rectangles oriented in the direction of the plot at that point */

    for ( i = 0; i < width; i++ )
    {
        hypot = ( i > 0 && i < width - 1 )
                ? sqrt ( 1 + pow( plotData [ i + 1 ] - plotData [ i ], 2 ) )
                : 1;

        theta = atanf ( plotDerivatives [ i ] );
        sincosf ( theta, &sintheta, &costheta );

        for ( k = 0; k < hypot; k++ )
        {
            for ( j = -lineWidth / 2; j < lineWidth / 2; j += 0.48 )
            {
                x = i             + k * costheta - j * sintheta;
                y = plotData[ i ] + k * sintheta + j * costheta;

                CLAMP ( x, 0, xmax );
                y = height - y; // Uninvert
                CLAMP ( y, 0, ymax );

                plot [ y * width + x ] = fgColourPacked;
            }
        }
    }

    glGenTextures ( 1, &tex );
    glBindTexture ( GL_TEXTURE_2D, tex );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, plot );

    glBindTexture(GL_TEXTURE_2D, 0);

    free ( plot );

    return (ImTextureID)(intptr_t)tex;
}

void sdlImpl_free_image ( ImTextureID img )
{
    GLuint tex = (GLuint)((intptr_t)img);
    glDeleteTextures( 1, &tex );
}

ImTextureID ( *drawEnvelopePlot )( float* plotData, float* plotDerivatives, int width, int height, ImColor bgColour,
        ImColor fgColour, float lineWidth ) = &sdlImpl_drawEnvelopePlot;

void (*free_image) ( ImTextureID img ) = &sdlImpl_free_image;

#define impl_LCTRL SDL_SCANCODE_LCTRL
#define impl_RCTRL SDL_SCANCODE_RCTRL

IMGUI_API bool ImGui::Ext::EnvelopeEditor ( EnvelopeEditorContext *context )
{
    ImVec2 plotArea, windowOffset, mousePos;
    int i = 0, j;
    float x, y, dx, dy;
    double nodeClampX, nodeClampXMax, time, value;
    breakpoint* newbp;

    ImU32 bgColourPacked  = packRGB ( context->bgColour ), fgColourPacked = packRGB ( context->fgColour ),
          fgColour2Packed = packRGB ( context->fgColour2 );

    if ( ! context->env )
    {
        return false;
    }

    ImGui::PushID ( "EnvelopeEditor" );
    ImGui::PushStyleVar ( ImGuiStyleVar_WindowPadding, ImVec2 ( 0, 0) );
    ImGui::BeginChild ( "EnvelopeEditor", context->dimensions, true, ImGuiWindowFlags_NoScrollbar );

    windowOffset = ImGui::GetWindowPos  ( );
    plotArea     = ImGui::GetWindowSize ( );
    mousePos     = ImGui::GetMousePos   ( );

    mousePos.x -= windowOffset.x;
    mousePos.y -= windowOffset.y;

    if ( ImGui::IsMouseReleased ( 0 ) )
    {
        // If we were dragging then stop
        context->_draggingPoint = -1;
    }

    if ( context->_updatePlot )
    {
        if ( ! context->_plotData )
        {
            context->_plotData        = (float*) calloc ( plotArea.x, sizeof ( float ) );
            context->_plotDerivatives = (float*) calloc ( plotArea.x, sizeof ( float ) );
        }
        else
        {
            free ( context->_plotData );
            free ( context->_plotDerivatives );
            free_image ( context->_plotImg );
            context->_plotData        = (float*) calloc ( plotArea.x, sizeof ( float ) );
            context->_plotDerivatives = (float*) calloc ( plotArea.x, sizeof ( float ) );
        }

        plot_envelope ( context->env, (int)plotArea.x, (int)plotArea.y, context->_plotData );
        calculatePlotDerivatives ( context->_plotData, context->_plotDerivatives, plotArea.x );

        context->_plotImg =
                (*drawEnvelopePlot)( context->_plotData, context->_plotDerivatives,
                        plotArea.x, plotArea.y, context->bgColour, context->fgColour,
                        context->lineThickness * context->dpi );

        context->_updatePlot = false;
    }

    ImGui::Image ( context->_plotImg, plotArea );

    if ( ( ImGui::IsKeyDown ( impl_LCTRL ) || ImGui::IsKeyDown ( impl_RCTRL ) )
         &&   mousePos.x >= 0          && mousePos.x <=  plotArea.x
         &&   mousePos.y <= plotArea.y && mousePos.y >= 0 )
    {
        time  = ( mousePos.x / plotArea.x ) * context->env->maxTime;
        value = value_at ( context->env, time );

        y = ( value / context->env->maxVal ) * plotArea.y;
        y = plotArea.y - y;
        x = mousePos.x;

        CLAMP ( y, 0, plotArea.y );
        CLAMP ( x, 0, plotArea.x );

        ImGui::GetWindowDrawList ( )->AddCircle ( ImVec2 ( windowOffset.x + x, windowOffset.y + y ),
                                                  2 * context->lineThickness * context->dpi, context->fgColour2, 30,
                                                  2 * context->lineThickness * context->dpi / 4 );

        ImGui::BeginTooltip ( );
        ImGui::LabelText ( "time",  "%f", time  );
        ImGui::LabelText ( "value", "%f", value - context->axisHeight );
        ImGui::EndTooltip ( );

        if ( ImGui::IsMouseClicked ( 0, false ) )
        {
            newbp = ( breakpoint* ) calloc ( 1, sizeof ( breakpoint ) );
            newbp->time           = time;
            newbp->value          = value;
            newbp->interpType     = LINEAR;
            newbp->interpCallback = interp_functions [ LINEAR ];
            insert_breakpoint ( context->env, newbp );
        }
    }

    // Draw X axis

    if ( context->axisHeight >= 0 )
    {
        y = context->axisHeight * plotArea.y;

        y = plotArea.y - y;

        CLAMP ( y, 0, plotArea.y );

        ImGui::GetWindowDrawList ( )->AddLine ( ImVec2 ( windowOffset.x, y + windowOffset.y ),
                ImVec2 ( windowOffset.x + plotArea.x, y + windowOffset.y ), context->fgColour2, 1 );
    }

    nodeClampX = context->env->minTime;

    context->env->current = context->env->first;

    while ( context->env->current )
    {
        ImGui::PushID ( i );

        if ( context->env->current->next )
        {
            nodeClampXMax = context->env->current->next->time;
        }
        else
        {
            nodeClampXMax = context->env->maxTime;
        }

        x = ( context->env->current->time  / context->env->maxTime ) * plotArea.x;
        y = ( context->env->current->value / context->env->maxVal  ) * plotArea.y;
        y = plotArea.y - y;

        ImGui::GetWindowDrawList ( )->AddCircleFilled ( ImVec2 ( windowOffset.x + x, windowOffset.y + y ),
                2 * context->lineThickness * context->dpi, fgColourPacked, 30 );

        if ( sqrt ( pow ( mousePos.x - x, 2 ) + pow ( mousePos.y - y, 2 ) )
             <= 2 * context->lineThickness  * context->dpi )
        {
            ImGui::GetWindowDrawList ( )->AddCircleFilled ( ImVec2 ( windowOffset.x + x, windowOffset.y + y ),
                    2 * context->lineThickness * context->dpi, fgColour2Packed, 30 );

            if ( ImGui::IsMouseClicked ( 1, false ) )
            {
                ImGui::OpenPopup ( "NodeOptions" );
            }

            if ( ImGui::IsMouseClicked ( 0, false ) )
            {
                // Set flag to start moving this node
                // When position is updated don't forget to invalidate the plot drawing
                context->_draggingPoint = i;
                context->_mousePosition = mousePos;
            }

            // Maybe use IsMouseDragging?
        }

        if ( ImGui::BeginPopup ( "NodeOptions" ) )
        {
            // Listbox with interpolation types and any other parameters, e.g manually set value
            if ( ImGui::ListBox ( "Segment type", (int*)&context->env->current->interpType, context->interpTypeStrings,
                    4 ) )
            {
                context->env->current->interpCallback = interp_functions [ context->env->current->interpType ];

                // If the user selected Quadratic bezier we must supply a control point in the interp params
                if ( context->env->current->interpType == QUADRATIC_BEZIER && context->env->current->nInterp_params < 2)
                {
                    context->env->current->nInterp_params      = 2;
                    context->env->current->interp_params       = (double*) malloc ( sizeof ( double ) * 2 );
                    context->env->current->interp_params [ 0 ] = (context->env->current->time +
                                                                  context->env->current->next->time) / 2;
                    context->env->current->interp_params [ 1 ] = (context->env->current->value +
                                                                  context->env->current->next->value) / 2;
                }

                context->_updatePlot = true;
            }
            ImGui::EndPopup ( );
        }

        if ( context->_draggingPoint == i )
        {
            dx = mousePos.x - context->_mousePosition.x;
            dy = mousePos.y - context->_mousePosition.y;

            context->env->current->time  += context->env->maxTime * ( dx / plotArea.x );
            context->env->current->value -= context->env->maxVal  * ( dy / plotArea.y );

            CLAMP ( context->env->current->value, context->env->minVal,  context->env->maxVal  );
            CLAMP ( context->env->current->time,  nodeClampX,            nodeClampXMax         );

            context->_mousePosition = mousePos;

            context->_updatePlot = true;
        }

        if ( context->env->current->nInterp_params > 0 && context->env->current->nInterp_params % 2 == 0
        &&  context->env->current->interpType != LINEAR && context->env->current->interpType != NEAREST_NEIGHBOUR )
        {
            for ( j = 0; j < context->env->current->nInterp_params; j += 2 )
            {
                i++;
                x = ( context->env->current->interp_params [ j ]     / context->env->maxTime ) * plotArea.x;
                y = ( context->env->current->interp_params [ j + 1 ] / context->env->maxVal  ) * plotArea.y;
                y = plotArea.y - y;

                ImGui::GetWindowDrawList ( )->AddCircle ( ImVec2 ( windowOffset.x + x, windowOffset.y + y ),
                                                        2 * context->lineThickness * context->dpi, fgColourPacked, 30,
                                                        2 * context->lineThickness * context->dpi / 4 );

                if ( sqrt ( pow ( mousePos.x - x, 2 ) + pow ( mousePos.y - y, 2 ) )
                     <= 2 * context->lineThickness  * context->dpi )
                {
                    ImGui::GetWindowDrawList ( )->AddCircle ( ImVec2 ( windowOffset.x + x, windowOffset.y + y ),
                            2 * context->lineThickness * context->dpi, fgColour2Packed, 30,
                                                              2 * context->lineThickness * context->dpi / 4 );

                    if ( ImGui::IsMouseClicked ( 0, false ) )
                    {
                        // Set flag to start moving this node
                        context->_draggingPoint = i;
                        context->_mousePosition = mousePos;
                    }
                }

                if ( context->_draggingPoint == i )
                {
                    dx = mousePos.x - context->_mousePosition.x;
                    dy = mousePos.y - context->_mousePosition.y;

                    context->env->current->interp_params [ j ]     += context->env->maxTime * ( dx / plotArea.x );
                    context->env->current->interp_params [ j + 1 ] -= context->env->maxVal  * ( dy / plotArea.y );

                    CLAMP ( context->env->current->interp_params [ j ], context->env->minTime, context->env->maxTime );
                    CLAMP ( context->env->current->interp_params [ j + 1 ], context->env->minVal, context->env->maxVal);

                    context->_mousePosition = mousePos;

                    context->_updatePlot = true;
                }
            }
        }

        nodeClampX = context->env->current->time;

        ImGui::PopID ( );
        context->env->current = context->env->current->next;
        i++;
    }

    context->env->current = context->env->first;

    ImGui::EndChild ( );

    ImGui::PopStyleVar ( );
    ImGui::PopID ( );

    return false;
}

IMGUI_API void ImGui::Ext::EnvelopeEditorFreeContext ( EnvelopeEditorContext *ctx )
{
    if ( ctx->_plotDerivatives )
    {
        free ( ctx->_plotDerivatives );
    }

    if ( ctx->_plotData )
    {
        free ( ctx->_plotData );
    }

    if ( ctx->env )
    {
        free ( ctx->env );
    }
}