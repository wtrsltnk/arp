
#include <algorithm>
#include <app.hpp>
#include <chrono>
#include <glad/glad.h>
#include <imgui.h>

#include "imgui_knob.h"

const int Note_C_OffsetFromC = 0;
const int Note_CSharp_OffsetFromC = 1;
const int Note_D_OffsetFromC = 2;
const int Note_DSharp_OffsetFromC = 3;
const int Note_E_OffsetFromC = 4;
const int Note_F_OffsetFromC = 5;
const int Note_FSharp_OffsetFromC = 6;
const int Note_G_OffsetFromC = 7;
const int Note_GSharp_OffsetFromC = 8;
const int Note_A_OffsetFromC = 9;
const int Note_ASharp_OffsetFromC = 10;
const int Note_B_OffsetFromC = 11;
const int firstKeyNoteNumber = 24;

void App::OnInit()
{
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 22.0f);

    auto &style = ImGui::GetStyle();
    style.ItemSpacing = ImVec2(10, 10);

    glClearColor(0.56f, 0.7f, 0.67f, 1.0f);

    // RtMidiOut constructor
    try
    {
        _midiout = new RtMidiOut();
    }
    catch (RtMidiError &error)
    {
        error.printMessage();
        exit(EXIT_FAILURE);
    }

    auto ports = _midiout->getPortCount();

    for (unsigned int i = 0; i < ports; i++)
    {
        try
        {
            _portNames.push_back(_midiout->getPortName(i));
        }
        catch (RtMidiError &error)
        {
            error.printMessage();
        }
    }
}

void App::OnResize(int width, int height)
{
    _width = width;
    _height = height;

    glViewport(0, 0, width, height);

    std::cout << _width << "x" << _height << std::endl;
}

ImVec2 buttonSize(50, 80);

void App::PianoKey(
    const char *label,
    int noteNumberInOctave,
    unsigned char velocity)
{
    unsigned char note = firstKeyNoteNumber + (_octaveShift * 12) + noteNumberInOctave;

    ImGui::Button(label, buttonSize);

    if (notesDown.find(note) == notesDown.end() && ImGui::IsItemClicked())
    {
        std::vector<unsigned char> message = {144, note, velocity};
        _midiout->sendMessage(&message);
        notesDown.insert(note);
        if (recordMode)
        {
            _notesToArp.push_back(note);
        }
    }
    else if (notesDown.find(note) != notesDown.end() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        std::vector<unsigned char> message = {128, note, 0};
        _midiout->sendMessage(&message);
        notesDown.erase(note);
    }
}

enum ArpModes
{
    Up = 0,
    Down = 1,
    Inclusive = 2,
    Exclusive = 3,
    Random = 4,
    Order = 5,
};

