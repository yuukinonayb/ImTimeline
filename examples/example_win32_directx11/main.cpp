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
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#define IMAPP_IMPL
#include "..\imguizmo\ImApp.h"

#ifdef None
#undef None
#endif

#include "Timeline.h"
#include "..\imguizmo\ImGuizmo.h"
#include "..\imguizmo\ImSequencer.h"
#include "..\imguizmo\ImZoomSlider.h"
#include "..\imguizmo\ImCurveEdit.h"
#include "..\imguizmo\ImVectorEditor.h"
#include "..\imguizmo\GraphEditor.h"
#include "..\imguizmo\ImLightRig.h"
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>

bool useWindow = true;
int gizmoCount = 1;
bool gizmoEnabled[4] = { true, true, true, true };

// Optional second viewport with its own camera
bool useSecondView = false;
float cameraView2[16] =
{ 1.f, 0.f, 0.f, 0.f,
  0.f, 1.f, 0.f, 0.f,
  0.f, 0.f, 1.f, 0.f,
  0.f, 0.f, 0.f, 1.f };
float camDistance2 = 8.f;
float camYAngle2 = 165.f / 180.f * 3.14159f;
float camXAngle2 = 32.f / 180.f * 3.14159f;
float camDistance = 8.f;
float camYAngle = 165.f / 180.f * 3.14159f;
float camXAngle = 32.f / 180.f * 3.14159f;
static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
static bool useSnap(false);
static float snap[3] = { 1.f, 1.f, 1.f };
static float bounds[] = { -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
static float boundsSnap[] = { 0.1f, 0.1f, 0.1f };
static bool boundSizingSnap = false;


//
//
// ImSequencer interface
//
//
static const char* SequencerItemTypeNames[] = { "Camera","Music", "ScreenEffect", "FadeIn", "Animation" };

struct RampEdit : public ImCurveEdit::Delegate
{
    RampEdit()
    {
        mPts[0][0] = ImVec2(-10.f, 0);
        mPts[0][1] = ImVec2(20.f, 0.6f);
        mPts[0][2] = ImVec2(25.f, 0.2f);
        mPts[0][3] = ImVec2(70.f, 0.4f);
        mPts[0][4] = ImVec2(120.f, 1.f);
        mPointCount[0] = 5;

        mPts[1][0] = ImVec2(-50.f, 0.2f);
        mPts[1][1] = ImVec2(33.f, 0.7f);
        mPts[1][2] = ImVec2(80.f, 0.2f);
        mPts[1][3] = ImVec2(82.f, 0.8f);
        mPointCount[1] = 4;


        mPts[2][0] = ImVec2(40.f, 0);
        mPts[2][1] = ImVec2(60.f, 0.1f);
        mPts[2][2] = ImVec2(90.f, 0.82f);
        mPts[2][3] = ImVec2(150.f, 0.24f);
        mPts[2][4] = ImVec2(200.f, 0.34f);
        mPts[2][5] = ImVec2(250.f, 0.12f);
        mPointCount[2] = 6;
        mbVisible[0] = mbVisible[1] = mbVisible[2] = true;
        mMax = ImVec2(1.f, 1.f);
        mMin = ImVec2(0.f, 0.f);
    }
    size_t GetCurveCount()
    {
        return 3;
    }

    bool IsVisible(size_t curveIndex)
    {
        return mbVisible[curveIndex];
    }
    size_t GetPointCount(size_t curveIndex)
    {
        return mPointCount[curveIndex];
    }

    uint32_t GetCurveColor(size_t curveIndex)
    {
        uint32_t cols[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 };
        return cols[curveIndex];
    }
    ImVec2* GetPoints(size_t curveIndex)
    {
        return mPts[curveIndex];
    }
    virtual ImCurveEdit::CurveType GetCurveType(size_t curveIndex) const { return ImCurveEdit::CurveSmooth; }
    virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value)
    {
        mPts[curveIndex][pointIndex] = ImVec2(value.x, value.y);
        SortValues(curveIndex);
        for (size_t i = 0; i < GetPointCount(curveIndex); i++)
        {
            if (mPts[curveIndex][i].x == value.x)
                return (int)i;
        }
        return pointIndex;
    }
    virtual void AddPoint(size_t curveIndex, ImVec2 value)
    {
        if (mPointCount[curveIndex] >= 8)
            return;
        mPts[curveIndex][mPointCount[curveIndex]++] = value;
        SortValues(curveIndex);
    }
    virtual ImVec2& GetMax() { return mMax; }
    virtual ImVec2& GetMin() { return mMin; }
    virtual unsigned int GetBackgroundColor() { return 0; }
    ImVec2 mPts[3][8];
    size_t mPointCount[3];
    bool mbVisible[3];
    ImVec2 mMin;
    ImVec2 mMax;
private:
    void SortValues(size_t curveIndex)
    {
        auto b = std::begin(mPts[curveIndex]);
        auto e = std::begin(mPts[curveIndex]) + GetPointCount(curveIndex);
        std::sort(b, e, [](ImVec2 a, ImVec2 b) { return a.x < b.x; });

    }
};
struct MySequencer : public ImSequencer::SequenceInterface
{
    // interface with sequencer

