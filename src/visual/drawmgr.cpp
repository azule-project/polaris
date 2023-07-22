/*
 * drawmgr.cpp
 *
 *  Created on: May 22, 2017
 *      Author: nullifiedcat
 */
#include <MiscTemporary.hpp>
#include <hacks/Aimbot.hpp>
#include <hacks/hacklist.hpp>
#if ENABLE_IMGUI_DRAWING
#include "imgui/imrenderer.hpp"
#elif ENABLE_GLEZ_DRAWING
#include <glez/glez.hpp>
#include <glez/record.hpp>
#include <glez/draw.hpp>
#endif
#include <settings/Bool.hpp>
#include <settings/Float.hpp>
#include <settings/Rgba.hpp>
#include <menu/GuiInterface.hpp>
#include "common.hpp"
#include "visual/drawing.hpp"
#include "hack.hpp"
#include "menu/menu/Menu.hpp"
#include "drawmgr.hpp"

static settings::Boolean info_text{ "hack-info.enable", "true" };
static settings::Boolean info_text_min{ "hack-info.minimal", "false" };
static settings::Boolean info_name{ "hack-info.name", "Weebware" };
static settings::Int info_style{ "hack-info.style", "0" };
static settings::Int info_x{"hack-info.x", "10"};
static settings::Int info_y{"hack-info.y", "10"};
static settings::Float info_alpha{"hack-info.alpha", "0.7"};

void RenderCheatVisuals()
{
    {
        PROF_SECTION(BeginCheatVisuals)
        BeginCheatVisuals();
    }
    {
        PROF_SECTION(DrawCheatVisuals)
        DrawCheatVisuals();
    }
    {
        PROF_SECTION(EndCheatVisuals)
        EndCheatVisuals();
    }
}
#if ENABLE_GLEZ_DRAWING
glez::record::Record bufferA{};
glez::record::Record bufferB{};

glez::record::Record *buffers[] = { &bufferA, &bufferB };
#endif
int currentBuffer = 0;

void BeginCheatVisuals()
{
#if ENABLE_IMGUI_DRAWING
    im_renderer::bufferBegin();
#elif ENABLE_GLEZ_DRAWING
    buffers[currentBuffer]->begin();
#endif
    ResetStrings();
}

void DrawCheatVisuals()
{
    {
        PROF_SECTION(DRAW_info)
        std::string name_s, reason_s;
        PROF_SECTION(PT_info_text)
        #if ENABLE_GUI
        if (*info_text/* && (!g_IEngine->IsConnected() || g_IEngine->Con_IsVisible())*/)
        {
        if (*info_style ==  0) { //new
        //replace std::localtime
        char timeString[10];
        time_t current_time;
        struct tm *time_info;
        time(&current_time);
        time_info = localtime(&current_time);
        //std::localtime ^
        strftime(timeString, sizeof(timeString), "%H:%M:%S", time_info);

        
        float w, h;
        //make use of info_name variable
        /*fonts::center_screen->stringSize("Weebware", &w, &h);
        draw::Rectangle(*info_x - 5, *info_y - 5, w + 10, h + 10, colors::Transparent(colors::black, *info_alpha));
         draw::String(*info_x, *info_y, colors::gui, "Weebware", *fonts::center_screen);*/
        }
        else if(*info_style == 1) {//old "90s"
           auto color = colors::RainbowCurrent();
            color.a    = 1.0f;
            AddSideString("Weebware", color);
        }
           
            if (!*info_text_min && !*info_style == 0)
            {
                AddSideString("Press '" + open_gui_button.toString() + "' key.", colors::gui);
                AddSideString(hack::GetVersion(), colors::gui);
#endif
            }
        }
    }
    if (spectator_target)
    {
        AddCenterString("Press SPACE to stop spectating");
    }
    {
        PROF_SECTION(DRAW_WRAPPER);
        EC::run(EC::Draw);
    }
    if (CE_GOOD(g_pLocalPlayer->entity) && !g_Settings.bInvalid)
    {
        Prediction_PaintTraverse();
    }
    {
        PROF_SECTION(DRAW_strings);
        DrawStrings();
    }
#if ENABLE_GUI
    {
        PROF_SECTION(DRAW_GUI);
        gui::draw();
    }
#endif
}


void EndCheatVisuals()
{
#if ENABLE_GLEZ_DRAWING
    buffers[currentBuffer]->end();
#endif
#if ENABLE_GLEZ_DRAWING || ENABLE_IMGUI_DRAWING
    currentBuffer = !currentBuffer;
#endif
}

void DrawCache()
{
#if ENABLE_GLEZ_DRAWING
    buffers[!currentBuffer]->replay();
#endif
}