void App::OnFrame()
{
    int msBetweenNotes = int(60000.0f / _bpm);

    static std::chrono::time_point<std::chrono::steady_clock> lastNote = std::chrono::steady_clock::now();
    static unsigned char lastPlayerNote = 0;
    static int currentDirection = 1;

    auto now = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastNote);

    if (!pauseMode && !recordMode && !_notesToArp.empty())
    {
        auto notes = std::vector<unsigned char>(_notesToArp);

        if (_arpMode != ArpModes::Order)
        {
            std::sort(notes.begin(), notes.end());
        }

        if (_noteLength > 1.0f) _noteLength = 1.0f;
        if (_noteLength <= 0.0f) _noteLength = 0.01f;

        if (lastPlayerNote != 0 && diff.count() > (msBetweenNotes * _noteLength))
        {
            std::vector<unsigned char> message = {
                128,
                lastPlayerNote,
                0,
            };
            _midiout->sendMessage(&message);
            lastPlayerNote = 0;
        }

        if (diff.count() > msBetweenNotes)
        {
            lastPlayerNote = notes[_currentNote];
            std::vector<unsigned char> message = {
                144,
                lastPlayerNote,
                static_cast<unsigned char>(_velocity),
            };
            _midiout->sendMessage(&message);
            lastNote = now;

            if (_arpMode == ArpModes::Up || _arpMode == ArpModes::Order)
            {
                _currentNote++;
                if (_currentNote >= _notesToArp.size())
                {
                    _currentNote = 0;
                }
            }
            else if (_arpMode == ArpModes::Down)
            {
                if (_currentNote == 0)
                {
                    _currentNote = _notesToArp.size() - 1;
                }
                else
                {
                    _currentNote--;
                }
            }
            else if (_arpMode == ArpModes::Inclusive)
            {
                if (currentDirection < 0 && _currentNote == 0)
                {
                    currentDirection = 1;
                }
                else if (currentDirection > 0 && _currentNote + 1 >= _notesToArp.size())
                {
                    currentDirection = -1;
                }
                else
                {
                    _currentNote += currentDirection;
                }
            }
            else if (_arpMode == ArpModes::Exclusive)
            {
                if (currentDirection < 0 && _currentNote == 0)
                {
                    currentDirection = 1;
                }
                else if (currentDirection > 0 && _currentNote + 1 >= _notesToArp.size())
                {
                    currentDirection = -1;
                }

                _currentNote += currentDirection;
            }
            else if (_arpMode == ArpModes::Random)
            {
                _currentNote = std::rand() % _notesToArp.size();
            }
        }
    }
    else
    {
        lastNote = now;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(22.0f / 255.0f, 85.0f / 255.0f, 147.0f / 255.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f / 255.0f, 99.0f / 255.0f, 177.0f / 255.0f, 1.0f));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(float(_width), float(_height)));
    ImGui::Begin("Test", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

    ImVec2 toolBarButtonSize(100, 30);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, toolBarButtonSize.y / 2.0f);

    if (!recordMode && !pauseMode)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(99.0f / 255.0f, 0.0f / 255.0f, 177.0f / 255.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(22.0f / 255.0f, 0.0f / 255.0f, 100.0f / 255.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(22.0f / 255.0f, 0.0f / 255.0f, 100.0f / 255.0f, 1.0f));
    }

    ImGui::Button("Play", toolBarButtonSize);

    if (!recordMode && !pauseMode)
    {
        ImGui::PopStyleColor(3);
    }
    if (ImGui::IsItemClicked())
    {
        pauseMode = !pauseMode;
        if (recordMode)
        {
            pauseMode = false;
        }
        recordMode = false;
        if (pauseMode)
        {
            for (auto &note : _notesToArp)
            {
                std::vector<unsigned char> message = {
                    128,
                    note,
                    0,
                };
                _midiout->sendMessage(&message);
            }
        }
    }

    ImGui::SameLine();

    if (recordMode)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(99.0f / 255.0f, 0.0f / 255.0f, 177.0f / 255.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(22.0f / 255.0f, 0.0f / 255.0f, 100.0f / 255.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(22.0f / 255.0f, 0.0f / 255.0f, 100.0f / 255.0f, 1.0f));
    }

    ImGui::Button("Record", toolBarButtonSize);

    if (recordMode)
    {
        ImGui::PopStyleColor(3);
    }
    ImGui::PopStyleVar();

    if (ImGui::IsItemClicked())
    {
        recordMode = !recordMode;
        if (recordMode)
        {
            for (auto &note : _notesToArp)
            {
                std::vector<unsigned char> message = {
                    128,
                    note,
                    0,
                };
                _midiout->sendMessage(&message);
            }
            _notesToArp.clear();
            _currentNote = 0;
        }
    }

    ImGui::BeginGroup();
    ImGui::Text("Midi output");
    static int e = 0;
    ImGui::RadioButton("No Midi output", &e, 0);
    for (size_t i = 0; i < _portNames.size(); i++)
    {
        ImGui::SameLine();
        if (ImGui::RadioButton(_portNames[i].c_str(), &e, int(i + 1)))
        {
            if (_midiout->isPortOpen())
            {
                _midiout->closePort();
            }
            _midiout->openPort(int(i));
        }
    }
    ImGui::EndGroup();

    ImGui::Separator();

    ImGui::BeginGroup();
    ImGui::Text("Arp Mode");
    ImGui::RadioButton("Up", &_arpMode, ArpModes::Up);

    ImGui::SameLine();

    ImGui::RadioButton("Down", &_arpMode, ArpModes::Down);

    ImGui::SameLine();

    ImGui::RadioButton("Inclusive up/down", &_arpMode, ArpModes::Inclusive);

    ImGui::SameLine();

    ImGui::RadioButton("Exclusive up/down", &_arpMode, ArpModes::Exclusive);

    ImGui::SameLine();

    ImGui::RadioButton("Random", &_arpMode, ArpModes::Random);

    ImGui::SameLine();

    ImGui::RadioButton("Order", &_arpMode, ArpModes::Order);
    ImGui::EndGroup();

    ImGui::Separator();

    ImGui::Knob("BPM", &_bpm, 80.0f, 180.0f, ImVec2(80.0f, 60.0f));

    ImGui::SameLine();

    ImGui::KnobUchar("Octave", &_octaveShift, 0, 8, ImVec2(80.0f, 60.0f));

    ImGui::SameLine();

    ImGui::KnobUchar("Velocity", &_velocity, 0, 127, ImVec2(80.0f, 60.0f));

    ImGui::SameLine();

    ImGui::Knob("Note length", &_noteLength, 0.01f, 0.99f, ImVec2(80.0f, 60.0f));

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Text("Operations on recorded notes");
    if (ImGui::Button("octave down"))
    {
        for (auto &note : _notesToArp)
        {
            note -= 12;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("octave up"))
    {
        for (auto &note : _notesToArp)
        {
            note += 12;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("note down"))
    {
        for (auto &note : _notesToArp)
        {
            note -= 1;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("note up"))
    {
        for (auto &note : _notesToArp)
        {
            note += 1;
        }
    }

    ImGui::EndGroup();

    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, buttonSize.x / 2.0f);

    { // Top Row
        ImGui::InvisibleButton("##", ImVec2(buttonSize.x / 2.0f, buttonSize.y));

        ImGui::SameLine();

        PianoKey("C#", Note_CSharp_OffsetFromC, _velocity);

        ImGui::SameLine();

        PianoKey("D#", Note_DSharp_OffsetFromC, _velocity);

        ImGui::SameLine();

        ImGui::InvisibleButton("##", buttonSize);

        ImGui::SameLine();

        PianoKey("F#", Note_FSharp_OffsetFromC, _velocity);

        ImGui::SameLine();

        PianoKey("G#", Note_GSharp_OffsetFromC, _velocity);

        ImGui::SameLine();

        PianoKey("A#", Note_ASharp_OffsetFromC, _velocity);
    }

    { // Bottom Row

        PianoKey("C", Note_C_OffsetFromC, _velocity);

        ImGui::SameLine();

        PianoKey("D", Note_D_OffsetFromC, _velocity);

        ImGui::SameLine();

        PianoKey("E", Note_E_OffsetFromC, _velocity);

        ImGui::SameLine();

        PianoKey("F", Note_F_OffsetFromC, _velocity);

        ImGui::SameLine();

        PianoKey("G", Note_G_OffsetFromC, _velocity);

        ImGui::SameLine();

        PianoKey("A", Note_A_OffsetFromC, _velocity);

        ImGui::SameLine();

        PianoKey("B", Note_B_OffsetFromC, _velocity);
    }

    ImGui::PopStyleVar();

    ImGui::End();
    ImGui::PopStyleColor(2);
}

void App::OnExit()
{
    for (auto &note : _notesToArp)
    {
        std::vector<unsigned char> message = {
            128,
            note,
            0,
        };
        _midiout->sendMessage(&message);
    }

    _midiout->closePort();
    delete _midiout;
    _midiout = nullptr;
}