    virtual int GetFrameMin() const {
        return mFrameMin;
    }
    virtual int GetFrameMax() const {
        return mFrameMax;
    }
    virtual int GetItemCount() const { return (int)myItems.size(); }

    virtual int GetItemTypeCount() const { return sizeof(SequencerItemTypeNames) / sizeof(char*); }
    virtual const char* GetItemTypeName(int typeIndex) const { return SequencerItemTypeNames[typeIndex]; }
    virtual const char* GetItemLabel(int index) const
    {
        static char tmps[512];
        snprintf(tmps, 512, "[%02d] %s", index, SequencerItemTypeNames[myItems[index].mType]);
        return tmps;
    }

    virtual void Get(int index, int** start, int** end, int* type, unsigned int* color)
    {
        MySequenceItem& item = myItems[index];
        if (color)
            *color = 0xFFAA8080; // same color for everyone, return color based on type
        if (start)
            *start = &item.mFrameStart;
        if (end)
            *end = &item.mFrameEnd;
        if (type)
            *type = item.mType;
    }
    virtual void Add(int type) { myItems.push_back(MySequenceItem{ type, 0, 10, false }); };
    virtual void Del(int index) { myItems.erase(myItems.begin() + index); }
    virtual void Duplicate(int index) { myItems.push_back(myItems[index]); }

    virtual size_t GetCustomHeight(int index) { return  0; }

    // my datas
    MySequencer() : mFrameMin(0), mFrameMax(0) {}
    int mFrameMin, mFrameMax;
    struct MySequenceItem
    {
        int mType;
        int mFrameStart, mFrameEnd;
        bool mExpanded;
    };
    std::vector<MySequenceItem> myItems;
    RampEdit rampEdit;

    virtual void DoubleClick(int index) {
        if (myItems[index].mExpanded)
        {
            myItems[index].mExpanded = false;
            return;
        }
        for (auto& item : myItems)
            item.mExpanded = false;
        myItems[index].mExpanded = !myItems[index].mExpanded;
    }

    virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect)
    {
        static const char* labels[] = { "Translation", "Rotation" , "Scale" };

        rampEdit.mMax = ImVec2(float(mFrameMax), 1.f);
        rampEdit.mMin = ImVec2(float(mFrameMin), 0.f);
        draw_list->PushClipRect(legendClippingRect.Min, legendClippingRect.Max, true);
        for (int i = 0; i < 3; i++)
        {
            ImVec2 pta(legendRect.Min.x + 30, legendRect.Min.y + i * 14.f);
            ImVec2 ptb(legendRect.Max.x, legendRect.Min.y + (i + 1) * 14.f);
            draw_list->AddText(pta, rampEdit.mbVisible[i] ? 0xFFFFFFFF : 0x80FFFFFF, labels[i]);
            if (ImRect(pta, ptb).Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(0))
                rampEdit.mbVisible[i] = !rampEdit.mbVisible[i];
        }
        draw_list->PopClipRect();

        ImGui::SetCursorScreenPos(rc.Min);
        ImCurveEdit::Edit(rampEdit, rc.Max - rc.Min, 137 + index, &clippingRect);
    }

};

struct MyTimeline : public ImTimeline::TimelineInterface
{
    // interface with sequencer

