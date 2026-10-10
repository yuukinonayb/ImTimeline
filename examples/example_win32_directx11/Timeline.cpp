// https://github.com/CedricGuillemet/ImGuizmo
// v1.92.5 WIP
//
// The MIT License(MIT)
//
// Copyright(c) 2016-2026 Cedric Guillemet and contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#pragma optimize("", off)
#include "Timeline.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstdlib>

namespace ImTimeline
{
#ifndef IMGUI_DEFINE_MATH_OPERATORS
    static ImVec2 operator+(const ImVec2& a, const ImVec2& b) {
        return ImVec2(a.x + b.x, a.y + b.y);
    }
#endif
    static bool TimelineAddDelButton(ImDrawList* draw_list, ImVec2 pos, bool add = true)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImRect btnRect(pos, ImVec2(pos.x + 16, pos.y + 16));
        bool overBtn = btnRect.Contains(io.MousePos);
        bool containedClick = overBtn && btnRect.Contains(io.MouseClickedPos[0]);
        bool clickedBtn = containedClick && io.MouseReleased[0];
        int btnColor = overBtn ? 0xAAEAFFAA : 0x77A3B2AA;
        if (containedClick && io.MouseDownDuration[0] > 0)
            btnRect.Expand(2.0f);

        float midy = pos.y + 16 / 2 - 0.5f;
        float midx = pos.x + 16 / 2 - 0.5f;
        draw_list->AddRect(btnRect.Min, btnRect.Max, btnColor, 4);
        draw_list->AddLine(ImVec2(btnRect.Min.x + 3, midy), ImVec2(btnRect.Max.x - 3, midy), btnColor, 2);
        if (add)
            draw_list->AddLine(ImVec2(midx, btnRect.Min.y + 3), ImVec2(midx, btnRect.Max.y - 3), btnColor, 2);
        return clickedBtn;
    }
    static const int timeline_pixel_size = 1600;
    static const int timeline_pixel_block = 20;
    bool Timeline(TimelineInterface* sequence, int* current_frame, int* selectedEntry, int viewFramesSize)
    {
        if (viewFramesSize == 0) return false;
        bool ret = false;
        int sequenceCount = sequence->GetItemCount();
        ImGuiIO& io = ImGui::GetIO();

        int frame_count = 0;
        int timeline_frames = sequence->GetTotalSize();
        int actual_timeline_frames = sequence->GetTotalSize();
        int ItemHeight = 20;

        int start_frame_off = 0;

        int timeline_block_frame = 1;
        if (viewFramesSize < 0)
        {
            while (timeline_block_frame * (timeline_pixel_size / timeline_pixel_block) < actual_timeline_frames)
            {
                timeline_block_frame++;
            }
        }
        else
        {
            timeline_block_frame = viewFramesSize;
            int visible_frames = timeline_block_frame * (timeline_pixel_size / timeline_pixel_block);
            if (visible_frames < actual_timeline_frames)
            {
                int half = visible_frames / 2;
                start_frame_off = *current_frame - half;
            }
            if (start_frame_off > actual_timeline_frames - visible_frames)
            {
                start_frame_off = actual_timeline_frames - visible_frames;
            }
            if (start_frame_off < 0)
            {
                start_frame_off = 0;
            }
        }
        float timeline_pixel_frame = static_cast<float>(timeline_block_frame) / static_cast<float>(timeline_pixel_block);
        int visible_frames = timeline_block_frame * (timeline_pixel_size / timeline_pixel_block);

        ImGui::BeginGroup();

        int controlHeight =( sequenceCount + 1) * ItemHeight;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
        ImVec2 canvas_size = ImVec2(timeline_pixel_size, controlHeight);

        ImRect regionRect(canvas_pos, canvas_pos + canvas_size);


        //if (visibleFrameCount >= frameCount)
        {
            ImVec2 headerSize(canvas_size.x, (float)ItemHeight);

            ImGui::InvisibleButton("topBar", headerSize);
            draw_list->AddRectFilled(canvas_pos, canvas_pos + headerSize, 0, 0);
            ImVec2 childFramePos = ImGui::GetCursorScreenPos();
            ImVec2 childFrameSize(canvas_size.x, canvas_size.y - headerSize.y);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
            ImGui::BeginChild(889, childFrameSize, ImGuiChildFlags_FrameStyle);

            sequence->focused = ImGui::IsWindowFocused();
            const ImVec2 contentMin = ImGui::GetItemRectMin();
            const ImVec2 contentMax = ImGui::GetItemRectMax();
            const ImRect contentRect(contentMin, contentMax);
            const float contentHeight = contentMax.y - contentMin.y;

            // full background
            draw_list->AddRectFilled(canvas_pos, canvas_pos + canvas_size, 0x11000000, 0);

            //header frame number and lines
            int modFrameCount = timeline_pixel_block * timeline_pixel_frame * 10;
            int frameStep = timeline_pixel_block * timeline_pixel_frame;
            int halfModFrameCount = modFrameCount / 2;

            auto drawLine = [&](int i, int regionHeight, bool forcedraw_base=false) {
                bool baseIndex = ((i % modFrameCount) == 0) || (i == sequence->GetFrameMax() || i == sequence->GetFrameMin());
                bool halfIndex = (i % halfModFrameCount) == 0;
                int px = (int)canvas_pos.x + int((i- start_frame_off) / timeline_pixel_frame) ;
                int tiretStart = baseIndex ? 4 : (halfIndex ? 10 : 14);
                int tiretEnd = regionHeight;
                float thick= baseIndex ? 2.f : 1.f;
                float thick_off = baseIndex ? 0.5f : 0.f;

                if (px <= (canvas_size.x + canvas_pos.x) && px >= (canvas_pos.x ))
                {
                    draw_list->AddLine(
                        ImVec2((float)px- thick_off, canvas_pos.y + (float)tiretStart),
                        ImVec2((float)px- thick_off, canvas_pos.y + (float)tiretEnd),
                        0x60000000,
                        thick
                    );
                }

                if (baseIndex && px >= (canvas_pos.x ) && px <= (canvas_size.x + canvas_pos.x ))
                {
                    if (i + frameStep < timeline_frames || forcedraw_base)
                    {
                        char tmps[512];
                        ImFormatString(tmps, IM_ARRAYSIZE(tmps), "%d", i);
                        draw_list->AddText(ImVec2((float)px + 3.f, canvas_pos.y), 0x60000000, tmps);
                    }
                }

                };

            auto drawLineContent = [&](int i, int /*regionHeight*/) {
                int px = (int)canvas_pos.x + int(i * timeline_pixel_frame);
                int tiretStart = int(contentMin.y);
                int tiretEnd = int(contentMax.y);

                if (px <= (canvas_size.x + canvas_pos.x) && px >= (canvas_pos.x ))
                {
                    //draw_list->AddLine(ImVec2((float)px, canvas_pos.y + (float)tiretStart), ImVec2((float)px, canvas_pos.y + (float)tiretEnd - 1), 0xFF606060, 1);

                    draw_list->AddLine(ImVec2(float(px), float(tiretStart)), ImVec2(float(px), float(tiretEnd)), 0x30606060, 1);
                }
                };

            for (int i = sequence->GetFrameMin(); i <= sequence->GetFrameMax(); i += frameStep)
            {
                drawLine(i, controlHeight);
            }
            drawLine(sequence->GetFrameMin(), controlHeight);
            drawLine(sequence->GetFrameMax(), controlHeight,true);
            // slots background
            for (int i = 0; i < sequenceCount; i++)
            {
                unsigned int col = (i & 1) ? 0x340A0606 : 0x64413D3D;
                ImVec2 pos = ImVec2(contentMin.x , contentMin.y + ItemHeight * i + 1 );
                ImVec2 sz = ImVec2(canvas_size.x + canvas_pos.x, pos.y + ItemHeight - 1 );
                if (i == *selectedEntry)
                {
                    col = 0x9000FFFF;
                }
                draw_list->AddRectFilled(pos, sz, col, 0);
            }
            // slots
            for (int i = 0; i < sequenceCount; i++)
            {
                int* start, * end;
                unsigned int color;
                sequence->Get(i, &start, &end, NULL, &color);

                ImVec2 pos = ImVec2(contentMin.x, contentMin.y + ItemHeight * i + 1);
                ImVec2 slotP1(pos.x + ImMax((*start- start_frame_off),0) / timeline_pixel_frame, pos.y + 2);
                ImVec2 slotP2(pos.x + ImMin((*end - start_frame_off),visible_frames) / timeline_pixel_frame + timeline_pixel_frame, pos.y + ItemHeight - 3);
                ImVec2 slotP3(pos.x + ImMin((*end - start_frame_off), visible_frames) / timeline_pixel_frame + timeline_pixel_frame, pos.y + ItemHeight - 3 );
                unsigned int slotColor = color | 0xFF000000;
                unsigned int slotColorHalf = (color & 0xFFFFFF) | 0x40000000;

                if (slotP1.x <= (canvas_size.x + contentMin.x) && slotP2.x >= (contentMin.x))
                {
                    draw_list->AddRectFilled(slotP1, slotP3, slotColorHalf, 2);
                    draw_list->AddRectFilled(slotP1, slotP2, slotColor, 2);
                }
                // Ensure grabbable handles
                const float max_handle_width = slotP2.x - slotP1.x / 3.0f;
                const float min_handle_width = ImMin(10.0f, max_handle_width);
                const float handle_width = ImClamp(timeline_pixel_frame / 2.0f, min_handle_width, max_handle_width);
                ImRect rects[3] = { ImRect(slotP1, ImVec2(slotP1.x + handle_width, slotP2.y))
                    , ImRect(ImVec2(slotP2.x - handle_width, slotP1.y), slotP2)
                    , ImRect(slotP1, slotP2) };

                const unsigned int quadColor[] = { 0xFFFFFFFF, 0xFFFFFFFF, slotColor + 0x202020 };
                // 
                //if (movingEntry == -1 && (sequenceOptions & SEQUENCER_EDIT_STARTEND))// TODOFOCUS && backgroundRect.Contains(io.MousePos))
                {
                    for (int j = 2; j >= 0; j--)
                    {
                        ImRect& rc = rects[j];
                        if (!rc.Contains(io.MousePos))
                            continue;
                        draw_list->AddRectFilled(rc.Min, rc.Max, quadColor[j], 2);
                    }

                    for (int j = 0; j < 3; j++)
                    {
                        ImRect& rc = rects[j];
                        if (!rc.Contains(io.MousePos))
                            continue;
                        if (!ImRect(childFramePos, childFramePos + childFrameSize).Contains(io.MousePos))
                            continue;
                    }
                }
            }

            // cursor
            if (current_frame && *current_frame <= sequence->GetFrameMax())
            {
                static const float cursorWidth = 6.f;
                float cursorOffset = contentMin.x + (*current_frame - start_frame_off) / timeline_pixel_frame ;
                draw_list->AddLine(ImVec2(cursorOffset, canvas_pos.y), ImVec2(cursorOffset, canvas_pos.y + controlHeight ), 0x902A2AFF, cursorWidth);
                char tmps[512];
                ImFormatString(tmps, IM_ARRAYSIZE(tmps), "%d", *current_frame);
                draw_list->AddText(ImVec2(cursorOffset + 10, canvas_pos.y + 9), 0xFF2A2AFF, tmps);
            }

            //draw_list->PushClipRect(canvas_pos + ImVec2(0,ItemHeight), canvas_pos + childFrameSize, true);

            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
        ImGui::EndGroup();
        return ret;
    }
}