    virtual int GetFrameMin() const {
        return mFrameMin;
    }
    virtual int GetFrameMax() const {
        return mFrameMax;
    }
    virtual int GetItemCount() const { return (int)myItems.size(); }

    virtual int GetItemTypeCount() const { return sizeof(SequencerItemTypeNames) / sizeof(char*); }
    virtual const char* GetItemTypeName(int typeIndex) const { return SequencerItemTypeNames[typeIndex]; }
    virtual const char* GetItemLabel(int index) const
    {
        static char tmps[512];
        snprintf(tmps, 512, "[%02d] %s", index, SequencerItemTypeNames[myItems[index].mType]);
        return tmps;
    }

    virtual void Get(int index, int** start, int** end, int* type, unsigned int* color)
    {
        MySequenceItem& item = myItems[index];
        if (color)
            *color = 0xFFAA8080; // same color for everyone, return color based on type
        if (start)
            *start = &item.mFrameStart;
        if (end)
            *end = &item.mFrameEnd;
        if (type)
            *type = item.mType;
    }
    virtual void Add(int type) { myItems.push_back(MySequenceItem{ type, 0, 10, false }); };
    virtual void Del(int index) { myItems.erase(myItems.begin() + index); }
    virtual void Duplicate(int index) { myItems.push_back(myItems[index]); }

    virtual size_t GetCustomHeight(int index) { return  0; }

    // my datas
    MyTimeline() : mFrameMin(0), mFrameMax(0) {}
    int mFrameMin, mFrameMax;
    struct MySequenceItem
    {
        int mType;
        int mFrameStart, mFrameEnd;
        bool mExpanded;
    };
    std::vector<MySequenceItem> myItems;
    RampEdit rampEdit;

    virtual void DoubleClick(int index) {
        if (myItems[index].mExpanded)
        {
            myItems[index].mExpanded = false;
            return;
        }
        for (auto& item : myItems)
            item.mExpanded = false;
        myItems[index].mExpanded = !myItems[index].mExpanded;
    }

    virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect)
    {
        static const char* labels[] = { "Translation", "Rotation" , "Scale" };

        rampEdit.mMax = ImVec2(float(mFrameMax), 1.f);
        rampEdit.mMin = ImVec2(float(mFrameMin), 0.f);
        draw_list->PushClipRect(legendClippingRect.Min, legendClippingRect.Max, true);
        for (int i = 0; i < 3; i++)
        {
            ImVec2 pta(legendRect.Min.x + 30, legendRect.Min.y + i * 14.f);
            ImVec2 ptb(legendRect.Max.x, legendRect.Min.y + (i + 1) * 14.f);
            draw_list->AddText(pta, rampEdit.mbVisible[i] ? 0xFFFFFFFF : 0x80FFFFFF, labels[i]);
            if (ImRect(pta, ptb).Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(0))
                rampEdit.mbVisible[i] = !rampEdit.mbVisible[i];
        }
        draw_list->PopClipRect();

        ImGui::SetCursorScreenPos(rc.Min);
        ImCurveEdit::Edit(rampEdit, rc.Max - rc.Min, 137 + index, &clippingRect);
    }

};

bool count_frame(int& c)
{
    c++;
    if (c >= 10)
    {
        c = 0;
        return true;
    }
    return false;
}

int main(int, char**)
{
    ImApp::ImApp imApp;

    ImApp::Config config;
    config.mWidth = 1280;
    config.mHeight = 720;
    //config.mFullscreen = true;
    imApp.Init(config);

    int lastUsing = 0;

    float cameraView[16] =
    { 1.f, 0.f, 0.f, 0.f,
      0.f, 1.f, 0.f, 0.f,
      0.f, 0.f, 1.f, 0.f,
      0.f, 0.f, 0.f, 1.f };

    float cameraProjection[16];

    // sequence with default values
    MyTimeline myTimeline;
    myTimeline.mFrameMin = 0;
    myTimeline.mFrameMax = 1600;
    myTimeline.myItems.push_back(MyTimeline::MySequenceItem{ 0, 100, 300, false });
    myTimeline.myItems.push_back(MyTimeline::MySequenceItem{ 1, 20, 300, false });
    myTimeline.myItems.push_back(MyTimeline::MySequenceItem{ 3, 120, 600, false });
    myTimeline.myItems.push_back(MyTimeline::MySequenceItem{ 2, 601, 900, false });
    myTimeline.myItems.push_back(MyTimeline::MySequenceItem{ 4, 90, 999, false });
    myTimeline.myItems.push_back(MyTimeline::MySequenceItem{ 4, 90, 1500, false });
    myTimeline.myItems.push_back(MyTimeline::MySequenceItem{ 4, 1300, 1600, false });
    myTimeline.myItems.push_back(MyTimeline::MySequenceItem{ 4, 1450, 1500, false });

    // sequence with default values
    MySequencer mySequencer;
    mySequencer.mFrameMin = 0;
    mySequencer.mFrameMax = 1000;
    mySequencer.myItems.push_back(MySequencer::MySequenceItem{ 0, 10, 30, false });
    mySequencer.myItems.push_back(MySequencer::MySequenceItem{ 1, 20, 30, false });
    mySequencer.myItems.push_back(MySequencer::MySequenceItem{ 3, 12, 60, false });
    mySequencer.myItems.push_back(MySequencer::MySequenceItem{ 2, 61, 90, false });
    mySequencer.myItems.push_back(MySequencer::MySequenceItem{ 4, 90, 99, false });

    // Camera projection
    bool isPerspective = true;
    float fov = 27.f;
    float viewWidth = 10.f; // for orthographic
    bool infiniteFarPlane = false;
    int handedness = 0; // 0 = right-handed, 1 = left-handed

    bool firstFrame = true;

    int32_t now_frame = 0;
    int cnt = 0;
    // Main loop
    while (!imApp.Done())
    {
        imApp.NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        bool rightHanded = (handedness == 0);
        ImGuizmo::SetOrthographic(!isPerspective);
        ImGuizmo::BeginFrame();

        ImGui::SetNextWindowPos(ImVec2(1024, 100), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(256, 256), ImGuiCond_Appearing);

        // create a window and insert the inspector
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Appearing);

        // let's create the sequencer
        static int selectedEntry = -1;
        static int firstFrame = 0;
        static bool expanded = true;
        static int currentFrame = 100;
        static bool counting = false;
        static int view_size = -1;

        int loop_start = 0;
        int loop_end = 0;
        ImGui::Begin("Editor");
        {
            if (counting && count_frame(cnt))
            {
                currentFrame++;
                if (currentFrame > myTimeline.mFrameMax)
                {
                    currentFrame = 0;
                }
            }

            ImGui::PushItemWidth(130);
            ImGui::InputInt("Frame ", &myTimeline.mFrameMax);
            ImGui::InputInt("Entry ", &selectedEntry);
            ImGui::Checkbox("Play", &counting);
            ImGui::InputInt("FrameScale", &view_size);
            
        }

        ImGui::End();
        {
            ImGui::SetNextWindowPos(ImVec2(10, 210), ImGuiCond_Appearing);
            ImGui::SetNextWindowSize(ImVec2(1700, 200), ImGuiCond_Appearing);

            bool timeline = true;
            ImGuiWindowFlags window_flags = 0;
            window_flags |= ImGuiWindowFlags_NoBackground;
            window_flags |= ImGuiWindowFlags_NoTitleBar;
            window_flags |= ImGuiWindowFlags_NoScrollbar;
            ImGui::Begin("TimeLine", &timeline, window_flags);
            Timeline(&myTimeline, &currentFrame, &selectedEntry,view_size);
            ImGui::End();
        }
        {
            ImGui::SetNextWindowPos(ImVec2(10, 410), ImGuiCond_Appearing);
            ImGui::SetNextWindowSize(ImVec2(1700, 200), ImGuiCond_Appearing);

            int currentFrame1 = 100;
            bool timeline = true;
            ImGuiWindowFlags window_flags = 0;
            window_flags |= ImGuiWindowFlags_NoBackground;
            window_flags |= ImGuiWindowFlags_NoTitleBar;
            ImGui::Begin("Sequencer", &timeline, window_flags);
            Sequencer(&mySequencer, &currentFrame1, &selectedEntry, &firstFrame);
            ImGui::End();
        }

        // render everything
        glClearColor(1.f, 1.f, 1.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        imApp.EndFrame();
    }

    imApp.Finish();

    return 0;
}
